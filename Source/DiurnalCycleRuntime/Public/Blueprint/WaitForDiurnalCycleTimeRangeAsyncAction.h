#pragma once

#include "CoreMinimal.h"
#include "Engine/CancellableAsyncAction.h"

#include "DiurnalCycleTypes.h"

#include "WaitForDiurnalCycleTimeRangeAsyncAction.generated.h"

class UDiurnalCycleSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FWaitForDiurnalCycleTimeRangeCompleted,
	FDiurnalTimeRange, TimeRange,
	FDiurnalDateTime, TransitionTime);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FWaitForDiurnalCycleTimeRangeInvalidated,
	FGameplayTag, RangeTag);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FWaitForDiurnalCycleTimeRangeFailed,
	FGameplayTag, RangeTag);

/**
 * Waits for the next enter or exit transition of one exact time-range tag.
 *
 * The action waits for a transition, not merely a state. Starting an Enter
 * wait while the range is already active therefore waits for the next future
 * entry after it has exited and re-entered.
 *
 * The action is session-scoped and is not serialized into save games.
 */
UCLASS(
	BlueprintType,
	meta = (
		ExposedAsyncProxy = "AsyncAction",
		HideThen
	))
class DIURNALCYCLERUNTIME_API
UWaitForDiurnalCycleTimeRangeAsyncAction final
	: public UCancellableAsyncAction
{
	GENERATED_BODY()

public:
#pragma region Factory

	/** Waits for the next transition that makes RangeTag active. */
	UFUNCTION(
		BlueprintCallable,
		Category = "Day Night Cycle|Async|Time Ranges",
		meta = (
			BlueprintInternalUseOnly = "true",
			WorldContext = "WorldContextObject",
			DisplayName = "Wait for Day Night Cycle Time Range Entered"
		))
	static UWaitForDiurnalCycleTimeRangeAsyncAction*
	WaitForDiurnalCycleTimeRangeEntered(
		UObject* WorldContextObject,
		UPARAM(
			meta = (
				Categories = "DiurnalCycle.TimeRange"
			))
		FGameplayTag RangeTag);

	/** Waits for the next transition that makes RangeTag inactive. */
	UFUNCTION(
		BlueprintCallable,
		Category = "Day Night Cycle|Async|Time Ranges",
		meta = (
			BlueprintInternalUseOnly = "true",
			WorldContext = "WorldContextObject",
			DisplayName = "Wait for Day Night Cycle Time Range Exited"
		))
	static UWaitForDiurnalCycleTimeRangeAsyncAction*
	WaitForDiurnalCycleTimeRangeExited(
		UObject* WorldContextObject,
		UPARAM(
			meta = (
				Categories = "DiurnalCycle.TimeRange"
			))
		FGameplayTag RangeTag);

#pragma endregion

#pragma region Outputs

	/** Fired once when the requested range transition occurs. */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Day Night Cycle|Async")
	FWaitForDiurnalCycleTimeRangeCompleted Completed;

	/** Fired when the target range is removed before the transition can occur. */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Day Night Cycle|Async")
	FWaitForDiurnalCycleTimeRangeInvalidated Invalidated;

	/** Fired when the wait cannot be started. */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Day Night Cycle|Async")
	FWaitForDiurnalCycleTimeRangeFailed Failed;

#pragma endregion

#pragma region AsyncAction

	virtual void Activate() override;
	virtual void Cancel() override;

#pragma endregion

private:
	enum class EWaitMode : uint8
	{
		Enter,
		Exit
	};

	static UWaitForDiurnalCycleTimeRangeAsyncAction* CreateAction(
		UObject* WorldContextObject,
		FGameplayTag RangeTag,
		EWaitMode WaitMode);

	UDiurnalCycleSubsystem* ResolveSubsystem() const;

	void HandleTimeRangeEntered(
		const FDiurnalTimeRange& TimeRange,
		const FDiurnalDateTime& CurrentDateTime);

	void HandleTimeRangeExited(
		const FDiurnalTimeRange& TimeRange,
		const FDiurnalDateTime& CurrentDateTime);

	void HandleTimeRangeRemoved(
		const FDiurnalTimeRange& TimeRange);

	void HandleTimeChanged(
		const FDiurnalTimeChange& Change);

	void FinishCompleted(
		const FDiurnalTimeRange& TimeRange,
		const FDiurnalDateTime& TransitionTime);

	void FinishInvalidated();
	void FinishFailed();
	void CleanupBindings();

	TWeakObjectPtr<UObject> WorldContextObject;
	FGameplayTag TargetRangeTag;
	EWaitMode Mode =
		EWaitMode::Enter;

	/** Last observed active state of the target range. */
	bool bTargetWasActive = false;

	TWeakObjectPtr<UDiurnalCycleSubsystem> NativeSubsystem;

	FDelegateHandle TimeRangeEnteredHandle;
	FDelegateHandle TimeRangeExitedHandle;
	FDelegateHandle TimeRangeRemovedHandle;
	FDelegateHandle TimeChangedHandle;
};