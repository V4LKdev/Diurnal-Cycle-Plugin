#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "DiurnalCycleTypes.h"

#include "DiurnalCycleBlueprintSubsystem.generated.h"

class UDiurnalCycleSubsystem;

#pragma region DynamicDelegates

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnDayNightCycleTimeChanged,
	FDiurnalDateTime, PreviousDateTime,
	FDiurnalDateTime, CurrentDateTime,
	EDiurnalTimeChangeReason, Reason);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnDayNightCycleTimeEventTriggered,
	FDiurnalTimeEvent, TimeEvent,
	FDiurnalDateTime, OccurrenceTime);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnDayNightCycleTimeEventOccurrence,
	FDiurnalEventOccurrenceHandle, Occurrence,
	FDiurnalTimeEvent, TimeEvent,
	FDiurnalDateTime, OccurrenceTime);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnDayNightCycleTimeEventAdded,
	FDiurnalTimeEvent, TimeEvent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnDayNightCycleTimeEventRemoved,
	FDiurnalTimeEvent, TimeEvent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnDayNightCyclePauseStateChanged,
	bool, bIsPaused);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnDayNightCycleTimeScaleChanged,
	double, PreviousTimeScale,
	double, NewTimeScale);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnDayNightCycleTimeRangeAdded,
	FDiurnalTimeRange, TimeRange);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnDayNightCycleTimeRangeRemoved,
	FDiurnalTimeRange, TimeRange);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnDayNightCycleTimeRangeEntered,
	FDiurnalTimeRange, TimeRange,
	FDiurnalDateTime, CurrentDateTime);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnDayNightCycleTimeRangeExited,
	FDiurnalTimeRange, TimeRange,
	FDiurnalDateTime, CurrentDateTime);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnDayNightCycleTimeRangeEntryEntered,
	FDiurnalScheduleEntryReference, Entry,
	FDiurnalTimeRange, TimeRange,
	FDiurnalDateTime, CurrentDateTime);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnDayNightCycleTimeRangeEntryExited,
	FDiurnalScheduleEntryReference, Entry,
	FDiurnalTimeRange, TimeRange,
	FDiurnalDateTime, CurrentDateTime);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnDayNightCycleTimeGateActivated,
	FDiurnalTimeEvent, TimeEvent,
	FDiurnalDateTime, ActivationTime);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnDayNightCycleTimeGateOccurrenceActivated,
	FDiurnalEventOccurrenceHandle, Occurrence,
	FDiurnalTimeEvent, TimeEvent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnDayNightCycleTimeGateOccurrenceReleased,
	FDiurnalEventOccurrenceHandle, Occurrence,
	FDiurnalDateTime, ReleaseTime);

#pragma endregion

/**
 * Blueprint-facing notifications for the active day-night-cycle clock.
 *
 * This subsystem shares the lifetime of its owning game instance. It bridges
 * the native clock delegates into persistent Blueprint-assignable dispatchers
 * without adding reflected delegates to UDiurnalCycleSubsystem itself.
 */
UCLASS(
	BlueprintType,
	DisplayName = "Day Night Cycle Notifications")
class DIURNALCYCLEBLUEPRINT_API
UDiurnalCycleBlueprintSubsystem final
	: public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
