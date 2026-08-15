#include "GameplayDebuggerCategory_DiurnalCycle.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "DiurnalCycleTypes.h"
#include "Subsystem/DiurnalCycleWorldSubsystem.h"
#include "DiurnalCycleWorldSettings.h"
#include "Subsystem/DiurnalCycleSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameplayDebuggerTypes.h"
#include "InputCoreTypes.h"

namespace
{
	const TCHAR* GetNetModeText(
		const ENetMode NetMode)
	{
		switch (NetMode)
		{
		case NM_Standalone:
			return TEXT("Standalone");

		case NM_DedicatedServer:
			return TEXT("Dedicated Server");

		case NM_ListenServer:
			return TEXT("Listen Server");

		case NM_Client:
			return TEXT("Client");

		default:
			return TEXT("Unknown");
		}
	}

	const TCHAR* GetWorldTimePolicyText(
		const uint8 PolicyValue)
	{
		switch (static_cast<EDiurnalCycleWorldTimePolicy>(
			PolicyValue))
		{
		case EDiurnalCycleWorldTimePolicy::UseProjectDefault:
			return TEXT("Project Default");

		case EDiurnalCycleWorldTimePolicy::Advance:
			return TEXT("Advance");

		case EDiurnalCycleWorldTimePolicy::Freeze:
			return TEXT("Freeze");

		default:
			return TEXT("Unknown");
		}
	}

	FString FormatTagList(
		const TArray<FName>& Tags)
	{
		if (Tags.IsEmpty())
		{
			return TEXT("None");
		}

		constexpr int32 MaxDisplayedTags = 4;

		const int32 DisplayCount =
			FMath::Min(
				Tags.Num(),
				MaxDisplayedTags);

		TArray<FString> Labels;
		Labels.Reserve(
			DisplayCount);

		for (int32 Index = 0;
			 Index < DisplayCount;
			 ++Index)
		{
			Labels.Add(
				Tags[Index].ToString());
		}

		FString Result =
			FString::Join(
				Labels,
				TEXT(", "));

		if (Tags.Num() > DisplayCount)
		{
			Result += FString::Printf(
				TEXT(" (+%d)"),
				Tags.Num() - DisplayCount);
		}

		return Result;
	}

	FString BuildStopReasonText(
		const bool bPaused,
		const bool bZeroScale,
		const bool bGateBlocked,
		const bool bWorldPolicyAvailable,
		const bool bWorldAllowsAdvancement)
	{
		TArray<FString> Reasons;

		if (bPaused)
		{
			Reasons.Add(
				TEXT("Paused"));
		}

		if (bZeroScale)
		{
			Reasons.Add(
				TEXT("Zero Scale"));
		}

		if (bGateBlocked)
		{
			Reasons.Add(
				TEXT("Time Gate"));
		}

		if (!bWorldPolicyAvailable)
		{
			Reasons.Add(
				TEXT("World Policy Unavailable"));
		}
		else if (!bWorldAllowsAdvancement)
		{
			Reasons.Add(
				TEXT("World Frozen"));
		}

		return FString::Join(
			Reasons,
			TEXT(", "));
	}
}

#pragma region Construction

FGameplayDebuggerCategory_DiurnalCycle::
FGameplayDebuggerCategory_DiurnalCycle()
{
	bShowOnlyWithDebugActor =
		false;

	SetDataPackReplication(
		&Data);

	BindKeyPress(
		FGameplayDebuggerInputHandlerConfig(
			TEXT("TogglePause"),
			EKeys::P.GetFName()),
		this,
		&FGameplayDebuggerCategory_DiurnalCycle::
			TogglePaused,
		EGameplayDebuggerInputMode::Replicated);

	BindKeyPress(
		FGameplayDebuggerInputHandlerConfig(
			TEXT("RewindHour"),
			EKeys::Comma.GetFName()),
		this,
		&FGameplayDebuggerCategory_DiurnalCycle::
			RewindHour,
		EGameplayDebuggerInputMode::Replicated);

	BindKeyPress(
		FGameplayDebuggerInputHandlerConfig(
			TEXT("AdvanceHour"),
			EKeys::Period.GetFName()),
		this,
		&FGameplayDebuggerCategory_DiurnalCycle::
			AdvanceHour,
		EGameplayDebuggerInputMode::Replicated);

	BindKeyPress(
		FGameplayDebuggerInputHandlerConfig(
			TEXT("DecreaseTimeScale"),
			EKeys::Hyphen.GetFName()),
		this,
		&FGameplayDebuggerCategory_DiurnalCycle::
			DecreaseTimeScale,
		EGameplayDebuggerInputMode::Replicated);

	BindKeyPress(
		FGameplayDebuggerInputHandlerConfig(
			TEXT("IncreaseTimeScale"),
			EKeys::Equals.GetFName()),
		this,
		&FGameplayDebuggerCategory_DiurnalCycle::
			IncreaseTimeScale,
		EGameplayDebuggerInputMode::Replicated);

	BindKeyPress(
		FGameplayDebuggerInputHandlerConfig(
			TEXT("ReleaseTimeGates"),
			EKeys::G.GetFName()),
		this,
		&FGameplayDebuggerCategory_DiurnalCycle::
			ReleaseTimeGates,
		EGameplayDebuggerInputMode::Replicated);

	BindKeyPress(
		FGameplayDebuggerInputHandlerConfig(
			TEXT("CycleWorldTimePolicyOverride"),
			EKeys::O.GetFName()),
		this,
		&FGameplayDebuggerCategory_DiurnalCycle::
			CycleWorldTimePolicyOverride,
		EGameplayDebuggerInputMode::Replicated);
}

