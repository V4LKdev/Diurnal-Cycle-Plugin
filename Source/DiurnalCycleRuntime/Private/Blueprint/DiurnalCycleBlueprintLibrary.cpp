#include "Blueprint/DiurnalCycleBlueprintLibrary.h"

#include "Blueprint/DiurnalCycleBlueprintSubsystem.h"
#include "Subsystem/DiurnalCycleWorldSubsystem.h"
#include "Subsystem/DiurnalCycleSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

#pragma region Resolution

UDiurnalCycleSubsystem*
UDiurnalCycleBlueprintLibrary::ResolveSubsystem(
	const UObject* WorldContextObject)
{
	if (!GEngine
		|| !IsValid(WorldContextObject))
	{
		return nullptr;
	}

	UWorld* World =
		GEngine->GetWorldFromContextObject(
			WorldContextObject,
			EGetWorldErrorMode::ReturnNull);

	if (!IsValid(World))
	{
		return nullptr;
	}

	UGameInstance* GameInstance =
		World->GetGameInstance();

	if (!IsValid(GameInstance))
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<
		UDiurnalCycleSubsystem>();
}

UDiurnalCycleWorldSubsystem*
UDiurnalCycleBlueprintLibrary::ResolveWorldSubsystem(
	const UObject* WorldContextObject)
{
	if (!GEngine
		|| !IsValid(WorldContextObject))
	{
		return nullptr;
	}

	UWorld* World =
		GEngine->GetWorldFromContextObject(
			WorldContextObject,
			EGetWorldErrorMode::ReturnNull);

	if (!IsValid(World))
	{
		return nullptr;
	}

	return World->GetSubsystem<
		UDiurnalCycleWorldSubsystem>();
}

#pragma endregion

#pragma region Availability

bool UDiurnalCycleBlueprintLibrary::
IsDayNightCycleAvailable(
	const UObject* WorldContextObject)
{
	return ResolveSubsystem(
		WorldContextObject) != nullptr;
}

#pragma endregion

#pragma region ClockControl

bool UDiurnalCycleBlueprintLibrary::
SetDayNightCyclePaused(
	const UObject* WorldContextObject,
	const bool bPaused)
{
	UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	if (!Subsystem)
	{
		return false;
	}

	Subsystem->SetPaused(
		bPaused);

	return true;
}

bool UDiurnalCycleBlueprintLibrary::
SetDayNightCycleTimeScale(
	const UObject* WorldContextObject,
	const double NewTimeScale)
{
	UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		&& Subsystem->TrySetTimeScale(
			NewTimeScale);
}

bool UDiurnalCycleBlueprintLibrary::
SetDayNightCycleDateTime(
	const UObject* WorldContextObject,
	const FDiurnalDateTime& NewDateTime)
{
	UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		&& Subsystem->TrySetDateTime(
			NewDateTime);
}

bool UDiurnalCycleBlueprintLibrary::
AdvanceDayNightCycleHours(
	const UObject* WorldContextObject,
	const double GameHours)
{
	UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		&& Subsystem->TryAdvanceHours(
			GameHours);
}

#pragma endregion

#pragma region ClockQueries

bool UDiurnalCycleBlueprintLibrary::
IsDayNightCyclePaused(
	const UObject* WorldContextObject)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		&& Subsystem->IsPaused();
}

FDiurnalDateTime UDiurnalCycleBlueprintLibrary::
GetDayNightCycleDateTime(
	const UObject* WorldContextObject)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		? Subsystem->GetDateTime()
		: FDiurnalDateTime{};
}

FDiurnalTimeOfDay UDiurnalCycleBlueprintLibrary::
GetDayNightCycleTimeOfDay(
	const UObject* WorldContextObject)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		? Subsystem->GetTimeOfDay()
		: FDiurnalTimeOfDay{};
}

int32 UDiurnalCycleBlueprintLibrary::
GetDayNightCycleCurrentDay(
	const UObject* WorldContextObject)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		? Subsystem->GetCurrentDay()
		: 1;
}

