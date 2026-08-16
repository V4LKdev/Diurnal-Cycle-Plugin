#include "Blueprint/WaitForDiurnalCycleTimeRangeAsyncAction.h"

#include "Subsystem/DiurnalCycleSubsystem.h"

// Session-scoped Blueprint async adapter.

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

#pragma region Factory

UWaitForDiurnalCycleTimeRangeAsyncAction*
UWaitForDiurnalCycleTimeRangeAsyncAction::
WaitForDiurnalCycleTimeRangeEntered(
	UObject* WorldContextObject,
	const FGameplayTag RangeTag)
{
	return CreateAction(
		WorldContextObject,
		RangeTag,
		EWaitMode::Enter);
}

UWaitForDiurnalCycleTimeRangeAsyncAction*
UWaitForDiurnalCycleTimeRangeAsyncAction::
WaitForDiurnalCycleTimeRangeExited(
	UObject* WorldContextObject,
	const FGameplayTag RangeTag)
{
	return CreateAction(
		WorldContextObject,
		RangeTag,
		EWaitMode::Exit);
}

UWaitForDiurnalCycleTimeRangeAsyncAction*
UWaitForDiurnalCycleTimeRangeAsyncAction::
CreateAction(
	UObject* WorldContextObject,
	const FGameplayTag RangeTag,
	const EWaitMode WaitMode)
{
	UWaitForDiurnalCycleTimeRangeAsyncAction* Action =
		NewObject<
			UWaitForDiurnalCycleTimeRangeAsyncAction>();

	Action->WorldContextObject =
		WorldContextObject;

	Action->TargetRangeTag =
		RangeTag;

	Action->Mode =
		WaitMode;

	if (IsValid(
			WorldContextObject))
	{
		Action->RegisterWithGameInstance(
			WorldContextObject);
	}

	return Action;
}

#pragma endregion

#pragma region AsyncAction

void UWaitForDiurnalCycleTimeRangeAsyncAction::Activate()
{
	if (!TargetRangeTag.IsValid())
	{
		FinishFailed();
		return;
	}

	UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem();

	if (!Subsystem
		|| !Subsystem->HasTimeRange(TargetRangeTag))
	{
		FinishFailed();
		return;
	}

	NativeSubsystem =
		Subsystem;

	bTargetWasActive =
		Subsystem->IsCurrentTimeInRange(
			TargetRangeTag);

	TimeRangeEnteredHandle =
		Subsystem->OnTimeRangeEntered().AddUObject(
			this,
			&UWaitForDiurnalCycleTimeRangeAsyncAction::
				HandleTimeRangeEntered);

	TimeRangeExitedHandle =
		Subsystem->OnTimeRangeExited().AddUObject(
			this,
			&UWaitForDiurnalCycleTimeRangeAsyncAction::
				HandleTimeRangeExited);

	TimeRangeRemovedHandle =
		Subsystem->OnTimeRangeRemoved().AddUObject(
			this,
			&UWaitForDiurnalCycleTimeRangeAsyncAction::
				HandleTimeRangeRemoved);

	/*
	 * State restoration can replace the complete range schedule without
	 * emitting individual remove notifications for inactive ranges.
	 */
	TimeChangedHandle =
		Subsystem->OnTimeChanged().AddUObject(
			this,
			&UWaitForDiurnalCycleTimeRangeAsyncAction::
				HandleTimeChanged);
}

void UWaitForDiurnalCycleTimeRangeAsyncAction::Cancel()
{
	CleanupBindings();
	Super::Cancel();
}

#pragma endregion

#pragma region Resolution