#pragma endregion

#pragma region FGameplayDebuggerCategory

void FGameplayDebuggerCategory_DiurnalCycle::CollectData(
	APlayerController* OwnerPC,
	AActor* /* DebugActor */)
{
	Data =
		FRepData{};

	CachedSubsystem.Reset();
	CachedWorldSubsystem.Reset();

	UWorld* World =
		OwnerPC
			? OwnerPC->GetWorld()
			: nullptr;

	if (!IsValid(World))
	{
		return;
	}

	UGameInstance* GameInstance =
		World->GetGameInstance();

	UDiurnalCycleSubsystem* Subsystem =
		GameInstance
			? GameInstance->GetSubsystem<
				UDiurnalCycleSubsystem>()
			: nullptr;

	if (!IsValid(Subsystem))
	{
		return;
	}

	UDiurnalCycleWorldSubsystem* WorldSubsystem =
		World->GetSubsystem<
			UDiurnalCycleWorldSubsystem>();

	CachedSubsystem =
		Subsystem;

	CachedWorldSubsystem =
		WorldSubsystem;

	const FDiurnalDateTime DateTime =
		Subsystem->GetDateTime();

	Data.bAvailable =
		true;

	Data.bPaused =
		Subsystem->IsPaused();

	Data.bBlockedByTimeGate =
		Subsystem->IsBlockedByTimeGate();

	Data.WorldName =
		GetNameSafe(
			World);

	Data.NetMode =
		static_cast<uint8>(
			World->GetNetMode());

	Data.Day =
		DateTime.Day;

	Data.Hour =
		DateTime.Hour;

	Data.Minute =
		DateTime.Minute;

	Data.Second =
		DateTime.Second;

	Data.TimeScale =
		Subsystem->GetTimeScale();

	Data.TotalGameHours =
		Subsystem->GetTotalGameHours();

	Data.DayProgress =
		Subsystem->GetDayProgress();

	Data.BaseRealSecondsPerGameHour =
		Subsystem->GetRealSecondsPerGameHour();

	if (IsValid(WorldSubsystem))
	{
		Data.bWorldPolicyAvailable =
			true;

		Data.bWorldAllowsAdvancement =
			WorldSubsystem->ShouldAdvanceTime();

		Data.bHasRuntimeWorldPolicyOverride =
			WorldSubsystem->
				HasRuntimeTimePolicyOverride();

		Data.ConfiguredWorldPolicy =
			static_cast<uint8>(
				WorldSubsystem->
					GetConfiguredTimePolicy());

		Data.EffectiveWorldPolicy =
			static_cast<uint8>(
				WorldSubsystem->
					GetEffectiveTimePolicy());
	}

	const bool bAutomaticAdvancementEnabled =
		!Data.bPaused
		&& !Data.bBlockedByTimeGate
		&& Data.TimeScale > 0.0
		&& Data.bWorldPolicyAvailable
		&& Data.bWorldAllowsAdvancement;

	Data.EffectiveRealSecondsPerGameHour =
		bAutomaticAdvancementEnabled
			? Data.BaseRealSecondsPerGameHour
				/ Data.TimeScale
			: 0.0;

	const TConstArrayView<FDiurnalTimeEvent> Events =
		Subsystem->GetTimeEvents();

	Data.EventCount =
		Events.Num();

	for (const FDiurnalTimeEvent& Event : Events)
	{
		if (Event.bDatedEvent)
		{
			++Data.DatedEventCount;
		}
		else
		{
			++Data.DailyEventCount;
		}

		if (Event.IsBlocking())
		{
			++Data.BlockingEventCount;
		}
	}

	Data.TimeRangeCount =
		Subsystem->GetTimeRanges().Num();

	for (const FGameplayTag RangeTag :
		 Subsystem->GetActiveTimeRanges())
	{
		Data.ActiveTimeRanges.Add(
			RangeTag.GetTagName());
	}

	for (const FGameplayTag GateTag :
		 Subsystem->GetActiveTimeGates())
	{
		Data.ActiveTimeGates.Add(
			GateTag.GetTagName());
	}

	FDiurnalTimeEvent NextEvent;
	FDiurnalDateTime NextOccurrenceTime;

	Data.bHasNextEvent =
		Subsystem->TryGetNextTimeEvent(
			NextEvent,
			NextOccurrenceTime);

	if (Data.bHasNextEvent)
	{
		Data.NextEventName =
			NextEvent.EventTag.GetTagName();

		Data.bNextEventDated =
			NextEvent.bDatedEvent;

		Data.bNextEventBlocking =
			NextEvent.IsBlocking();

		Data.NextEventDay =
			NextOccurrenceTime.Day;

		Data.NextEventHour =
			NextOccurrenceTime.Hour;

		Data.NextEventMinute =
			NextOccurrenceTime.Minute;

		Data.NextEventSecond =
			NextOccurrenceTime.Second;
	}
}