double UDiurnalCycleBlueprintLibrary::
GetDayNightCycleTimeOfDayHours(
	const UObject* WorldContextObject)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		? Subsystem->GetTimeOfDayHours()
		: 0.0;
}

double UDiurnalCycleBlueprintLibrary::
GetDayNightCycleDayProgress(
	const UObject* WorldContextObject)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		? Subsystem->GetDayProgress()
		: 0.0;
}

double UDiurnalCycleBlueprintLibrary::
GetDayNightCycleTotalGameHours(
	const UObject* WorldContextObject)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		? Subsystem->GetTotalGameHours()
		: 0.0;
}

double UDiurnalCycleBlueprintLibrary::
GetDayNightCycleTimeScale(
	const UObject* WorldContextObject)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		? Subsystem->GetTimeScale()
		: 0.0;
}

double UDiurnalCycleBlueprintLibrary::
GetDayNightCycleBaseRate(
	const UObject* WorldContextObject)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		? Subsystem->GetRealSecondsPerGameHour()
		: 0.0;
}

double UDiurnalCycleBlueprintLibrary::
GetDayNightCycleEffectiveRate(
	const UObject* WorldContextObject)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	const UDiurnalCycleWorldSubsystem* WorldSubsystem =
		ResolveWorldSubsystem(
			WorldContextObject);

	if (!Subsystem
		|| !WorldSubsystem
		|| Subsystem->IsPaused()
		|| Subsystem->IsBlockedByTimeGate()
		|| !WorldSubsystem->ShouldAdvanceTime())
	{
		return 0.0;
	}

	const double TimeScale =
		Subsystem->GetTimeScale();

	return TimeScale > 0.0
		? Subsystem->GetRealSecondsPerGameHour()
			/ TimeScale
		: 0.0;
}

#pragma endregion

#pragma region WorldPolicy

bool UDiurnalCycleBlueprintLibrary::
DoesCurrentWorldAdvanceDayNightCycle(
	const UObject* WorldContextObject)
{
	const UDiurnalCycleWorldSubsystem* Subsystem =
		ResolveWorldSubsystem(
			WorldContextObject);

	return Subsystem
		&& Subsystem->ShouldAdvanceTime();
}

EDiurnalCycleWorldTimePolicy
UDiurnalCycleBlueprintLibrary::
GetDayNightCycleWorldTimePolicy(
	const UObject* WorldContextObject)
{
	const UDiurnalCycleWorldSubsystem* Subsystem =
		ResolveWorldSubsystem(
			WorldContextObject);

	return Subsystem
		? Subsystem->GetEffectiveTimePolicy()
		: EDiurnalCycleWorldTimePolicy::UseProjectDefault;
}

bool UDiurnalCycleBlueprintLibrary::
HasDayNightCycleWorldTimePolicyOverride(
	const UObject* WorldContextObject)
{
	const UDiurnalCycleWorldSubsystem* Subsystem =
		ResolveWorldSubsystem(
			WorldContextObject);

	return Subsystem
		&& Subsystem->HasRuntimeTimePolicyOverride();
}

bool UDiurnalCycleBlueprintLibrary::
SetDayNightCycleWorldTimePolicyOverride(
	const UObject* WorldContextObject,
	const EDiurnalCycleWorldTimePolicy Policy)
{
	UDiurnalCycleWorldSubsystem* Subsystem =
		ResolveWorldSubsystem(
			WorldContextObject);

	if (!Subsystem)
	{
		return false;
	}

	Subsystem->SetRuntimeTimePolicy(
		Policy);

	return true;
}

bool UDiurnalCycleBlueprintLibrary::
ClearDayNightCycleWorldTimePolicyOverride(
	const UObject* WorldContextObject)
{
	UDiurnalCycleWorldSubsystem* Subsystem =
		ResolveWorldSubsystem(
			WorldContextObject);

	if (!Subsystem)
	{
		return false;
	}

	Subsystem->ClearRuntimeTimePolicy();

	return true;
}

#pragma endregion

#pragma region Events