UDiurnalCycleSubsystem*
UWaitForDiurnalCycleTimeRangeAsyncAction::
ResolveSubsystem() const
{
	const UObject* ContextObject =
		WorldContextObject.Get();

	if (!GEngine
		|| !IsValid(ContextObject))
	{
		return nullptr;
	}

	UWorld* World =
		GEngine->GetWorldFromContextObject(
			ContextObject,
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

#pragma endregion

#pragma region NativeHandlers

void UWaitForDiurnalCycleTimeRangeAsyncAction::
HandleTimeRangeEntered(
	const FDiurnalTimeRange& TimeRange,
	const FDiurnalDateTime& CurrentDateTime)
{
	if (!TimeRange.HasTagExact(TargetRangeTag))
	{
		return;
	}

	const UDiurnalCycleSubsystem* Subsystem = NativeSubsystem.Get();
	const bool bIsNowActive = Subsystem && Subsystem->IsCurrentTimeInRange(TargetRangeTag);
	const bool bEnteredAggregate = !bTargetWasActive && bIsNowActive;
	bTargetWasActive = bIsNowActive;

	if (Mode == EWaitMode::Enter && bEnteredAggregate)
	{
		FinishCompleted(
			TimeRange,
			CurrentDateTime);
	}
}

void UWaitForDiurnalCycleTimeRangeAsyncAction::
HandleTimeRangeExited(
	const FDiurnalTimeRange& TimeRange,
	const FDiurnalDateTime& CurrentDateTime)
{
	if (!TimeRange.HasTagExact(TargetRangeTag))
	{
		return;
	}

	const UDiurnalCycleSubsystem* Subsystem = NativeSubsystem.Get();
	const bool bIsNowActive = Subsystem && Subsystem->IsCurrentTimeInRange(TargetRangeTag);
	const bool bExitedAggregate = bTargetWasActive && !bIsNowActive;
	bTargetWasActive = bIsNowActive;

	if (Mode == EWaitMode::Exit && bExitedAggregate)
	{
		FinishCompleted(
			TimeRange,
			CurrentDateTime);
	}
}

void UWaitForDiurnalCycleTimeRangeAsyncAction::
HandleTimeRangeRemoved(
	const FDiurnalTimeRange& TimeRange)
{
	if (!TimeRange.HasTagExact(TargetRangeTag))
	{
		return;
	}

	/*
	 * Removing an active range emits Removed first and Exited immediately
	 * afterward. Preserve an Exit wait through that synchronous transition so
	 * it completes normally instead of being invalidated prematurely.
	 */
	if (Mode == EWaitMode::Exit
		&& bTargetWasActive)
	{
		return;
	}

	const UDiurnalCycleSubsystem* Subsystem = NativeSubsystem.Get();
	if (!Subsystem || !Subsystem->HasTimeRange(TargetRangeTag))
	{
		FinishInvalidated();
	}
}

void UWaitForDiurnalCycleTimeRangeAsyncAction::
HandleTimeChanged(
	const FDiurnalTimeChange& Change)
{
	if (Change.Reason
		!= EDiurnalTimeChangeReason::StateRestored)
	{
		return;
	}

	const UDiurnalCycleSubsystem* Subsystem =
		NativeSubsystem.Get();

	if (!Subsystem
		|| !Subsystem->HasTimeRange(
			TargetRangeTag))
	{
		FinishInvalidated();
	}
}

#pragma endregion

#pragma region Completion

void UWaitForDiurnalCycleTimeRangeAsyncAction::
FinishCompleted(
	const FDiurnalTimeRange& TimeRange,
	const FDiurnalDateTime& TransitionTime)
{
	if (!ShouldBroadcastDelegates())
	{
		CleanupBindings();
		SetReadyToDestroy();
		return;
	}

	CleanupBindings();

	Completed.Broadcast(
		TimeRange,
		TransitionTime);

	SetReadyToDestroy();
}

void UWaitForDiurnalCycleTimeRangeAsyncAction::
FinishInvalidated()
{
	if (!ShouldBroadcastDelegates())
	{
		CleanupBindings();
		SetReadyToDestroy();
		return;
	}

	const FGameplayTag InvalidatedTag =
		TargetRangeTag;

	CleanupBindings();

	Invalidated.Broadcast(
		InvalidatedTag);

	SetReadyToDestroy();
}

void UWaitForDiurnalCycleTimeRangeAsyncAction::
FinishFailed()
{
	if (!ShouldBroadcastDelegates())
	{
		CleanupBindings();
		SetReadyToDestroy();
		return;
	}

	const FGameplayTag FailedTag =
		TargetRangeTag;

	CleanupBindings();

	Failed.Broadcast(
		FailedTag);

	SetReadyToDestroy();
}

void UWaitForDiurnalCycleTimeRangeAsyncAction::
CleanupBindings()
{
	if (UDiurnalCycleSubsystem* Subsystem =
		NativeSubsystem.Get())
	{
		if (TimeRangeEnteredHandle.IsValid())
		{
			Subsystem->OnTimeRangeEntered().Remove(
				TimeRangeEnteredHandle);
		}

		if (TimeRangeExitedHandle.IsValid())
		{
			Subsystem->OnTimeRangeExited().Remove(
				TimeRangeExitedHandle);
		}

		if (TimeRangeRemovedHandle.IsValid())
		{
			Subsystem->OnTimeRangeRemoved().Remove(
				TimeRangeRemovedHandle);
		}

		if (TimeChangedHandle.IsValid())
		{
			Subsystem->OnTimeChanged().Remove(
				TimeChangedHandle);
		}
	}

	TimeRangeEnteredHandle.Reset();
	TimeRangeExitedHandle.Reset();
	TimeRangeRemovedHandle.Reset();
	TimeChangedHandle.Reset();

	NativeSubsystem.Reset();
}

#pragma endregion