void FGameplayDebuggerCategory_DiurnalCycle::DrawData(
	APlayerController* /* OwnerPC */,
	FGameplayDebuggerCanvasContext& CanvasContext)
{
	if (!Data.bAvailable)
	{
		CanvasContext.Printf(
			TEXT(
				"{yellow}Day Night Cycle runtime "
				"is unavailable."));

		return;
	}

	const bool bAutomaticAdvancementEnabled =
		!Data.bPaused
		&& !Data.bBlockedByTimeGate
		&& Data.TimeScale > 0.0
		&& Data.bWorldPolicyAvailable
		&& Data.bWorldAllowsAdvancement;

	const FString StopReasonText =
		BuildStopReasonText(
			Data.bPaused,
			Data.TimeScale <= 0.0,
			Data.bBlockedByTimeGate,
			Data.bWorldPolicyAvailable,
			Data.bWorldAllowsAdvancement);

	const FString ActiveRangeText =
		FormatTagList(
			Data.ActiveTimeRanges);

	const FString ActiveGateText =
		FormatTagList(
			Data.ActiveTimeGates);

	CanvasContext.Printf(
		TEXT(
			"{yellow}World: {white}%s "
			"{yellow}| Net: {white}%s"),
		*Data.WorldName,
		GetNetModeText(
			static_cast<ENetMode>(
				Data.NetMode)));

	CanvasContext.Printf(
		TEXT(
			"{yellow}Time: {white}"
			"Day %d, %02d:%02d:%02d "
			"{yellow}| Day: {white}%.1f%% "
			"{yellow}| Total: {white}%g h"),
		Data.Day,
		Data.Hour,
		Data.Minute,
		Data.Second,
		Data.DayProgress * 100.0,
		Data.TotalGameHours);

	if (bAutomaticAdvancementEnabled)
	{
		CanvasContext.Printf(
			TEXT(
				"{yellow}Automatic Advancement: "
				"{green}Running"));
	}
	else
	{
		CanvasContext.Printf(
			TEXT(
				"{yellow}Automatic Advancement: "
				"{red}Stopped "
				"{yellow}| Reason: {white}%s"),
			*StopReasonText);
	}

	CanvasContext.Printf(
		TEXT(
			"{yellow}Clock: "
			"{white}%gx "
			"{yellow}| Base: {white}%g s/game h "
			"{yellow}| Effective: %s%g s/game h"),
		Data.TimeScale,
		Data.BaseRealSecondsPerGameHour,
		bAutomaticAdvancementEnabled
			? TEXT("{white}")
			: TEXT("{red}"),
		Data.EffectiveRealSecondsPerGameHour);

	if (Data.bWorldPolicyAvailable)
	{
		CanvasContext.Printf(
			TEXT(
				"{yellow}World Policy: "
				"{white}%s configured "
				"{yellow}| Effective: {white}%s "
				"{yellow}| Runtime Override: {white}%s"),
			GetWorldTimePolicyText(
				Data.ConfiguredWorldPolicy),
			GetWorldTimePolicyText(
				Data.EffectiveWorldPolicy),
			Data.bHasRuntimeWorldPolicyOverride
				? TEXT("Yes")
				: TEXT("No"));
	}
	else
	{
		CanvasContext.Printf(
			TEXT(
				"{yellow}World Policy: "
				"{red}Unavailable"));
	}

	CanvasContext.Printf(
		TEXT(
			"{yellow}Events: {white}%d "
			"{yellow}| Daily: {white}%d "
			"{yellow}| Dated: {white}%d "
			"{yellow}| Gates: {white}%d"),
		Data.EventCount,
		Data.DailyEventCount,
		Data.DatedEventCount,
		Data.BlockingEventCount);

	if (Data.bHasNextEvent)
	{
		CanvasContext.Printf(
			TEXT(
				"{yellow}Next Event: {white}%s "
				"{yellow}| %s / %s "
				"{yellow}| At: {white}"
				"Day %d, %02d:%02d:%02d"),
			*Data.NextEventName.ToString(),
			Data.bNextEventDated
				? TEXT("Dated")
				: TEXT("Daily"),
			Data.bNextEventBlocking
				? TEXT("Block Time")
				: TEXT("Notify"),
			Data.NextEventDay,
			Data.NextEventHour,
			Data.NextEventMinute,
			Data.NextEventSecond);
	}
	else
	{
		CanvasContext.Printf(
			TEXT(
				"{yellow}Next Event: {white}None"));
	}

	CanvasContext.Printf(
		TEXT(
			"{yellow}Time Ranges: {white}%d "
			"{yellow}| Active: {white}%s"),
		Data.TimeRangeCount,
		*ActiveRangeText);

	CanvasContext.Printf(
		TEXT(
			"{yellow}Active Time Gates: %s%s"),
		Data.ActiveTimeGates.IsEmpty()
			? TEXT("{white}")
			: TEXT("{red}"),
		*ActiveGateText);

	CanvasContext.Printf(
		TEXT(""));

	CanvasContext.Printf(
		TEXT(
			"{cyan}[P] {white}Pause/Resume  "
			"{cyan}[,] {white}-1 Hour  "
			"{cyan}[.] {white}+1 Hour  "
			"{cyan}[-] {white}Half Speed  "
			"{cyan}[=] {white}Double Speed"));

	CanvasContext.Printf(
		TEXT(
			"{cyan}[G] {white}Release All Gates  "
			"{cyan}[O] {white}Cycle World Override "
			"{grey}(Default -> Freeze -> Advance)"));
}

