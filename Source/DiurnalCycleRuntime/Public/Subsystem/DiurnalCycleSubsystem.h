#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"

#include "DiurnalCycleTypes.h"

#include "DiurnalCycleSubsystem.generated.h"

class UWorld;

#pragma region NativeDelegates

/**
 * Broadcast after the clock reaches a new externally visible time.
 *
 * Forward advancement emits this notification when the whole game second
 * changes. Explicit date-time changes and state restoration emit it after the
 * resulting temporal state has been fully applied.
 */
DECLARE_MULTICAST_DELEGATE_OneParam(
	FOnDiurnalTimeChanged,
	const FDiurnalTimeChange&);

/**
 * Broadcast for each ordinary scheduled occurrence crossed during forward
 * advancement.
 *
 * Occurrences are dispatched chronologically. Multiple events at the same
 * timestamp preserve runtime schedule order. Teleporting and state restoration
 * do not replay ordinary event occurrences.
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnDiurnalTimeEventTriggered,
	const FDiurnalTimeEvent&,
	const FDiurnalDateTime& /* OccurrenceTime */);

/** Broadcast after an event has been added to the runtime schedule. */
DECLARE_MULTICAST_DELEGATE_OneParam(
	FOnDiurnalTimeEventAdded,
	const FDiurnalTimeEvent&);

/** Broadcast after an event has been removed from the runtime schedule. */
DECLARE_MULTICAST_DELEGATE_OneParam(
	FOnDiurnalTimeEventRemoved,
	const FDiurnalTimeEvent&);

/** Broadcast after a time range has been added to the runtime schedule. */
DECLARE_MULTICAST_DELEGATE_OneParam(
	FOnDiurnalTimeRangeAdded,
	const FDiurnalTimeRange&);

/** Broadcast after a time range has been removed from the runtime schedule. */
DECLARE_MULTICAST_DELEGATE_OneParam(
	FOnDiurnalTimeRangeRemoved,
	const FDiurnalTimeRange&);

/** Broadcast when the explicit pause state changes. */
DECLARE_MULTICAST_DELEGATE_OneParam(
	FOnDiurnalPauseStateChanged,
	bool /* bIsPaused */);

/** Broadcast when the clock-speed multiplier changes. */
DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnDiurnalTimeScaleChanged,
	double /* PreviousTimeScale */,
	double /* NewTimeScale */);

/**
 * Broadcast when a time range becomes active.
 *
 * Range transitions describe endpoint state only. Large jumps do not replay
 * ranges that were entered and exited entirely between the previous and final
 * clock positions.
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnDiurnalTimeRangeEntered,
	const FDiurnalTimeRange&,
	const FDiurnalDateTime& /* CurrentDateTime */);

/**
 * Broadcast when a previously active time range becomes inactive.
 *
 * Exits are emitted before entries during the same reconciliation.
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnDiurnalTimeRangeExited,
	const FDiurnalTimeRange&,
	const FDiurnalDateTime& /* CurrentDateTime */);

/**
 * Broadcast when a blocking event activates a time gate.
 *
 * Multiple gates may activate at the same timestamp. The complete active-gate
 * state has already been applied when listeners are notified.
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnDiurnalTimeGateActivated,
	const FDiurnalTimeEvent&,
	const FDiurnalDateTime& /* ActivationTime */);

/**
 * Broadcast when an active time gate is released.
 *
 * The clock remains blocked while any other active gate remains unresolved.
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnDiurnalTimeGateReleased,
	FGameplayTag /* GateTag */,
	const FDiurnalDateTime& /* ReleaseTime */);

#pragma endregion