TArray<FDiurnalTimeEvent>
UDiurnalCycleBlueprintLibrary::
GetDayNightCycleEvents(
	const UObject* WorldContextObject)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	TArray<FDiurnalTimeEvent> Result;

	if (!Subsystem)
	{
		return Result;
	}

	const TConstArrayView<FDiurnalTimeEvent> Events =
		Subsystem->GetTimeEvents();

	Result.Append(
		Events.GetData(),
		Events.Num());

	return Result;
}

bool UDiurnalCycleBlueprintLibrary::
GetDayNightCycleEvent(
	const UObject* WorldContextObject,
	const FGameplayTag EventTag,
	FDiurnalTimeEvent& OutTimeEvent)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	if (!Subsystem)
	{
		OutTimeEvent =
			FDiurnalTimeEvent{};

		return false;
	}

	return Subsystem->TryGetTimeEvent(
		EventTag,
		OutTimeEvent);
}

bool UDiurnalCycleBlueprintLibrary::
HasDayNightCycleEvent(
	const UObject* WorldContextObject,
	const FGameplayTag EventTag)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		&& Subsystem->HasTimeEvent(
			EventTag);
}

bool UDiurnalCycleBlueprintLibrary::
AddDayNightCycleEvent(
	const UObject* WorldContextObject,
	const FDiurnalTimeEvent& TimeEvent)
{
	UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		&& Subsystem->TryAddTimeEvent(
			TimeEvent);
}

bool UDiurnalCycleBlueprintLibrary::
RemoveDayNightCycleEvent(
	const UObject* WorldContextObject,
	const FGameplayTag EventTag)
{
	UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		&& Subsystem->RemoveTimeEvent(
			EventTag);
}

bool UDiurnalCycleBlueprintLibrary::
GetNextDayNightCycleEvent(
	const UObject* WorldContextObject,
	FDiurnalTimeEvent& OutTimeEvent,
	FDiurnalDateTime& OutOccurrenceTime)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	if (!Subsystem)
	{
		OutTimeEvent =
			FDiurnalTimeEvent{};

		OutOccurrenceTime =
			FDiurnalDateTime{};

		return false;
	}

	return Subsystem->TryGetNextTimeEvent(
		OutTimeEvent,
		OutOccurrenceTime);
}

bool UDiurnalCycleBlueprintLibrary::
GetNextDayNightCycleEventOccurrence(
	const UObject* WorldContextObject,
	const FGameplayTag EventTag,
	FDiurnalDateTime& OutOccurrenceTime)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	if (!Subsystem)
	{
		OutOccurrenceTime =
			FDiurnalDateTime{};

		return false;
	}

	return Subsystem->TryGetNextOccurrence(
		EventTag,
		OutOccurrenceTime);
}

FDiurnalTimeEvent
UDiurnalCycleBlueprintLibrary::
MakeDailyTimeEvent(
	const FGameplayTag EventTag,
	const FDiurnalTimeOfDay& TimeOfDay,
	const EDiurnalTimeEventBehavior Behavior)
{
	return FDiurnalTimeEvent(
		EventTag,
		TimeOfDay,
		false,
		1,
		Behavior);
}

FDiurnalTimeEvent
UDiurnalCycleBlueprintLibrary::
MakeDatedTimeEvent(
	const FGameplayTag EventTag,
	const int32 EventDay,
	const FDiurnalTimeOfDay& TimeOfDay,
	const EDiurnalTimeEventBehavior Behavior)
{
	return FDiurnalTimeEvent(
		EventTag,
		TimeOfDay,
		true,
		EventDay,
		Behavior);
}

bool UDiurnalCycleBlueprintLibrary::
IsDayNightCycleEventValid(
	const FDiurnalTimeEvent& TimeEvent)
{
	return TimeEvent.IsValid();
}

#pragma endregion

#pragma region TimeRanges

TArray<FDiurnalTimeRange>
UDiurnalCycleBlueprintLibrary::
GetDayNightCycleTimeRanges(
	const UObject* WorldContextObject)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	TArray<FDiurnalTimeRange> Result;

	if (!Subsystem)
	{
		return Result;
	}

	const TConstArrayView<FDiurnalTimeRange> Ranges =
		Subsystem->GetTimeRanges();

	Result.Append(
		Ranges.GetData(),
		Ranges.Num());

	return Result;
}