#pragma endregion

#pragma region Factory

TSharedRef<FGameplayDebuggerCategory>
FGameplayDebuggerCategory_DiurnalCycle::MakeInstance()
{
	return MakeShared<
		FGameplayDebuggerCategory_DiurnalCycle>();
}

#pragma endregion

#pragma region ReplicatedData

void FGameplayDebuggerCategory_DiurnalCycle::
FRepData::Serialize(
	FArchive& Ar)
{
	Ar << bAvailable;

	Ar << bPaused;
	Ar << bBlockedByTimeGate;

	Ar << bWorldPolicyAvailable;
	Ar << bWorldAllowsAdvancement;
	Ar << bHasRuntimeWorldPolicyOverride;

	Ar << NetMode;
	Ar << ConfiguredWorldPolicy;
	Ar << EffectiveWorldPolicy;

	Ar << Day;
	Ar << Hour;
	Ar << Minute;
	Ar << Second;

	Ar << TimeScale;
	Ar << TotalGameHours;
	Ar << DayProgress;

	Ar << BaseRealSecondsPerGameHour;
	Ar << EffectiveRealSecondsPerGameHour;

	Ar << WorldName;

	Ar << EventCount;
	Ar << DailyEventCount;
	Ar << DatedEventCount;
	Ar << BlockingEventCount;

	Ar << TimeRangeCount;
	Ar << ActiveTimeRanges;
	Ar << ActiveTimeGates;

	Ar << bHasNextEvent;
	Ar << bNextEventDated;
	Ar << bNextEventBlocking;

	Ar << NextEventName;

	Ar << NextEventDay;
	Ar << NextEventHour;
	Ar << NextEventMinute;
	Ar << NextEventSecond;
}

