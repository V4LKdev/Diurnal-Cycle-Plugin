#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"

#include "DiurnalCycleTypes.h"

#include "DiurnalCycleSubsystem.generated.h"

class UWorld;
class UDiurnalSchedule;

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

/** Identity-rich counterpart used when one exact occurrence matters. */
DECLARE_MULTICAST_DELEGATE_ThreeParams(
	FOnDiurnalTimeEventOccurrence,
	const FDiurnalEventOccurrenceHandle&,
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

/** Identity-rich time-range entry transition. */
DECLARE_MULTICAST_DELEGATE_ThreeParams(
	FOnDiurnalTimeRangeEntryEntered,
	const FDiurnalScheduleEntryReference&,
	const FDiurnalTimeRange&,
	const FDiurnalDateTime& /* CurrentDateTime */);

/** Identity-rich time-range exit transition. */
DECLARE_MULTICAST_DELEGATE_ThreeParams(
	FOnDiurnalTimeRangeEntryExited,
	const FDiurnalScheduleEntryReference&,
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

/** Exact blocking occurrence activation. */
DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnDiurnalTimeGateOccurrenceActivated,
	const FDiurnalEventOccurrenceHandle&,
	const FDiurnalTimeEvent&);

/** Exact blocking occurrence release. */
DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnDiurnalTimeGateOccurrenceReleased,
	const FDiurnalEventOccurrenceHandle&,
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
	 * later time is processed. An advance beyond the maximum representable date
	 * is rejected, pauses the clock, and emits one diagnostic warning until the
	 * clock is explicitly resumed or moved.
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

#pragma region ScheduleLayers

	/** Atomically replaces the ordered authored schedule layers. May synchronously load uncached assets. */
	bool TrySetActiveSchedules(
		const TArray<TSoftObjectPtr<UDiurnalSchedule>>& NewSchedules);

	/** Adds one authored layer after the currently active layers. */
	bool TryActivateSchedule(UDiurnalSchedule* Schedule);

	/** Removes one authored layer without changing clock time. */
	bool DeactivateSchedule(UDiurnalSchedule* Schedule);

	TConstArrayView<TSoftObjectPtr<UDiurnalSchedule>> GetActiveSchedules() const
	{
		return ActiveScheduleReferences;
	}

	/** Human-readable automatic pause reason, empty for an explicit/user pause. */
	const FString& GetPauseReason() const { return PauseReason; }

	TConstArrayView<FDiurnalResolvedTimeEvent> GetResolvedTimeEvents() const
	{
		return ResolvedTimeEvents;
	}

	TConstArrayView<FDiurnalResolvedTimeRange> GetResolvedTimeRanges() const
	{
		return ResolvedTimeRanges;
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

	/** Returns whether any runtime event contains EventTag. */
	bool HasTimeEvent(
		FGameplayTag EventTag) const;

	/** Returns every event containing EventTag in runtime schedule order. */
	TArray<FDiurnalResolvedTimeEvent> FindTimeEventsByTag(
		FGameplayTag EventTag) const;

	/** Returns every event whose optional tags satisfy TagQuery. */
	TArray<FDiurnalResolvedTimeEvent> FindTimeEventsByTagQuery(
		const FGameplayTagQuery& TagQuery) const;

	/** Resolves one exact authored/runtime event reference. */
	bool TryGetTimeEvent(
		const FDiurnalScheduleEntryReference& Reference,
		FDiurnalResolvedTimeEvent& OutTimeEvent) const;

	/**
	 * Adds a validated event to the runtime schedule.
	 *
	 * Tags are optional and may be shared. Ordinary events are never fired
	 * retrospectively when added. A blocking event added exactly at the current
	 * clock timestamp immediately becomes an active gate.
	 *
	 * @return True when TimeEvent was valid and added.
	 */
	bool TryAddTimeEvent(
		const FDiurnalTimeEvent& TimeEvent);

	/** Adds an event and returns its exact runtime reference. */
	bool TryAddTimeEvent(
		const FDiurnalTimeEvent& TimeEvent,
		FDiurnalScheduleEntryReference& OutReference);

	/** Removes exactly one event by stable reference. */
	bool RemoveTimeEvent(
		const FDiurnalScheduleEntryReference& Reference);

	/** Removes every event containing EventTag and returns the removed count. */
	int32 RemoveTimeEventsByTag(FGameplayTag EventTag);

	/** Re-enables exactly one authored event. */
	bool ReenableTimeEvent(const FDiurnalScheduleEntryReference& Reference);

	/** Re-enables every disabled event containing EventTag. */
	int32 ReenableTimeEventsByTag(FGameplayTag EventTag);

	/**
	 * Finds the next scheduled event occurrence strictly after the current time.
	 *
	 * Repeating events resolve to their next recurrence. One-off events are
	 * considered only while their configured occurrence remains in the future.
	 * Blocking and ordinary events are queried uniformly.
	 *
	 * @return True when a future occurrence exists.
	 */
	bool TryGetNextTimeEvent(
		FDiurnalTimeEvent& OutEvent,
		FDiurnalDateTime& OutOccurrenceTime) const;

	/**
	 * Finds the earliest next occurrence among all events containing EventTag,
	 * strictly after the current time.
	 *
	 * @return True when EventTag exists and has a future occurrence.
	 */
	bool TryGetNextOccurrence(
		FGameplayTag EventTag,
		FDiurnalDateTime& OutOccurrenceTime) const;

	/** Finds the next occurrence of one exact event reference. */
	bool TryGetNextOccurrence(
		const FDiurnalScheduleEntryReference& Reference,
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

	/** Returns whether the runtime time-range schedule contains RangeTag. */
	bool HasTimeRange(
		FGameplayTag RangeTag) const;

	TArray<FDiurnalResolvedTimeRange> FindTimeRangesByTag(
		FGameplayTag RangeTag) const;

	TArray<FDiurnalResolvedTimeRange> FindTimeRangesByTagQuery(
		const FGameplayTagQuery& TagQuery) const;

	bool TryGetTimeRange(
		const FDiurnalScheduleEntryReference& Reference,
		FDiurnalResolvedTimeRange& OutTimeRange) const;

	/**
	 * Adds a validated range to the runtime schedule.
	 *
	 * Tags are optional and may be shared. If the range contains the current
	 * time, it becomes active immediately.
	 *
	 * @return True when TimeRange was valid and added.
	 */
	bool TryAddTimeRange(
		const FDiurnalTimeRange& TimeRange);

	bool TryAddTimeRange(
		const FDiurnalTimeRange& TimeRange,
		FDiurnalScheduleEntryReference& OutReference);

	bool RemoveTimeRange(const FDiurnalScheduleEntryReference& Reference);

	int32 RemoveTimeRangesByTag(FGameplayTag RangeTag);

	bool ReenableTimeRange(const FDiurnalScheduleEntryReference& Reference);

	int32 ReenableTimeRangesByTag(FGameplayTag RangeTag);

	/** Returns whether TimeOfDay lies inside any range containing RangeTag. */
	bool IsTimeOfDayInRange(
		FGameplayTag RangeTag,
		const FDiurnalTimeOfDay& TimeOfDay) const;

	/** Returns whether the current clock time lies inside RangeTag. */
	bool IsCurrentTimeInRange(
		FGameplayTag RangeTag) const;

	/**
	 * Returns the unique aggregate tags contributed by active ranges.
	 * Exact contributions remain available through GetActiveTimeRangeEntries().
	 */
	TArray<FGameplayTag> GetActiveTimeRangeTags() const;

	/** Exact references for every currently active range contribution. */
	TArray<FDiurnalScheduleEntryReference> GetActiveTimeRangeEntries() const;

#pragma endregion

#pragma region TimeGates

	/** Returns whether one or more time gates currently block advancement. */
	bool IsBlockedByTimeGate() const
	{
		return !ActiveTimeGateOccurrences.IsEmpty();
	}

	/**
	 * Returns the unique aggregate tags contributed by active gate occurrences.
	 * Tagless gates still block and are visible through exact occurrence handles.
	 */
	TArray<FGameplayTag> GetActiveTimeGateTags() const;

	/** Exact blocking occurrences currently holding the clock. */
	TConstArrayView<FDiurnalEventOccurrenceHandle> GetActiveTimeGateOccurrences() const
	{
		return ActiveTimeGateOccurrences;
	}

	/** Returns whether GateTag is one of the currently active time gates. */
	bool IsTimeGateActive(
		FGameplayTag GateTag) const;

	bool IsTimeGateOccurrenceActive(
		const FDiurnalEventOccurrenceHandle& Occurrence) const;

	/** Releases one exact blocking occurrence. */
	bool ReleaseTimeGate(const FDiurnalEventOccurrenceHandle& Occurrence);

	/** Releases every active gate whose event contains GateTag. */
	int32 ReleaseTimeGatesByTag(FGameplayTag GateTag);

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
	 * This aggregate includes the current clock, explicit pause state, configured
	 * base clock rate, authored schedule layers, runtime overlays, disabled-entry
	 * overlays, and active exact gate occurrences.
	 */
	FDiurnalCycleState CaptureState() const;

	/** Captures time, scale, base clock rate, and pause state. */
	FDiurnalClockState CaptureClockState() const;

	FDiurnalScheduleRuntimeState CaptureScheduleState() const;

	/**
	 * Atomically restores a previously captured runtime state.
	 *
	 * The complete state is validated before any current state is replaced.
	 * Ordinary scheduled occurrences are not replayed.
	 *
	 * Runtime ranges are reconciled against the restored clock position. Active
	 * gates are restored exactly from State.ActiveTimeGateOccurrences rather than inferred
	 * solely from the timestamp, preserving whether a gate at that timestamp had
	 * already been released when the save was captured.
	 *
	 * @return True when the complete state was valid and restored.
	 */
	bool TryRestoreState(
		const FDiurnalCycleState& State);

	bool TryRestoreClockState(const FDiurnalClockState& State);

	bool TryRestoreScheduleState(const FDiurnalScheduleRuntimeState& State);

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

	FOnDiurnalTimeEventOccurrence& OnTimeEventOccurrence()
	{
		return TimeEventOccurrenceEvent;
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

	FOnDiurnalTimeRangeEntryEntered& OnTimeRangeEntryEntered()
	{
		return TimeRangeEntryEnteredEvent;
	}

	FOnDiurnalTimeRangeEntryExited& OnTimeRangeEntryExited()
	{
		return TimeRangeEntryExitedEvent;
	}

	FOnDiurnalTimeGateActivated& OnTimeGateActivated()
	{
		return TimeGateActivatedEvent;
	}

	FOnDiurnalTimeGateOccurrenceActivated& OnTimeGateOccurrenceActivated()
	{
		return TimeGateOccurrenceActivatedEvent;
	}

	FOnDiurnalTimeGateOccurrenceReleased& OnTimeGateOccurrenceReleased()
	{
		return TimeGateOccurrenceReleasedEvent;
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

	bool CompileScheduleLayers(
		const TArray<TSoftObjectPtr<UDiurnalSchedule>>& ScheduleReferences,
		const TArray<FDiurnalTimeEvent>& InRuntimeEvents,
		const TArray<FDiurnalTimeRange>& InRuntimeRanges,
		const TArray<FDiurnalScheduleEntryReference>& InDisabledEvents,
		const TArray<FDiurnalScheduleEntryReference>& InDisabledRanges,
		TArray<FDiurnalResolvedTimeEvent>& OutEvents,
		TArray<FDiurnalResolvedTimeRange>& OutRanges,
		TArray<TObjectPtr<UDiurnalSchedule>>& OutLoadedSchedules) const;

	void ApplyCompiledSchedule(
		TArray<FDiurnalResolvedTimeEvent>&& NewEvents,
		TArray<FDiurnalResolvedTimeRange>&& NewRanges,
		TArray<TObjectPtr<UDiurnalSchedule>>&& NewLoadedSchedules,
		bool bBroadcastTransitions);

	bool RebuildCompiledSchedule(bool bBroadcastTransitions);

	bool PrepareScheduleStateForRestore(
		const FDiurnalScheduleRuntimeState& State,
		double TargetHours,
		TArray<FDiurnalScheduleEntryReference>& OutDisabledEvents,
		TArray<FDiurnalScheduleEntryReference>& OutDisabledRanges,
		TArray<FDiurnalEventOccurrenceHandle>& OutActiveGates,
		TArray<FDiurnalResolvedTimeEvent>& OutEvents,
		TArray<FDiurnalResolvedTimeRange>& OutRanges,
		TArray<TObjectPtr<UDiurnalSchedule>>& OutLoadedSchedules) const;

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

	TArray<FDiurnalResolvedTimeRange>
	GetActiveTimeRangeDefinitions(
		const FDiurnalDateTime& DateTime) const;

	void BroadcastTimeRangeTransitions(
		const TArray<FDiurnalResolvedTimeRange>& PreviousActiveRanges,
		const TArray<FDiurnalResolvedTimeRange>& CurrentActiveRanges,
		const FDiurnalDateTime& CurrentDateTime);

#pragma endregion

#pragma region TimeGateProcessing

	/**
	 * Returns exact blocking occurrence handles scheduled at DateTime.
	 * Returned order follows the runtime event schedule.
	 */
	TArray<FDiurnalEventOccurrenceHandle> GetScheduledTimeGatesAt(
		const FDiurnalDateTime& DateTime) const;

	/**
	 * Replaces the complete active-gate set and optionally emits the resulting
	 * release/activation transitions.
	 */
	void SetActiveTimeGates(
		const TArray<FDiurnalEventOccurrenceHandle>& NewActiveTimeGates,
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

	/** Non-empty when runtime safety paused the clock. */
	FString PauseReason;
	bool bMaximumDateWarningEmitted = false;

	/** Last whole game second reported through TimeChangedEvent. */
	int64 LastBroadcastGameSecond =
		INDEX_NONE;

	/** Validated runtime event schedule sorted by exact time of day. */
	TArray<FDiurnalTimeEvent> TimeEvents;

	/** Validated mutable runtime time-range schedule. */
	TArray<FDiurnalTimeRange> TimeRanges;

	/** Exact event occurrences currently blocking advancement. */
	TArray<FDiurnalEventOccurrenceHandle> ActiveTimeGateOccurrences;

	/** Ordered identities and strong references for immutable authored layers. */
	TArray<TSoftObjectPtr<UDiurnalSchedule>> ActiveScheduleReferences;
	TArray<TObjectPtr<UDiurnalSchedule>> ActiveScheduleAssets;

	/** Mutable overlay, kept separate from authored schedule assets. */
	TArray<FDiurnalTimeEvent> RuntimeTimeEvents;
	TArray<FDiurnalTimeRange> RuntimeTimeRanges;
	TArray<FDiurnalScheduleEntryReference> DisabledEventEntries;
	TArray<FDiurnalScheduleEntryReference> DisabledRangeEntries;

	/** Provenance-preserving compiled schedule corresponding to flat arrays. */
	TArray<FDiurnalResolvedTimeEvent> ResolvedTimeEvents;
	TArray<FDiurnalResolvedTimeRange> ResolvedTimeRanges;

#pragma endregion

#pragma region NativeDelegateStorage

	FOnDiurnalTimeChanged TimeChangedEvent;
	FOnDiurnalTimeEventTriggered TimeEventTriggeredEvent;
	FOnDiurnalTimeEventOccurrence TimeEventOccurrenceEvent;
	FOnDiurnalTimeEventAdded TimeEventAddedEvent;
	FOnDiurnalTimeEventRemoved TimeEventRemovedEvent;

	FOnDiurnalPauseStateChanged PauseStateChangedEvent;
	FOnDiurnalTimeScaleChanged TimeScaleChangedEvent;

	FOnDiurnalTimeRangeAdded TimeRangeAddedEvent;
	FOnDiurnalTimeRangeRemoved TimeRangeRemovedEvent;
	FOnDiurnalTimeRangeEntered TimeRangeEnteredEvent;
	FOnDiurnalTimeRangeExited TimeRangeExitedEvent;
	FOnDiurnalTimeRangeEntryEntered TimeRangeEntryEnteredEvent;
	FOnDiurnalTimeRangeEntryExited TimeRangeEntryExitedEvent;

	FOnDiurnalTimeGateActivated TimeGateActivatedEvent;
	FOnDiurnalTimeGateOccurrenceActivated TimeGateOccurrenceActivatedEvent;
	FOnDiurnalTimeGateOccurrenceReleased TimeGateOccurrenceReleasedEvent;

#pragma endregion
};
