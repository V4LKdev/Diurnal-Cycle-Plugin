#include "Subsystem/DiurnalCycleSubsystem.h"

#include "DiurnalCycleLog.h"
#include "DiurnalCycleSettings.h"
#include "Subsystem/DiurnalCycleWorldSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

#pragma region Lifecycle

UDiurnalCycleSubsystem::UDiurnalCycleSubsystem()
	: FTickableGameObject(
		ETickableTickType::Never)
{
}

void UDiurnalCycleSubsystem::BeginDestroy()
{
	SetTickableTickType(
		ETickableTickType::Never);

	Super::BeginDestroy();
}

void UDiurnalCycleSubsystem::Initialize(
	FSubsystemCollectionBase& Collection)
{
	Super::Initialize(
		Collection);

	ApplySettings();

	LastBroadcastGameSecond =
		static_cast<int64>(
			FMath::Floor(
				TotalGameHours
				* DiurnalCycle::GSecondsPerHour));

	SetTickableTickType(
		ETickableTickType::Conditional);

	UE_LOG(
		LogDiurnalCycle,
		Log,
		TEXT(
			"Initialized clock at %s | Scale: %gx | "
			"Base rate: %g real s/game h | Paused: %s | "
			"Events: %d | Ranges: %d | Active gates: %d."),
		*GetDateTime().ToString(),
		TimeScale,
		RealSecondsPerGameHour,
		bPaused
			? TEXT("yes")
			: TEXT("no"),
		TimeEvents.Num(),
		TimeRanges.Num(),
		ActiveTimeGateOccurrences.Num());
}

void UDiurnalCycleSubsystem::Deinitialize()
{
	SetTickableTickType(
		ETickableTickType::Never);

	bPaused = true;

	TimeEvents.Reset();
	TimeRanges.Reset();
	ActiveTimeGateOccurrences.Reset();
	ActiveScheduleReferences.Reset();
	ActiveScheduleAssets.Reset();
	RuntimeTimeEvents.Reset();
	RuntimeTimeRanges.Reset();
	DisabledEventEntries.Reset();
	DisabledRangeEntries.Reset();
	ResolvedTimeEvents.Reset();
	ResolvedTimeRanges.Reset();

	Super::Deinitialize();
}

#pragma endregion

#pragma region ClockControl

void UDiurnalCycleSubsystem::SetPaused(
	const bool bNewPaused)
{
	if (!bNewPaused)
	{
		PauseReason.Reset();
		bMaximumDateWarningEmitted = false;
	}
	if (bPaused == bNewPaused)
	{
		return;
	}

	bPaused =
		bNewPaused;

	UE_LOG(
		LogDiurnalCycle,
		Verbose,
		TEXT(
			"Clock %s at %s."),
		bPaused
			? TEXT("paused")
			: TEXT("resumed"),
		*GetDateTime().ToString());

	PauseStateChangedEvent.Broadcast(
		bPaused);
}

bool UDiurnalCycleSubsystem::TrySetTimeScale(
	const double NewTimeScale)
{
	if (!DiurnalCycle::IsValidTimeScale(
			NewTimeScale))
	{
		UE_LOG(
			LogDiurnalCycle,
			Warning,
			TEXT(
				"Rejected invalid time scale %g. "
				"Expected a finite value in [%.3f, %.3f]."),
			NewTimeScale,
			DiurnalCycle::GMinimumTimeScale,
			DiurnalCycle::GMaximumTimeScale);

		return false;
	}

	if (TimeScale == NewTimeScale)
	{
		return true;
	}

	const double PreviousTimeScale =
		TimeScale;

	TimeScale =
		NewTimeScale;

	TimeScaleChangedEvent.Broadcast(
		PreviousTimeScale,
		TimeScale);

	UE_LOG(
		LogDiurnalCycle,
		Verbose,
		TEXT(
			"Time scale changed from %gx to %gx%s."),
		PreviousTimeScale,
		TimeScale,
		TimeScale == 0.0
			? TEXT(
				", automatic advancement is stopped")
			: TEXT(""));

	return true;
}