#pragma endregion

#pragma region RuntimeResolution

UDiurnalCycleSubsystem*
FGameplayDebuggerCategory_DiurnalCycle::
GetSubsystem() const
{
	return CachedSubsystem.Get();
}

UDiurnalCycleWorldSubsystem*
FGameplayDebuggerCategory_DiurnalCycle::
GetWorldSubsystem() const
{
	return CachedWorldSubsystem.Get();
}

#pragma endregion

#pragma region InputActions

void FGameplayDebuggerCategory_DiurnalCycle::
TogglePaused()
{
	UDiurnalCycleSubsystem* Subsystem =
		GetSubsystem();

	if (!Subsystem)
	{
		return;
	}

	Subsystem->SetPaused(
		!Subsystem->IsPaused());

	ForceImmediateCollect();
}

void FGameplayDebuggerCategory_DiurnalCycle::
AdvanceHour()
{
	UDiurnalCycleSubsystem* Subsystem =
		GetSubsystem();

	if (Subsystem
		&& Subsystem->TryAdvanceHours(
			1.0))
	{
		ForceImmediateCollect();
	}
}

void FGameplayDebuggerCategory_DiurnalCycle::
RewindHour()
{
	UDiurnalCycleSubsystem* Subsystem =
		GetSubsystem();

	if (!Subsystem)
	{
		return;
	}

	const double NewTotalHours =
		FMath::Max(
			0.0,
			Subsystem->GetTotalGameHours()
				- 1.0);

	if (Subsystem->TrySetDateTime(
			FDiurnalDateTime::FromTotalHours(
				NewTotalHours)))
	{
		ForceImmediateCollect();
	}
}

void FGameplayDebuggerCategory_DiurnalCycle::
IncreaseTimeScale()
{
	UDiurnalCycleSubsystem* Subsystem =
		GetSubsystem();

	if (!Subsystem)
	{
		return;
	}

	const double CurrentScale =
		Subsystem->GetTimeScale();

	const double NewScale =
		CurrentScale <= 0.0
			? DiurnalCycle::GDefaultTimeScale
			: FMath::Min(
				CurrentScale * 2.0,
				DiurnalCycle::GMaximumTimeScale);

	if (Subsystem->TrySetTimeScale(
			NewScale))
	{
		ForceImmediateCollect();
	}
}

void FGameplayDebuggerCategory_DiurnalCycle::
DecreaseTimeScale()
{
	UDiurnalCycleSubsystem* Subsystem =
		GetSubsystem();

	if (!Subsystem)
	{
		return;
	}

	const double CurrentScale =
		Subsystem->GetTimeScale();

	const double NewScale =
		CurrentScale <= 0.0
			? 0.0
			: FMath::Max(
				CurrentScale * 0.5,
				DiurnalCycle::GMinimumTimeScale);

	if (Subsystem->TrySetTimeScale(
			NewScale))
	{
		ForceImmediateCollect();
	}
}

void FGameplayDebuggerCategory_DiurnalCycle::
ReleaseTimeGates()
{
	UDiurnalCycleSubsystem* Subsystem =
		GetSubsystem();

	if (Subsystem
		&& Subsystem->ReleaseAllTimeGates() > 0)
	{
		ForceImmediateCollect();
	}
}

void FGameplayDebuggerCategory_DiurnalCycle::
CycleWorldTimePolicyOverride()
{
	UDiurnalCycleWorldSubsystem* WorldSubsystem =
		GetWorldSubsystem();

	if (!WorldSubsystem)
	{
		return;
	}

	if (!WorldSubsystem->
		HasRuntimeTimePolicyOverride())
	{
		WorldSubsystem->SetRuntimeTimePolicy(
			EDiurnalCycleWorldTimePolicy::Freeze);
	}
	else if (WorldSubsystem->
		GetEffectiveTimePolicy()
			== EDiurnalCycleWorldTimePolicy::Freeze)
	{
		WorldSubsystem->SetRuntimeTimePolicy(
			EDiurnalCycleWorldTimePolicy::Advance);
	}
	else
	{
		WorldSubsystem->ClearRuntimeTimePolicy();
	}

	ForceImmediateCollect();
}

#pragma endregion

#endif // WITH_GAMEPLAY_DEBUGGER