#include "Blueprint/WaitForDiurnalCycleEventAsyncAction.h"

#include "Subsystem/DiurnalCycleSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

#pragma region Factory

UWaitForDiurnalCycleEventAsyncAction*
UWaitForDiurnalCycleEventAsyncAction::
WaitForDiurnalCycleEvent(
	UObject* WorldContextObject,
	const FGameplayTag EventTag)
{
	UWaitForDiurnalCycleEventAsyncAction* Action =
		NewObject<
			UWaitForDiurnalCycleEventAsyncAction>();

	Action->WorldContextObject =
		WorldContextObject;

	Action->TargetEventTag =
		EventTag;

	if (IsValid(
			WorldContextObject))
	{
		/*
		 * Keep the action alive independently of the Blueprint object that
		 * created it until it completes, fails, invalidates, or is cancelled.
		 */
		Action->RegisterWithGameInstance(
			WorldContextObject);
	}

	return Action;
}

#pragma endregion

#pragma region AsyncAction

void UWaitForDiurnalCycleEventAsyncAction::Activate()
{
	if (!TargetEventTag.IsValid())
	{
		FinishFailed();
		return;
	}

	UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem();

	if (!Subsystem
		|| !IsTargetReachable(
			*Subsystem))
	{
		FinishFailed();
		return;
	}

	NativeSubsystem =
		Subsystem;

	TimeEventTriggeredHandle =
		Subsystem->OnTimeEventTriggered().AddUObject(
			this,
			&UWaitForDiurnalCycleEventAsyncAction::
				HandleTimeEventTriggered);

	TimeEventRemovedHandle =
		Subsystem->OnTimeEventRemoved().AddUObject(
			this,
			&UWaitForDiurnalCycleEventAsyncAction::
				HandleTimeEventRemoved);

	TimeChangedHandle =
		Subsystem->OnTimeChanged().AddUObject(
			this,
			&UWaitForDiurnalCycleEventAsyncAction::
				HandleTimeChanged);
}

void UWaitForDiurnalCycleEventAsyncAction::Cancel()
{
	CleanupBindings();
	Super::Cancel();
}

#pragma endregion

#pragma region ResolutionAndValidation

UDiurnalCycleSubsystem*
UWaitForDiurnalCycleEventAsyncAction::
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

bool UWaitForDiurnalCycleEventAsyncAction::
IsTargetReachable(
	const UDiurnalCycleSubsystem& Subsystem) const
{
	FDiurnalTimeEvent TimeEvent;

	if (!Subsystem.TryGetTimeEvent(
			TargetEventTag,
			TimeEvent))
	{
		return false;
	}

	FDiurnalDateTime NextOccurrence;

	return Subsystem.TryGetNextOccurrence(
		TargetEventTag,
		NextOccurrence);
}

#pragma endregion

#pragma region NativeHandlers

void UWaitForDiurnalCycleEventAsyncAction::
HandleTimeEventTriggered(
	const FDiurnalTimeEvent& TimeEvent,
	const FDiurnalDateTime& OccurrenceTime)
{
	if (TimeEvent.EventTag
		!= TargetEventTag)
	{
		return;
	}

	FinishTriggered(
		TimeEvent,
		OccurrenceTime);
}

void UWaitForDiurnalCycleEventAsyncAction::
HandleTimeEventRemoved(
	const FDiurnalTimeEvent& TimeEvent)
{
	if (TimeEvent.EventTag
		== TargetEventTag)
	{
		FinishInvalidated();
	}
}

void UWaitForDiurnalCycleEventAsyncAction::
HandleTimeChanged(
	const FDiurnalTimeChange& Change)
{
	if (Change.Reason
			!= EDiurnalTimeChangeReason::DateTimeSet
		&& Change.Reason
			!= EDiurnalTimeChangeReason::StateRestored)
	{
		return;
	}

	const UDiurnalCycleSubsystem* Subsystem =
		NativeSubsystem.Get();

	if (!Subsystem
		|| !IsTargetReachable(
			*Subsystem))
	{
		FinishInvalidated();
	}
}

#pragma endregion

#pragma region Completion

void UWaitForDiurnalCycleEventAsyncAction::
FinishTriggered(
	const FDiurnalTimeEvent& TimeEvent,
	const FDiurnalDateTime& OccurrenceTime)
{
	if (!ShouldBroadcastDelegates())
	{
		CleanupBindings();
		SetReadyToDestroy();
		return;
	}

	CleanupBindings();

	Triggered.Broadcast(
		TimeEvent,
		OccurrenceTime);

	SetReadyToDestroy();
}

void UWaitForDiurnalCycleEventAsyncAction::
FinishInvalidated()
{
	if (!ShouldBroadcastDelegates())
	{
		CleanupBindings();
		SetReadyToDestroy();
		return;
	}

	const FGameplayTag InvalidatedTag =
		TargetEventTag;

	CleanupBindings();

	Invalidated.Broadcast(
		InvalidatedTag);

	SetReadyToDestroy();
}

void UWaitForDiurnalCycleEventAsyncAction::
FinishFailed()
{
	if (!ShouldBroadcastDelegates())
	{
		CleanupBindings();
		SetReadyToDestroy();
		return;
	}

	const FGameplayTag FailedTag =
		TargetEventTag;

	CleanupBindings();

	Failed.Broadcast(
		FailedTag);

	SetReadyToDestroy();
}

void UWaitForDiurnalCycleEventAsyncAction::
CleanupBindings()
{
	if (UDiurnalCycleSubsystem* Subsystem =
		NativeSubsystem.Get())
	{
		if (TimeEventTriggeredHandle.IsValid())
		{
			Subsystem->OnTimeEventTriggered().Remove(
				TimeEventTriggeredHandle);
		}

		if (TimeEventRemovedHandle.IsValid())
		{
			Subsystem->OnTimeEventRemoved().Remove(
				TimeEventRemovedHandle);
		}

		if (TimeChangedHandle.IsValid())
		{
			Subsystem->OnTimeChanged().Remove(
				TimeChangedHandle);
		}
	}

	TimeEventTriggeredHandle.Reset();
	TimeEventRemovedHandle.Reset();
	TimeChangedHandle.Reset();

	NativeSubsystem.Reset();
}

#pragma endregion