/**
 * Game-instance-scoped simulation clock.
 *
 * The subsystem initializes from UDiurnalCycleSettings and survives ordinary
 * world travel with its owning game instance. It is not replicated.
 *
 * Automatic advancement runs on the game thread while the clock has a valid
 * world, is not explicitly paused, has a positive time scale, and is not
 * blocked by any active time gate.
 *
 * Ordinary time events represent occurrences during forward simulation.
 * Time ranges represent current temporal state and are reconciled at the final
 * clock position after jumps. Blocking events additionally create persistent
 * time-gate state when reached or when explicitly teleporting onto their exact
 * occurrence.
 *
 * Mutable runtime state can be captured and restored through CaptureState()
 * and TryRestoreState(). The consuming game remains responsible for save slots
 * and persistence.
 *
 * All public operations and delegate callbacks are game-thread-only.
 */
UCLASS()
class DIURNALCYCLERUNTIME_API UDiurnalCycleSubsystem final
	: public UGameInstanceSubsystem
	, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UDiurnalCycleSubsystem();

#pragma region UObjectAndSubsystem

	virtual void BeginDestroy() override;

	virtual void Initialize(
		FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

#pragma endregion

#pragma region ClockControl

	/** Pauses automatic clock advancement. */
	void Pause()
	{
		SetPaused(true);
	}

	/** Resumes automatic clock advancement. */
	void Resume()
	{
		SetPaused(false);
	}

	/** Sets whether automatic clock advancement is explicitly paused. */
	void SetPaused(bool bNewPaused);

	/**
	 * Changes the clock-speed multiplier.
	 *
	 * Zero prevents automatic advancement without changing the explicit pause
	 * state. Values outside the supported finite range are rejected.
	 *
	 * @return True when NewTimeScale was valid and accepted.
	 */
	bool TrySetTimeScale(
		double NewTimeScale);

	/**
	 * Teleports the clock to an absolute date and time.
	 *
	 * Ordinary event occurrences between the previous and destination times are
	 * not replayed. Time ranges are reconciled against the destination state.
	 *
	 * Existing active gates are replaced by every blocking event scheduled
	 * exactly at NewDateTime. This also applies when teleporting to the current
	 * timestamp, allowing an explicit teleport to re-evaluate gate state.
	 *
	 * @return True when NewDateTime was valid and applied.
	 */
	bool TrySetDateTime(
		const FDiurnalDateTime& NewDateTime);

	/**
	 * Advances the clock by a positive number of game hours.
	 *
	 * Ordinary events crossed in (previous time, actual final time] are emitted
	 * chronologically. When a blocking event is encountered, advancement clamps
	 * to its timestamp, every gate at that timestamp becomes active, and no
	 * later time is processed.
	 *
	 * @return True when GameHours was valid and advancement was accepted.
	 */
	bool TryAdvanceHours(
		double GameHours);

#pragma endregion

#pragma region ClockQueries

	/** Returns whether automatic advancement is explicitly paused. */
	bool IsPaused() const
	{
		return bPaused;
	}

	/** Returns the current clock-speed multiplier. */
	double GetTimeScale() const
	{
		return TimeScale;
	}

	/** Returns the current game day and exact time of day. */
	FDiurnalDateTime GetDateTime() const;

	/** Returns the current time-of-day portion of the clock. */
	FDiurnalTimeOfDay GetTimeOfDay() const
	{
		return GetDateTime().GetTimeOfDay();
	}

	/** Returns the current one-based game day. */
	int32 GetCurrentDay() const;

	/** Returns the current fractional hour in the range [0, 24). */
	double GetTimeOfDayHours() const;

	/** Returns normalized progress through the current day in [0, 1). */
	double GetDayProgress() const;

	/** Returns elapsed game hours since Day 1 at 00:00. */
	double GetTotalGameHours() const
	{
		return TotalGameHours;
	}

	/**
	 * Returns the configured real seconds required for one game hour at a time
	 * scale of one.
	 */
	double GetRealSecondsPerGameHour() const
	{
		return RealSecondsPerGameHour;
	}

#pragma endregion

#pragma region EventSchedule

	/**
	 * Returns the validated runtime event schedule.
	 *
	 * Events are ordered chronologically by time of day. Equal timestamps
	 * preserve schedule order. The returned view remains valid only until the
	 * schedule is mutated.
	 */
	TConstArrayView<FDiurnalTimeEvent> GetTimeEvents() const
	{
		return TimeEvents;
	}

	/**
	 * Finds the event identified by EventTag.
	 *
	 * @return True when a matching runtime event exists.
	 */
	bool TryGetTimeEvent(
		FGameplayTag EventTag,
		FDiurnalTimeEvent& OutTimeEvent) const;

	/** Returns whether the runtime event schedule contains EventTag. */
	bool HasTimeEvent(
		FGameplayTag EventTag) const;

	/**
	 * Adds a validated event to the runtime schedule.
	 *
	 * Event tags must be valid and unique. Ordinary events are never fired
	 * retrospectively when added. A blocking event added exactly at the current
	 * clock timestamp immediately becomes an active gate.
	 *
	 * @return True when TimeEvent was valid and added.
	 */
	bool TryAddTimeEvent(
		const FDiurnalTimeEvent& TimeEvent);

	/**
	 * Removes the event identified by EventTag.
	 *
	 * Removing an active blocking event also releases its gate.
	 *
	 * @return True when a matching event was found and removed.
	 */
	bool RemoveTimeEvent(
		FGameplayTag EventTag);

	/**
	 * Finds the next scheduled event occurrence strictly after the current time.
	 *
	 * Daily events resolve to their next daily occurrence. Dated events are
	 * considered only while their configured occurrence remains in the future.
	 * Blocking and ordinary events are queried uniformly.
	 *
	 * @return True when a future occurrence exists.
	 */
	bool TryGetNextTimeEvent(
		FDiurnalTimeEvent& OutEvent,
		FDiurnalDateTime& OutOccurrenceTime) const;

	/**
	 * Finds the next occurrence of the event identified by EventTag strictly
	 * after the current time.
	 *
	 * @return True when EventTag exists and has a future occurrence.
	 */
	bool TryGetNextOccurrence(
		FGameplayTag EventTag,
		FDiurnalDateTime& OutOccurrenceTime) const;

#pragma endregion

#pragma region TimeRangeSchedule

	/**
	 * Returns the validated runtime time-range schedule.
	 *
	 * Configuration/runtime insertion order is preserved. The returned view
	 * remains valid only until the schedule is mutated.
	 */
	TConstArrayView<FDiurnalTimeRange> GetTimeRanges() const
	{
		return TimeRanges;
	}

	/**
	 * Finds the range identified by RangeTag.
	 *
	 * @return True when a matching runtime range exists.
	 */
	bool TryGetTimeRange(
		FGameplayTag RangeTag,
		FDiurnalTimeRange& OutTimeRange) const;

	/** Returns whether the runtime time-range schedule contains RangeTag. */
	bool HasTimeRange(
		FGameplayTag RangeTag) const;

	/**
	 * Adds a validated range to the runtime schedule.
	 *
	 * Range tags must be valid and unique. If the range contains the current
	 * time, it becomes active immediately.
	 *
	 * @return True when TimeRange was valid and added.
	 */
	bool TryAddTimeRange(
		const FDiurnalTimeRange& TimeRange);

	/**
	 * Removes the range identified by RangeTag.
	 *
	 * Removing a currently active range emits its exit notification.
	 *
	 * @return True when a matching range was found and removed.
	 */
	bool RemoveTimeRange(
		FGameplayTag RangeTag);

	/** Returns whether TimeOfDay lies inside the range identified by RangeTag. */
	bool IsTimeOfDayInRange(
		FGameplayTag RangeTag,
		const FDiurnalTimeOfDay& TimeOfDay) const;

	/** Returns whether the current clock time lies inside RangeTag. */
	bool IsCurrentTimeInRange(
		FGameplayTag RangeTag) const;

	/**
	 * Returns the tags of every range active at the current clock time.
	 *
	 * Overlapping ranges are returned independently in runtime schedule order.
	 */
	TArray<FGameplayTag> GetActiveTimeRanges() const;

#pragma endregion

#pragma region TimeGates

	/** Returns whether one or more time gates currently block advancement. */
	bool IsBlockedByTimeGate() const
	{
		return !ActiveTimeGates.IsEmpty();
	}

	/**
	 * Returns every currently active time-gate tag.
	 *
	 * Multiple gates may be active simultaneously. The returned view remains
	 * valid only until gate state changes.
	 */
	TConstArrayView<FGameplayTag> GetActiveTimeGates() const
	{
		return ActiveTimeGates;
	}

	/** Returns whether GateTag is one of the currently active time gates. */
	bool IsTimeGateActive(
		FGameplayTag GateTag) const;

	/**
	 * Releases one active time gate.
	 *
	 * The clock remains blocked while any other active gate remains unresolved.
	 *
	 * @return True when GateTag was active and released.
	 */
	bool ReleaseTimeGate(
		FGameplayTag GateTag);

	/**
	 * Releases every currently active time gate.
	 *
	 * Each released gate emits its normal release notification.
	 *
	 * @return Number of gates released.
	 */
	int32 ReleaseAllTimeGates();

#pragma endregion

#pragma region Persistence

	/**
	 * Captures the mutable runtime clock state.
	 *
	 * The explicit pause state and configured base clock rate are intentionally
	 * not persisted.
	 */
	FDiurnalCycleState CaptureState() const;

	/**
	 * Atomically restores a previously captured runtime state.
	 *
	 * The complete state is validated before any current state is replaced.
	 * Ordinary scheduled occurrences are not replayed.
	 *
	 * Runtime ranges are reconciled against the restored clock position. Active
	 * gates are restored exactly from State.ActiveTimeGates rather than inferred
	 * solely from the timestamp, preserving whether a gate at that timestamp had
	 * already been released when the save was captured.
	 *
	 * @return True when the complete state was valid and restored.
	 */
	bool TryRestoreState(
		const FDiurnalCycleState& State);

#pragma endregion

#pragma region Notifications

	FOnDiurnalTimeChanged& OnTimeChanged()
	{
		return TimeChangedEvent;
	}

	FOnDiurnalTimeEventTriggered& OnTimeEventTriggered()
	{
		return TimeEventTriggeredEvent;
	}

	FOnDiurnalTimeEventAdded& OnTimeEventAdded()
	{
		return TimeEventAddedEvent;
	}

	FOnDiurnalTimeEventRemoved& OnTimeEventRemoved()
	{
		return TimeEventRemovedEvent;
	}

	FOnDiurnalPauseStateChanged& OnPauseStateChanged()
	{
		return PauseStateChangedEvent;
	}

	FOnDiurnalTimeScaleChanged& OnTimeScaleChanged()
	{
		return TimeScaleChangedEvent;
	}

	FOnDiurnalTimeRangeAdded& OnTimeRangeAdded()
	{
		return TimeRangeAddedEvent;
	}

	FOnDiurnalTimeRangeRemoved& OnTimeRangeRemoved()
	{
		return TimeRangeRemovedEvent;
	}

	FOnDiurnalTimeRangeEntered& OnTimeRangeEntered()
	{
		return TimeRangeEnteredEvent;
	}

	FOnDiurnalTimeRangeExited& OnTimeRangeExited()
	{
		return TimeRangeExitedEvent;
	}

	FOnDiurnalTimeGateActivated& OnTimeGateActivated()
	{
		return TimeGateActivatedEvent;
	}

	FOnDiurnalTimeGateReleased& OnTimeGateReleased()
	{
		return TimeGateReleasedEvent;
	}

#pragma endregion

#pragma region FTickableGameObject

	virtual void Tick(
		float DeltaTime) override;

	virtual bool IsTickable() const override;

	virtual TStatId GetStatId() const override;

	virtual UWorld* GetTickableGameObjectWorld() const override;

	virtual bool IsTickableInEditor() const override
	{
		return false;
	}

#pragma endregion

private:
#pragma region InitializationAndClock

	void ApplySettings();

	/** Returns whether the current world permits automatic advancement. */
	bool ShouldAdvanceInCurrentWorld() const;

	void AdvanceInternal(
		double GameHours,
		EDiurnalTimeChangeReason Reason);

	void BroadcastTimeChanged(
		double PreviousHours,
		double CurrentHours,
		EDiurnalTimeChangeReason Reason);

#pragma endregion

#pragma region EventProcessing

	void DispatchCrossedTimeEvents(
		double PreviousHours,
		double CurrentHours);

	bool TryGetNextOccurrenceHours(
		const FDiurnalTimeEvent& TimeEvent,
		double FromHours,
		double& OutOccurrenceHours) const;

	bool TryFindFirstBlockingOccurrence(
		double PreviousHours,
		double RequestedHours,
		double& OutOccurrenceHours) const;

	void SortTimeEvents();

#pragma endregion

#pragma region TimeRangeProcessing

	TArray<FDiurnalTimeRange>
	GetActiveTimeRangeDefinitions(
		const FDiurnalTimeOfDay& TimeOfDay) const;

	void BroadcastTimeRangeTransitions(
		const TArray<FDiurnalTimeRange>& PreviousActiveRanges,
		const TArray<FDiurnalTimeRange>& CurrentActiveRanges,
		const FDiurnalDateTime& CurrentDateTime);

#pragma endregion

#pragma region TimeGateProcessing

	/**
	 * Returns every blocking event tag scheduled exactly at DateTime.
	 *
	 * Returned order follows the runtime event schedule.
	 */
	TArray<FGameplayTag> GetScheduledTimeGatesAt(
		const FDiurnalDateTime& DateTime) const;

	/**
	 * Replaces the complete active-gate set and optionally emits the resulting
	 * release/activation transitions.
	 */
	void SetActiveTimeGates(
		const TArray<FGameplayTag>& NewActiveTimeGates,
		const FDiurnalDateTime& TransitionTime,
		bool bBroadcastTransitions);

#pragma endregion

#pragma region RuntimeState

	/** Elapsed game hours since Day 1 at 00:00. */
	double TotalGameHours = 0.0;

	/** Real seconds required for one game hour at a time scale of one. */
	double RealSecondsPerGameHour =
		DiurnalCycle::GDefaultRealSecondsPerGameHour;

	/** Multiplier applied to automatic clock advancement. */
	double TimeScale =
		DiurnalCycle::GDefaultTimeScale;

	/** Whether automatic advancement is explicitly paused. */
	bool bPaused = false;

	/** Last whole game second reported through TimeChangedEvent. */
	int64 LastBroadcastGameSecond =
		INDEX_NONE;

	/** Validated runtime event schedule sorted by exact time of day. */
	TArray<FDiurnalTimeEvent> TimeEvents;

	/** Validated mutable runtime time-range schedule. */
	TArray<FDiurnalTimeRange> TimeRanges;

	/** Unique tags of every time gate currently blocking advancement. */
	TArray<FGameplayTag> ActiveTimeGates;

#pragma endregion

#pragma region NativeDelegateStorage

	FOnDiurnalTimeChanged TimeChangedEvent;
	FOnDiurnalTimeEventTriggered TimeEventTriggeredEvent;
	FOnDiurnalTimeEventAdded TimeEventAddedEvent;
	FOnDiurnalTimeEventRemoved TimeEventRemovedEvent;

	FOnDiurnalPauseStateChanged PauseStateChangedEvent;
	FOnDiurnalTimeScaleChanged TimeScaleChangedEvent;

	FOnDiurnalTimeRangeAdded TimeRangeAddedEvent;
	FOnDiurnalTimeRangeRemoved TimeRangeRemovedEvent;
	FOnDiurnalTimeRangeEntered TimeRangeEnteredEvent;
	FOnDiurnalTimeRangeExited TimeRangeExitedEvent;

	FOnDiurnalTimeGateActivated TimeGateActivatedEvent;
	FOnDiurnalTimeGateReleased TimeGateReleasedEvent;

#pragma endregion
};