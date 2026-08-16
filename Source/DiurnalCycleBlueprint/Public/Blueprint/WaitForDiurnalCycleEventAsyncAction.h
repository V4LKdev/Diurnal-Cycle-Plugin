#pragma once

#include "CoreMinimal.h"
#include "Engine/CancellableAsyncAction.h"

#include "DiurnalCycleTypes.h"

#include "WaitForDiurnalCycleEventAsyncAction.generated.h"

class UDiurnalCycleSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FWaitForDiurnalCycleEventTriggered,
	FDiurnalTimeEvent, TimeEvent,
	FDiurnalDateTime, OccurrenceTime);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FWaitForDiurnalCycleEventInvalidated,
	FGameplayTag, EventTag);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FWaitForDiurnalCycleEventFailed,
	FGameplayTag, EventTag);

/**
 * Waits for the next occurrence of any event containing a semantic tag.
 *
 * The action is session-scoped and is not serialized into save games.
 * Persistent gameplay systems must recreate waits after restoring their own
 * state.
 *
 * Repeating events wait for their next occurrence. One-off events can be
 * waited for only while a future occurrence remains reachable. Teleporting or
 * restoring the clock past one invalidates an already-active wait.
 *
 * Blocking events are still ordinary scheduled events, so their occurrence
 * also satisfies this wait when reached through forward advancement.
 */
UCLASS(
	BlueprintType,
	meta = (
		ExposedAsyncProxy = "AsyncAction",
		HideThen
	))
class DIURNALCYCLEBLUEPRINT_API
UWaitForDiurnalCycleEventAsyncAction final
	: public UCancellableAsyncAction
{
	GENERATED_BODY()

public:
#pragma region Factory

	/**
	 * Waits for the next occurrence contributed by any event containing EventTag.
	 *
	 * Tag membership is exact; multiple entries may share the tag and the first
	 * matching occurrence completes the action.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Day Night Cycle|Async|Events",
		meta = (
			BlueprintInternalUseOnly = "true",
			WorldContext = "WorldContextObject",
			DisplayName = "Wait for Day Night Cycle Event"
		))
	static UWaitForDiurnalCycleEventAsyncAction*
	WaitForDiurnalCycleEvent(
		UObject* WorldContextObject,
		UPARAM(
			meta = (
				Categories = "DiurnalCycle.TimeEvent"
			))
		FGameplayTag EventTag);

#pragma endregion

#pragma region Outputs

	/** Fired once when the requested scheduled occurrence is reached. */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Day Night Cycle|Async")
	FWaitForDiurnalCycleEventTriggered Triggered;

	/** Fired when a valid, started wait becomes unreachable before triggering. */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Day Night Cycle|Async")
	FWaitForDiurnalCycleEventInvalidated Invalidated;

	/** Fired when configuration/target is invalid or every matching occurrence is already in the past. */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Day Night Cycle|Async")
	FWaitForDiurnalCycleEventFailed Failed;

#pragma endregion

#pragma region AsyncAction

	virtual void Activate() override;
	virtual void Cancel() override;

#pragma endregion

private:
#pragma region ResolutionAndValidation

	UDiurnalCycleSubsystem* ResolveSubsystem() const;

	bool IsTargetReachable(
		const UDiurnalCycleSubsystem& Subsystem) const;

#pragma endregion

#pragma region NativeHandlers

	void HandleTimeEventTriggered(
		const FDiurnalTimeEvent& TimeEvent,
		const FDiurnalDateTime& OccurrenceTime);

	void HandleTimeEventRemoved(
		const FDiurnalTimeEvent& TimeEvent);

	void HandleTimeChanged(
		const FDiurnalTimeChange& Change);

#pragma endregion

#pragma region Completion

	void FinishTriggered(
		const FDiurnalTimeEvent& TimeEvent,
		const FDiurnalDateTime& OccurrenceTime);

	void FinishInvalidated();
	void FinishFailed();
	void CleanupBindings();

#pragma endregion

#pragma region RuntimeState

	TWeakObjectPtr<UObject> WorldContextObject;
	FGameplayTag TargetEventTag;
	TWeakObjectPtr<UDiurnalCycleSubsystem> NativeSubsystem;

	FDelegateHandle TimeEventTriggeredHandle;
	FDelegateHandle TimeEventRemovedHandle;
	FDelegateHandle TimeChangedHandle;

#pragma endregion
};