bool UDiurnalCycleBlueprintLibrary::
GetDayNightCycleTimeRange(
	const UObject* WorldContextObject,
	const FGameplayTag RangeTag,
	FDiurnalTimeRange& OutTimeRange)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	if (!Subsystem)
	{
		OutTimeRange =
			FDiurnalTimeRange{};

		return false;
	}

	return Subsystem->TryGetTimeRange(
		RangeTag,
		OutTimeRange);
}

bool UDiurnalCycleBlueprintLibrary::
HasDayNightCycleTimeRange(
	const UObject* WorldContextObject,
	const FGameplayTag RangeTag)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		&& Subsystem->HasTimeRange(
			RangeTag);
}

bool UDiurnalCycleBlueprintLibrary::
AddDayNightCycleTimeRange(
	const UObject* WorldContextObject,
	const FDiurnalTimeRange& TimeRange)
{
	UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		&& Subsystem->TryAddTimeRange(
			TimeRange);
}

bool UDiurnalCycleBlueprintLibrary::
RemoveDayNightCycleTimeRange(
	const UObject* WorldContextObject,
	const FGameplayTag RangeTag)
{
	UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		&& Subsystem->RemoveTimeRange(
			RangeTag);
}

bool UDiurnalCycleBlueprintLibrary::
IsTimeOfDayInRange(
	const UObject* WorldContextObject,
	const FGameplayTag RangeTag,
	const FDiurnalTimeOfDay& TimeOfDay)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		&& Subsystem->IsTimeOfDayInRange(
			RangeTag,
			TimeOfDay);
}

bool UDiurnalCycleBlueprintLibrary::
IsCurrentTimeInRange(
	const UObject* WorldContextObject,
	const FGameplayTag RangeTag)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		&& Subsystem->IsCurrentTimeInRange(
			RangeTag);
}

TArray<FGameplayTag>
UDiurnalCycleBlueprintLibrary::
GetActiveDayNightCycleTimeRanges(
	const UObject* WorldContextObject)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		? Subsystem->GetActiveTimeRanges()
		: TArray<FGameplayTag>{};
}

FDiurnalTimeRange
UDiurnalCycleBlueprintLibrary::
MakeDayNightCycleTimeRange(
	const FGameplayTag RangeTag,
	const FDiurnalTimeOfDay& StartTime,
	const FDiurnalTimeOfDay& EndTime)
{
	return FDiurnalTimeRange(
		RangeTag,
		StartTime,
		EndTime);
}

bool UDiurnalCycleBlueprintLibrary::
IsDayNightCycleTimeRangeValid(
	const FDiurnalTimeRange& TimeRange)
{
	return TimeRange.IsValid();
}

#pragma endregion

#pragma region TimeGates

bool UDiurnalCycleBlueprintLibrary::
IsDayNightCycleBlockedByTimeGate(
	const UObject* WorldContextObject)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		&& Subsystem->IsBlockedByTimeGate();
}

TArray<FGameplayTag>
UDiurnalCycleBlueprintLibrary::
GetActiveDayNightCycleTimeGates(
	const UObject* WorldContextObject)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	TArray<FGameplayTag> Result;

	if (!Subsystem)
	{
		return Result;
	}

	const TConstArrayView<FGameplayTag> Gates =
		Subsystem->GetActiveTimeGates();

	Result.Append(
		Gates.GetData(),
		Gates.Num());

	return Result;
}

bool UDiurnalCycleBlueprintLibrary::
IsDayNightCycleTimeGateActive(
	const UObject* WorldContextObject,
	const FGameplayTag GateTag)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		&& Subsystem->IsTimeGateActive(
			GateTag);
}

bool UDiurnalCycleBlueprintLibrary::
ReleaseDayNightCycleTimeGate(
	const UObject* WorldContextObject,
	const FGameplayTag GateTag)
{
	UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		&& Subsystem->ReleaseTimeGate(
			GateTag);
}