bool UDiurnalCycleSubsystem::TrySetDateTime(
	const FDiurnalDateTime& NewDateTime)
{
	if (!NewDateTime.IsValid())
	{
		UE_LOG(
			LogDiurnalCycle,
			Warning,
			TEXT(
				"Rejected invalid date-time: "
				"Day=%d Hour=%d Minute=%d Second=%d."),
			NewDateTime.Day,
			NewDateTime.Hour,
			NewDateTime.Minute,
			NewDateTime.Second);

		return false;
	}

	const double PreviousHours =
		TotalGameHours;

	const FDiurnalDateTime PreviousDateTime =
		GetDateTime();

	const TArray<FDiurnalResolvedTimeRange>
		PreviousActiveRanges =
			GetActiveTimeRangeDefinitions(
				PreviousDateTime);

	const double NewTotalGameHours =
		NewDateTime.ToTotalHours();
	if (!FMath::IsNearlyEqual(NewTotalGameHours, PreviousHours))
	{
		PauseReason.Reset();
		bMaximumDateWarningEmitted = false;
	}

	/*
	 * Explicit date-time assignment is an exact teleport. Ordinary events are
	 * not replayed, but destination temporal state is reconciled even when the
	 * numerical clock value does not change.
	 */
	TotalGameHours =
		NewTotalGameHours;

	LastBroadcastGameSecond =
		static_cast<int64>(
			FMath::Floor(
				TotalGameHours
				* DiurnalCycle::GSecondsPerHour));

	const FDiurnalDateTime AppliedDateTime =
		GetDateTime();

	SetActiveTimeGates(
		GetScheduledTimeGatesAt(
			AppliedDateTime),
		AppliedDateTime,
		true);

	const TArray<FDiurnalResolvedTimeRange>
		CurrentActiveRanges =
			GetActiveTimeRangeDefinitions(
				AppliedDateTime);

	BroadcastTimeRangeTransitions(
		PreviousActiveRanges,
		CurrentActiveRanges,
		AppliedDateTime);

	if (PreviousHours != TotalGameHours)
	{
		BroadcastTimeChanged(
			PreviousHours,
			TotalGameHours,
			EDiurnalTimeChangeReason::DateTimeSet);
	}

	UE_LOG(
		LogDiurnalCycle,
		Verbose,
		TEXT(
			"Clock set from %s to %s."),
		*PreviousDateTime.ToString(),
		*AppliedDateTime.ToString());

	return true;
}

bool UDiurnalCycleSubsystem::TryAdvanceHours(
	const double GameHours)
{
	if (!FMath::IsFinite(GameHours)
		|| GameHours <= 0.0)
	{
		UE_LOG(
			LogDiurnalCycle,
			Warning,
			TEXT(
				"Rejected invalid manual advancement: %g game hours. "
				"Expected a finite value greater than zero."),
			GameHours);

		return false;
	}

	if (IsBlockedByTimeGate())
	{
		UE_LOG(
			LogDiurnalCycle,
			Verbose,
			TEXT(
				"Ignored manual advancement while blocked "
				"by %d active time gate(s)."),
			ActiveTimeGateOccurrences.Num());

		return false;
	}

	const double RemainingHours =
		DiurnalCycle::MaximumTotalGameHours
		- TotalGameHours;

	if (GameHours > RemainingHours)
	{
		if (!bMaximumDateWarningEmitted)
		{
			UE_LOG(LogDiurnalCycle, Warning,
				TEXT("Clock advancement stopped at %s because adding %g game hours would exceed the maximum representable date. The clock is now paused."),
				*GetDateTime().ToString(), GameHours);
			bMaximumDateWarningEmitted = true;
		}
		PauseReason = TEXT("Maximum representable date reached");
		SetPaused(true);

		return false;
	}

	AdvanceInternal(
		GameHours,
		EDiurnalTimeChangeReason::ManualAdvance);

	return true;
}

#pragma endregion

#pragma region ClockQueries

FDiurnalDateTime UDiurnalCycleSubsystem::GetDateTime() const
{
	return FDiurnalDateTime::FromTotalHours(
		TotalGameHours);
}

int32 UDiurnalCycleSubsystem::GetCurrentDay() const
{
	return GetDateTime().Day;
}

double UDiurnalCycleSubsystem::GetTimeOfDayHours() const
{
	return FMath::Fmod(
		TotalGameHours,
		DiurnalCycle::GHoursPerDay);
}

double UDiurnalCycleSubsystem::GetDayProgress() const
{
	return GetTimeOfDayHours()
		/ DiurnalCycle::GHoursPerDay;
}

#pragma endregion

#pragma region Advancement

