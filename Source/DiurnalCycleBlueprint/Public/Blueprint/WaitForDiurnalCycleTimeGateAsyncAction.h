#pragma once

#include "CoreMinimal.h"
#include "Engine/CancellableAsyncAction.h"

#include "DiurnalCycleTypes.h"

#include "WaitForDiurnalCycleTimeGateAsyncAction.generated.h"

class UDiurnalCycleSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FWaitForDiurnalCycleTimeGateCompleted,
	FGameplayTag, GateTag,
	FDiurnalDateTime, TransitionTime);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FWaitForDiurnalCycleTimeGateInvalidated,
	FGameplayTag, GateTag);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FWaitForDiurnalCycleTimeGateFailed,
	FGameplayTag, GateTag);

/**
 * Waits for semantic time-gate tag activity.
 *
 * Activation waits complete immediately when the requested gate is already
 * active. Otherwise the target must identify a blocking event with a reachable
 * future occurrence.
 *
 * Release waits can be started only while one or more matching gates are
 * active and complete after all matching occurrences active at start release.
 *
 * The action is session-scoped and is not serialized into save games.
 */
UCLASS(
	BlueprintType,
	meta = (
		ExposedAsyncProxy = "AsyncAction",
		HideThen
	))
class DIURNALCYCLEBLUEPRINT_API
UWaitForDiurnalCycleTimeGateAsyncAction final
	: public UCancellableAsyncAction
{
	GENERATED_BODY()

public:
#pragma region Factory

	/** Waits until GateTag becomes an active time gate. */
	UFUNCTION(
		BlueprintCallable,
		Category = "Day Night Cycle|Async|Time Gates",
		meta = (
			BlueprintInternalUseOnly = "true",
			WorldContext = "WorldContextObject",
			DisplayName = "Wait for Day Night Cycle Time Gate Activated"
		))
	static UWaitForDiurnalCycleTimeGateAsyncAction*
	WaitForDiurnalCycleTimeGateActivated(
		UObject* WorldContextObject,
		UPARAM(
			meta = (
				Categories = "DiurnalCycle.TimeEvent"
			))
		FGameplayTag GateTag);

	/** Waits until the currently active GateTag is released. */
	UFUNCTION(
		BlueprintCallable,
		Category = "Day Night Cycle|Async|Time Gates",
		meta = (
			BlueprintInternalUseOnly = "true",
			WorldContext = "WorldContextObject",
			DisplayName = "Wait for Day Night Cycle Time Gate Released"
		))
	static UWaitForDiurnalCycleTimeGateAsyncAction*
	WaitForDiurnalCycleTimeGateReleased(
		UObject* WorldContextObject,
		UPARAM(
			meta = (
				Categories = "DiurnalCycle.TimeEvent"
			))
		FGameplayTag GateTag);

#pragma endregion

#pragma region Outputs

	/** Fired once when the requested gate state transition is satisfied. */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Day Night Cycle|Async")
	FWaitForDiurnalCycleTimeGateCompleted Completed;

	/** Fired when an active activation wait can no longer reach its target. */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Day Night Cycle|Async")
	FWaitForDiurnalCycleTimeGateInvalidated Invalidated;

	/** Fired when the wait cannot be started. */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Day Night Cycle|Async")
	FWaitForDiurnalCycleTimeGateFailed Failed;

#pragma endregion

#pragma region AsyncAction

	virtual void Activate() override;
	virtual void Cancel() override;

#pragma endregion

private:
	enum class EWaitMode : uint8
	{
		Activation,
		Release
	};

	static UWaitForDiurnalCycleTimeGateAsyncAction* CreateAction(
		UObject* WorldContextObject,
		FGameplayTag GateTag,
		EWaitMode WaitMode);

	UDiurnalCycleSubsystem* ResolveSubsystem() const;

	bool IsActivationTargetReachable(
		const UDiurnalCycleSubsystem& Subsystem) const;

	void HandleTimeGateActivated(
		const FDiurnalTimeEvent& TimeEvent,
		const FDiurnalDateTime& ActivationTime);

	void HandleTimeGateReleased(
		const FDiurnalEventOccurrenceHandle& Occurrence,
		const FDiurnalDateTime& ReleaseTime);

	void HandleTimeEventRemoved(
		const FDiurnalTimeEvent& TimeEvent);

	void HandleTimeChanged(
		const FDiurnalTimeChange& Change);

	void FinishCompleted(
		const FDiurnalDateTime& TransitionTime);

	void FinishInvalidated();
	void FinishFailed();
	void CleanupBindings();

	TWeakObjectPtr<UObject> WorldContextObject;
	FGameplayTag TargetGateTag;
	TSet<FGuid> TargetOccurrenceIds;

	EWaitMode Mode =
		EWaitMode::Activation;

	TWeakObjectPtr<UDiurnalCycleSubsystem> NativeSubsystem;

	FDelegateHandle TimeGateActivatedHandle;
	FDelegateHandle TimeGateReleasedHandle;
	FDelegateHandle TimeEventRemovedHandle;
	FDelegateHandle TimeChangedHandle;
};