int32 UDiurnalCycleBlueprintLibrary::
ReleaseAllDayNightCycleTimeGates(
	const UObject* WorldContextObject)
{
	UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		? Subsystem->ReleaseAllTimeGates()
		: 0;
}

#pragma endregion

#pragma region Persistence

bool UDiurnalCycleBlueprintLibrary::
CaptureDayNightCycleState(
	const UObject* WorldContextObject,
	FDiurnalCycleState& OutState)
{
	const UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	if (!Subsystem)
	{
		OutState =
			FDiurnalCycleState{};

		return false;
	}

	OutState =
		Subsystem->CaptureState();

	return true;
}

bool UDiurnalCycleBlueprintLibrary::
RestoreDayNightCycleState(
	const UObject* WorldContextObject,
	const FDiurnalCycleState& State)
{
	UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem(
			WorldContextObject);

	return Subsystem
		&& Subsystem->TryRestoreState(
			State);
}

#pragma endregion

#pragma region Notifications

UDiurnalCycleBlueprintSubsystem*
UDiurnalCycleBlueprintLibrary::
GetDayNightCycleNotifications(
	const UObject* WorldContextObject)
{
	if (!GEngine
		|| !IsValid(WorldContextObject))
	{
		return nullptr;
	}

	const UWorld* World =
		GEngine->GetWorldFromContextObject(
			WorldContextObject,
			EGetWorldErrorMode::ReturnNull);

	if (!IsValid(World))
	{
		return nullptr;
	}

	const UGameInstance* GameInstance =
		World->GetGameInstance();

	if (!IsValid(GameInstance))
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<
		UDiurnalCycleBlueprintSubsystem>();
}

#pragma endregion

#pragma region TimeUtilities

FDiurnalTimeOfDay
UDiurnalCycleBlueprintLibrary::
GetMidnightTimeOfDay()
{
	return FDiurnalTimeOfDay::Midnight();
}

FDiurnalTimeOfDay
UDiurnalCycleBlueprintLibrary::
GetSixAMTimeOfDay()
{
	return FDiurnalTimeOfDay::SixAM();
}

FDiurnalTimeOfDay
UDiurnalCycleBlueprintLibrary::
GetNoonTimeOfDay()
{
	return FDiurnalTimeOfDay::Noon();
}

FDiurnalTimeOfDay
UDiurnalCycleBlueprintLibrary::
GetSixPMTimeOfDay()
{
	return FDiurnalTimeOfDay::SixPM();
}

FDiurnalTimeOfDay
UDiurnalCycleBlueprintLibrary::
GetTimeOfDayFromDateTime(
	const FDiurnalDateTime& DateTime)
{
	return DateTime.IsValid()
		? DateTime.GetTimeOfDay()
		: FDiurnalTimeOfDay{};
}

FDiurnalDateTime
UDiurnalCycleBlueprintLibrary::
SetDateTimeTimeOfDay(
	const FDiurnalDateTime& DateTime,
	const FDiurnalTimeOfDay& TimeOfDay)
{
	if (!DateTime.IsValid()
		|| !TimeOfDay.IsValid())
	{
		return FDiurnalDateTime{};
	}

	return DateTime.WithTimeOfDay(
		TimeOfDay);
}

bool UDiurnalCycleBlueprintLibrary::
IsDayNightCycleDateTimeValid(
	const FDiurnalDateTime& DateTime)
{
	return DateTime.IsValid();
}

bool UDiurnalCycleBlueprintLibrary::
IsTimeOfDayValid(
	const FDiurnalTimeOfDay& TimeOfDay)
{
	return TimeOfDay.IsValid();
}

bool UDiurnalCycleBlueprintLibrary::
AreDateTimesEqual(
	const FDiurnalDateTime& Left,
	const FDiurnalDateTime& Right)
{
	return Left.IsValid()
		&& Right.IsValid()
		&& Left == Right;
}

