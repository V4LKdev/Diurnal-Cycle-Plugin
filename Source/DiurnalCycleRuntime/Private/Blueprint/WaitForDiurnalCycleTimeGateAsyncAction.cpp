#include "Blueprint/WaitForDiurnalCycleTimeGateAsyncAction.h"

#include "Subsystem/DiurnalCycleSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

#pragma region Factory

UWaitForDiurnalCycleTimeGateAsyncAction*
UWaitForDiurnalCycleTimeGateAsyncAction::
WaitForDiurnalCycleTimeGateActivated(
	UObject* WorldContextObject,
	const FGameplayTag GateTag)
{
	return CreateAction(
		WorldContextObject,
		GateTag,
		EWaitMode::Activation);
}

UWaitForDiurnalCycleTimeGateAsyncAction*
UWaitForDiurnalCycleTimeGateAsyncAction::
WaitForDiurnalCycleTimeGateReleased(
	UObject* WorldContextObject,
	const FGameplayTag GateTag)
{
	return CreateAction(
		WorldContextObject,
		GateTag,
		EWaitMode::Release);
}

UWaitForDiurnalCycleTimeGateAsyncAction*
UWaitForDiurnalCycleTimeGateAsyncAction::
CreateAction(
	UObject* WorldContextObject,
	const FGameplayTag GateTag,
	const EWaitMode WaitMode)
{
	UWaitForDiurnalCycleTimeGateAsyncAction* Action =
		NewObject<
			UWaitForDiurnalCycleTimeGateAsyncAction>();

	Action->WorldContextObject =
		WorldContextObject;

	Action->TargetGateTag =
		GateTag;

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

void UWaitForDiurnalCycleTimeGateAsyncAction::Activate()
{
	if (!TargetGateTag.IsValid())
	{
		FinishFailed();
		return;
	}

	UDiurnalCycleSubsystem* Subsystem =
		ResolveSubsystem();

	if (!Subsystem)
	{
		FinishFailed();
		return;
	}

	NativeSubsystem =
		Subsystem;

	if (Mode == EWaitMode::Activation)
	{
		if (Subsystem->IsTimeGateActive(
				TargetGateTag))
		{
			FinishCompleted(
				Subsystem->GetDateTime());

			return;
		}

		if (!IsActivationTargetReachable(
				*Subsystem))
		{
			FinishFailed();
			return;
		}
	}
	else if (!Subsystem->IsTimeGateActive(
				TargetGateTag))
	{
		FinishFailed();
		return;
	}

	TimeGateActivatedHandle =
		Subsystem->OnTimeGateActivated().AddUObject(
			this,
			&UWaitForDiurnalCycleTimeGateAsyncAction::
				HandleTimeGateActivated);

	TimeGateReleasedHandle =
		Subsystem->OnTimeGateReleased().AddUObject(
			this,
			&UWaitForDiurnalCycleTimeGateAsyncAction::
				HandleTimeGateReleased);

	TimeEventRemovedHandle =
		Subsystem->OnTimeEventRemoved().AddUObject(
			this,
			&UWaitForDiurnalCycleTimeGateAsyncAction::
				HandleTimeEventRemoved);

	TimeChangedHandle =
		Subsystem->OnTimeChanged().AddUObject(
			this,
			&UWaitForDiurnalCycleTimeGateAsyncAction::
				HandleTimeChanged);
}

void UWaitForDiurnalCycleTimeGateAsyncAction::Cancel()
{
	CleanupBindings();
	Super::Cancel();
}

#pragma endregion

#pragma region ResolutionAndValidation

UDiurnalCycleSubsystem*
UWaitForDiurnalCycleTimeGateAsyncAction::
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

bool UWaitForDiurnalCycleTimeGateAsyncAction::
IsActivationTargetReachable(
	const UDiurnalCycleSubsystem& Subsystem) const
{
	FDiurnalTimeEvent TimeEvent;

	if (!Subsystem.TryGetTimeEvent(
			TargetGateTag,
			TimeEvent)
		|| !TimeEvent.IsBlocking())
	{
		return false;
	}

	FDiurnalDateTime NextOccurrence;

	return Subsystem.TryGetNextOccurrence(
		TargetGateTag,
		NextOccurrence);
}

#pragma endregion

#pragma region NativeHandlers

void UWaitForDiurnalCycleTimeGateAsyncAction::
HandleTimeGateActivated(
	const FDiurnalTimeEvent& TimeEvent,
	const FDiurnalDateTime& ActivationTime)
{
	if (Mode != EWaitMode::Activation
		|| TimeEvent.EventTag
			!= TargetGateTag)
	{
		return;
	}

	FinishCompleted(
		ActivationTime);
}

void UWaitForDiurnalCycleTimeGateAsyncAction::
HandleTimeGateReleased(
	const FGameplayTag GateTag,
	const FDiurnalDateTime& ReleaseTime)
{
	if (Mode != EWaitMode::Release
		|| GateTag != TargetGateTag)
	{
		return;
	}

	FinishCompleted(
		ReleaseTime);
}

void UWaitForDiurnalCycleTimeGateAsyncAction::
HandleTimeEventRemoved(
	const FDiurnalTimeEvent& TimeEvent)
{
	if (Mode != EWaitMode::Activation
		|| TimeEvent.EventTag
			!= TargetGateTag)
	{
		return;
	}

	FinishInvalidated();
}

void UWaitForDiurnalCycleTimeGateAsyncAction::
HandleTimeChanged(
	const FDiurnalTimeChange& Change)
{
	if (Mode != EWaitMode::Activation
		|| (Change.Reason
				!= EDiurnalTimeChangeReason::DateTimeSet
			&& Change.Reason
				!= EDiurnalTimeChangeReason::StateRestored))
	{
		return;
	}

	const UDiurnalCycleSubsystem* Subsystem =
		NativeSubsystem.Get();

	/*
	 * Teleport/restore gate activation is emitted before TimeChanged. If this
	 * action is still alive here, its target did not become active.
	 */
	if (!Subsystem
		|| !IsActivationTargetReachable(
			*Subsystem))
	{
		FinishInvalidated();
	}
}

#pragma endregion

#pragma region Completion

void UWaitForDiurnalCycleTimeGateAsyncAction::
FinishCompleted(
	const FDiurnalDateTime& TransitionTime)
{
	if (!ShouldBroadcastDelegates())
	{
		CleanupBindings();
		SetReadyToDestroy();
		return;
	}

	const FGameplayTag CompletedTag =
		TargetGateTag;

	CleanupBindings();

	Completed.Broadcast(
		CompletedTag,
		TransitionTime);

	SetReadyToDestroy();
}

void UWaitForDiurnalCycleTimeGateAsyncAction::
FinishInvalidated()
{
	if (!ShouldBroadcastDelegates())
	{
		CleanupBindings();
		SetReadyToDestroy();
		return;
	}

	const FGameplayTag InvalidatedTag =
		TargetGateTag;

	CleanupBindings();

	Invalidated.Broadcast(
		InvalidatedTag);

	SetReadyToDestroy();
}

void UWaitForDiurnalCycleTimeGateAsyncAction::
FinishFailed()
{
	if (!ShouldBroadcastDelegates())
	{
		CleanupBindings();
		SetReadyToDestroy();
		return;
	}

	const FGameplayTag FailedTag =
		TargetGateTag;

	CleanupBindings();

	Failed.Broadcast(
		FailedTag);

	SetReadyToDestroy();
}

void UWaitForDiurnalCycleTimeGateAsyncAction::
CleanupBindings()
{
	if (UDiurnalCycleSubsystem* Subsystem =
		NativeSubsystem.Get())
	{
		if (TimeGateActivatedHandle.IsValid())
		{
			Subsystem->OnTimeGateActivated().Remove(
				TimeGateActivatedHandle);
		}

		if (TimeGateReleasedHandle.IsValid())
		{
			Subsystem->OnTimeGateReleased().Remove(
				TimeGateReleasedHandle);
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

	TimeGateActivatedHandle.Reset();
	TimeGateReleasedHandle.Reset();
	TimeEventRemovedHandle.Reset();
	TimeChangedHandle.Reset();

	NativeSubsystem.Reset();
}

#pragma endregion