void UDiurnalCycleSubsystem::Tick(
	const float DeltaTime)
{
	if (DeltaTime <= 0.0f)
	{
		return;
	}

	const double GameHours =
		static_cast<double>(
			DeltaTime)
		* TimeScale
		/ RealSecondsPerGameHour;

	if (!ensureMsgf(
			FMath::IsFinite(GameHours)
				&& GameHours > 0.0,
			TEXT(
				"Diurnal clock produced invalid advancement: "
				"DeltaTime=%g TimeScale=%g Rate=%g."),
			DeltaTime,
			TimeScale,
			RealSecondsPerGameHour))
	{
		return;
	}

	const double RemainingHours =
		DiurnalCycle::MaximumTotalGameHours
		- TotalGameHours;

	if (GameHours > RemainingHours)
	{
		if (!bMaximumDateWarningEmitted)
		{
			UE_LOG(LogDiurnalCycle, Warning,
				TEXT("Clock advancement stopped at %s because adding %g game hours would exceed the maximum representable date. The clock is now paused."),
				*GetDateTime().ToString(), GameHours);
			bMaximumDateWarningEmitted = true;
		}
		PauseReason = TEXT("Maximum representable date reached");
		SetPaused(true);
		return;
	}

	AdvanceInternal(
		GameHours,
		EDiurnalTimeChangeReason::AutomaticAdvance);
}

void UDiurnalCycleSubsystem::AdvanceInternal(
	const double GameHours,
	const EDiurnalTimeChangeReason Reason)
{
	checkf(
		FMath::IsFinite(GameHours)
			&& GameHours > 0.0,
		TEXT(
			"AdvanceInternal requires positive "
			"finite game hours."));

	checkf(
		GameHours
			<= DiurnalCycle::MaximumTotalGameHours
				- TotalGameHours,
		TEXT(
			"AdvanceInternal would exceed "
			"the supported date range."));

	checkf(
		!IsBlockedByTimeGate(),
		TEXT(
			"AdvanceInternal cannot advance "
			"while blocked by a time gate."));

	const double PreviousHours =
		TotalGameHours;

	const FDiurnalDateTime PreviousDateTime = GetDateTime();

	const TArray<FDiurnalResolvedTimeRange>
		PreviousActiveRanges =
			GetActiveTimeRangeDefinitions(
				PreviousDateTime);

	const double RequestedHours =
		TotalGameHours
		+ GameHours;

	double BlockingOccurrenceHours = 0.0;

	const bool bHitTimeGate =
		TryFindFirstBlockingOccurrence(
			PreviousHours,
			RequestedHours,
			BlockingOccurrenceHours);

	TotalGameHours =
		bHitTimeGate
			? BlockingOccurrenceHours
			: RequestedHours;

	/*
	 * Dispatch uses copied occurrence payloads. When the final timestamp is a
	 * gate occurrence, all gates at that timestamp are activated before events
	 * at that timestamp are emitted.
	 */
	DispatchCrossedTimeEvents(
		PreviousHours,
		TotalGameHours);

	const FDiurnalDateTime CurrentDateTime =
		GetDateTime();

	const TArray<FDiurnalResolvedTimeRange>
		CurrentActiveRanges =
			GetActiveTimeRangeDefinitions(
				CurrentDateTime);

	BroadcastTimeRangeTransitions(
		PreviousActiveRanges,
		CurrentActiveRanges,
		CurrentDateTime);

	const int64 CurrentGameSecond =
		static_cast<int64>(
			FMath::Floor(
				TotalGameHours
					* DiurnalCycle::GSecondsPerHour));

	if (CurrentGameSecond
		!= LastBroadcastGameSecond)
	{
		LastBroadcastGameSecond =
			CurrentGameSecond;

		BroadcastTimeChanged(
			PreviousHours,
			TotalGameHours,
			Reason);
	}
}

void UDiurnalCycleSubsystem::BroadcastTimeChanged(
	const double PreviousHours,
	const double CurrentHours,
	const EDiurnalTimeChangeReason Reason)
{
	FDiurnalTimeChange Change;

	Change.PreviousDateTime =
		FDiurnalDateTime::FromTotalHours(
			PreviousHours);

	Change.CurrentDateTime =
		FDiurnalDateTime::FromTotalHours(
			CurrentHours);

	Change.PreviousTotalGameHours =
		PreviousHours;

	Change.CurrentTotalGameHours =
		CurrentHours;

	Change.Reason =
		Reason;

	TimeChangedEvent.Broadcast(
		Change);
}