bool UDiurnalCycleBlueprintLibrary::
IsDateTimeBefore(
	const FDiurnalDateTime& Left,
	const FDiurnalDateTime& Right)
{
	return Left.IsValid()
		&& Right.IsValid()
		&& Left < Right;
}

bool UDiurnalCycleBlueprintLibrary::
IsDateTimeAfter(
	const FDiurnalDateTime& Left,
	const FDiurnalDateTime& Right)
{
	return Left.IsValid()
		&& Right.IsValid()
		&& Left > Right;
}

bool UDiurnalCycleBlueprintLibrary::
HasDateTimePassed(
	const FDiurnalDateTime& CurrentDateTime,
	const FDiurnalDateTime& TargetDateTime)
{
	return CurrentDateTime.IsValid()
		&& TargetDateTime.IsValid()
		&& CurrentDateTime >= TargetDateTime;
}

int64 UDiurnalCycleBlueprintLibrary::
GetGameSecondsBetweenDateTimes(
	const FDiurnalDateTime& FromDateTime,
	const FDiurnalDateTime& ToDateTime)
{
	if (!FromDateTime.IsValid()
		|| !ToDateTime.IsValid())
	{
		return 0;
	}

	return ToDateTime.ToTotalSeconds()
		- FromDateTime.ToTotalSeconds();
}

double UDiurnalCycleBlueprintLibrary::
GetGameHoursBetweenDateTimes(
	const FDiurnalDateTime& FromDateTime,
	const FDiurnalDateTime& ToDateTime)
{
	return static_cast<double>(
			GetGameSecondsBetweenDateTimes(
				FromDateTime,
				ToDateTime))
		/ DiurnalCycle::GSecondsPerHour;
}

int64 UDiurnalCycleBlueprintLibrary::
GetGameSecondsRemaining(
	const FDiurnalDateTime& CurrentDateTime,
	const FDiurnalDateTime& TargetDateTime)
{
	return FMath::Max<int64>(
		GetGameSecondsBetweenDateTimes(
			CurrentDateTime,
			TargetDateTime),
		0);
}

double UDiurnalCycleBlueprintLibrary::
GetGameHoursRemaining(
	const FDiurnalDateTime& CurrentDateTime,
	const FDiurnalDateTime& TargetDateTime)
{
	return static_cast<double>(
			GetGameSecondsRemaining(
				CurrentDateTime,
				TargetDateTime))
		/ DiurnalCycle::GSecondsPerHour;
}

bool UDiurnalCycleBlueprintLibrary::
HasTimeOfDayPassedToday(
	const FDiurnalDateTime& CurrentDateTime,
	const FDiurnalTimeOfDay& TargetTime)
{
	if (!CurrentDateTime.IsValid()
		|| !TargetTime.IsValid())
	{
		return false;
	}

	return CurrentDateTime.GetTimeOfDay()
		>= TargetTime;
}

int32 UDiurnalCycleBlueprintLibrary::
GetGameSecondsUntilTimeOfDay(
	const FDiurnalDateTime& CurrentDateTime,
	const FDiurnalTimeOfDay& TargetTime)
{
	if (!CurrentDateTime.IsValid()
		|| !TargetTime.IsValid())
	{
		return 0;
	}

	const int32 CurrentSeconds =
		CurrentDateTime.GetTimeOfDay()
			.ToSecondsIntoDay();

	const int32 TargetSeconds =
		TargetTime.ToSecondsIntoDay();

	int32 DeltaSeconds =
		TargetSeconds - CurrentSeconds;

	if (DeltaSeconds < 0)
	{
		DeltaSeconds +=
			static_cast<int32>(
				DiurnalCycle::GSecondsPerDay);
	}

	return DeltaSeconds;
}

double UDiurnalCycleBlueprintLibrary::
GetGameHoursUntilTimeOfDay(
	const FDiurnalDateTime& CurrentDateTime,
	const FDiurnalTimeOfDay& TargetTime)
{
	return static_cast<double>(
			GetGameSecondsUntilTimeOfDay(
				CurrentDateTime,
				TargetTime))
		/ DiurnalCycle::GSecondsPerHour;
}

#pragma endregion