#pragma region USubsystem

	virtual void Initialize(
		FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

#pragma endregion

#pragma region ClockNotifications

	/** Emitted after the clock reaches a new externally visible time. */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Day Night Cycle|Notifications|Clock")
	FOnDayNightCycleTimeChanged OnTimeChanged;

	/** Emitted when the explicit pause state changes. */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Day Night Cycle|Notifications|Clock")
	FOnDayNightCyclePauseStateChanged OnPauseStateChanged;

	/** Emitted when the clock-speed multiplier changes. */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Day Night Cycle|Notifications|Clock")
	FOnDayNightCycleTimeScaleChanged OnTimeScaleChanged;

#pragma endregion

#pragma region EventNotifications

	/** Emitted for each scheduled occurrence crossed during forward advancement. */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Day Night Cycle|Notifications|Events")
	FOnDayNightCycleTimeEventTriggered OnTimeEventTriggered;

	/** Identity-rich event occurrence used for exact gameplay bookkeeping. */
	UPROPERTY(BlueprintAssignable, Category = "Day Night Cycle|Notifications|Events")
	FOnDayNightCycleTimeEventOccurrence OnTimeEventOccurrence;

	/** Emitted after an event is added to the runtime schedule. */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Day Night Cycle|Notifications|Events")
	FOnDayNightCycleTimeEventAdded OnTimeEventAdded;

	/** Emitted after an event is removed from the runtime schedule. */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Day Night Cycle|Notifications|Events")
	FOnDayNightCycleTimeEventRemoved OnTimeEventRemoved;

#pragma endregion

#pragma region TimeRangeNotifications

	/** Emitted after a time range is added to the runtime schedule. */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Day Night Cycle|Notifications|Time Ranges")
	FOnDayNightCycleTimeRangeAdded OnTimeRangeAdded;

	/** Emitted after a time range is removed from the runtime schedule. */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Day Night Cycle|Notifications|Time Ranges")
	FOnDayNightCycleTimeRangeRemoved OnTimeRangeRemoved;

	/** Emitted when a time range becomes active. */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Day Night Cycle|Notifications|Time Ranges")
	FOnDayNightCycleTimeRangeEntered OnTimeRangeEntered;

	/** Emitted when a previously active time range becomes inactive. */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Day Night Cycle|Notifications|Time Ranges")
	FOnDayNightCycleTimeRangeExited OnTimeRangeExited;

	UPROPERTY(BlueprintAssignable, Category = "Day Night Cycle|Notifications|Time Ranges")
	FOnDayNightCycleTimeRangeEntryEntered OnTimeRangeEntryEntered;

	UPROPERTY(BlueprintAssignable, Category = "Day Night Cycle|Notifications|Time Ranges")
	FOnDayNightCycleTimeRangeEntryExited OnTimeRangeEntryExited;

#pragma endregion

#pragma region TimeGateNotifications

	/** Emitted when a blocking event activates a time gate. */
	UPROPERTY(
		BlueprintAssignable,
		Category = "Day Night Cycle|Notifications|Time Gates")
	FOnDayNightCycleTimeGateActivated OnTimeGateActivated;

	/** Exact blocking occurrence. Use this handle for acknowledgement. */
	UPROPERTY(BlueprintAssignable, Category = "Day Night Cycle|Notifications|Time Gates")
	FOnDayNightCycleTimeGateOccurrenceActivated OnTimeGateOccurrenceActivated;

	UPROPERTY(BlueprintAssignable, Category = "Day Night Cycle|Notifications|Time Gates")
	FOnDayNightCycleTimeGateOccurrenceReleased OnTimeGateOccurrenceReleased;

#pragma endregion

private:
#pragma region NativeHandlers

	void HandleTimeChanged(
		const FDiurnalTimeChange& Change);

	void HandleTimeEventTriggered(
		const FDiurnalTimeEvent& TimeEvent,
		const FDiurnalDateTime& OccurrenceTime);

	void HandleTimeEventOccurrence(
		const FDiurnalEventOccurrenceHandle& Occurrence,
		const FDiurnalTimeEvent& TimeEvent,
		const FDiurnalDateTime& OccurrenceTime);

	void HandleTimeEventAdded(
		const FDiurnalTimeEvent& TimeEvent);

	void HandleTimeEventRemoved(
		const FDiurnalTimeEvent& TimeEvent);

	void HandlePauseStateChanged(
		bool bIsPaused);

	void HandleTimeScaleChanged(
		double PreviousTimeScale,
		double NewTimeScale);

	void HandleTimeRangeAdded(
		const FDiurnalTimeRange& TimeRange);

	void HandleTimeRangeRemoved(
		const FDiurnalTimeRange& TimeRange);

	void HandleTimeRangeEntered(
		const FDiurnalTimeRange& TimeRange,
		const FDiurnalDateTime& CurrentDateTime);

	void HandleTimeRangeExited(
		const FDiurnalTimeRange& TimeRange,
		const FDiurnalDateTime& CurrentDateTime);

	void HandleTimeRangeEntryEntered(
		const FDiurnalScheduleEntryReference& Entry,
		const FDiurnalTimeRange& TimeRange,
		const FDiurnalDateTime& CurrentDateTime);

	void HandleTimeRangeEntryExited(
		const FDiurnalScheduleEntryReference& Entry,
		const FDiurnalTimeRange& TimeRange,
		const FDiurnalDateTime& CurrentDateTime);

	void HandleTimeGateActivated(
		const FDiurnalTimeEvent& TimeEvent,
		const FDiurnalDateTime& ActivationTime);

	void HandleTimeGateOccurrenceActivated(
		const FDiurnalEventOccurrenceHandle& Occurrence,
		const FDiurnalTimeEvent& TimeEvent);

	void HandleTimeGateOccurrenceReleased(
		const FDiurnalEventOccurrenceHandle& Occurrence,
		const FDiurnalDateTime& ReleaseTime);

#pragma endregion

#pragma region RuntimeState

	/** Native clock currently supplying the Blueprint notifications. */
	TWeakObjectPtr<UDiurnalCycleSubsystem> NativeSubsystem;

#pragma endregion
};