#pragma endregion

#pragma region Initialization

void UDiurnalCycleSubsystem::ApplySettings()
{
	const UDiurnalCycleSettings* Settings =
		GetDefault<UDiurnalCycleSettings>();

	check(Settings);

	if (DiurnalCycle::IsValidRealSecondsPerGameHour(
			Settings->RealSecondsPerGameHour))
	{
		RealSecondsPerGameHour =
			Settings->RealSecondsPerGameHour;
	}
	else
	{
		RealSecondsPerGameHour =
			DiurnalCycle::GDefaultRealSecondsPerGameHour;

		UE_LOG(
			LogDiurnalCycle,
			Warning,
			TEXT(
				"Invalid configured RealSecondsPerGameHour (%g). "
				"Using the runtime default %g."),
			Settings->RealSecondsPerGameHour,
			RealSecondsPerGameHour);
	}

	if (DiurnalCycle::IsValidTimeScale(
			Settings->DefaultTimeScale))
	{
		TimeScale =
			Settings->DefaultTimeScale;
	}
	else
	{
		TimeScale =
			DiurnalCycle::GDefaultTimeScale;

		UE_LOG(
			LogDiurnalCycle,
			Warning,
			TEXT(
				"Invalid configured DefaultTimeScale (%g). "
				"Using the runtime default %g."),
			Settings->DefaultTimeScale,
			TimeScale);
	}

	const FDiurnalDateTime StartingDateTime =
		Settings->StartingDateTime.IsValid()
			? Settings->StartingDateTime
			: FDiurnalDateTime(
				1,
				FDiurnalTimeOfDay(9));

	if (!Settings->StartingDateTime.IsValid())
	{
		UE_LOG(
			LogDiurnalCycle,
			Warning,
			TEXT(
				"Invalid configured starting date-time "
				"(Day=%d Hour=%d Minute=%d Second=%d). "
				"Using Day 1, 09:00:00."),
			Settings->StartingDateTime.Day,
			Settings->StartingDateTime.Hour,
			Settings->StartingDateTime.Minute,
			Settings->StartingDateTime.Second);
	}

	TotalGameHours =
		StartingDateTime.ToTotalHours();

	bPaused =
		Settings->bStartPaused;

	ActiveScheduleReferences = Settings->DefaultSchedules;
	RuntimeTimeEvents.Reset();
	RuntimeTimeRanges.Reset();
	DisabledEventEntries.Reset();
	DisabledRangeEntries.Reset();

	if (!RebuildCompiledSchedule(false))
	{
		ActiveScheduleReferences.Reset();
		ActiveScheduleAssets.Reset();
		TimeEvents.Reset();
		TimeRanges.Reset();
		ResolvedTimeEvents.Reset();
		ResolvedTimeRanges.Reset();
	}

	/*
	 * Starting exactly on one or more configured blocking events begins with
	 * those gates active. Initialization itself does not emit notifications.
	 */
	SetActiveTimeGates(
		GetScheduledTimeGatesAt(
			StartingDateTime),
		StartingDateTime,
		false);
}

#pragma endregion

#pragma region Tickable

bool UDiurnalCycleSubsystem::IsTickable() const
{
	return !IsTemplate()
		&& IsValid(
			GetGameInstance())
		&& IsValid(
			GetTickableGameObjectWorld())
		&& !bPaused
		&& !IsBlockedByTimeGate()
		&& TimeScale > 0.0
		&& ShouldAdvanceInCurrentWorld();
}

TStatId UDiurnalCycleSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(
		UDiurnalCycleSubsystem,
		STATGROUP_Tickables);
}

UWorld* UDiurnalCycleSubsystem::GetTickableGameObjectWorld() const
{
	const UGameInstance* GameInstance =
		GetGameInstance();

	return GameInstance
		? GameInstance->GetWorld()
		: nullptr;
}

bool UDiurnalCycleSubsystem::ShouldAdvanceInCurrentWorld() const
{
	UWorld* World =
		GetTickableGameObjectWorld();

	if (!IsValid(World))
	{
		return false;
	}

	const UDiurnalCycleWorldSubsystem* WorldSubsystem =
		World->GetSubsystem<
			UDiurnalCycleWorldSubsystem>();

	return IsValid(WorldSubsystem)
		&& WorldSubsystem->ShouldAdvanceTime();
}

#pragma endregion
