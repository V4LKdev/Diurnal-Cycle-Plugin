#include "Subsystem/DiurnalCycleSubsystem.h"

#include "DiurnalCycleLog.h"
#include "DiurnalSchedule.h"

namespace
{
	bool IsKnownDisabledEventReference(
		const FDiurnalScheduleEntryReference& Reference,
		const TArray<FDiurnalResolvedTimeEvent>& ActiveEntries)
	{
		if (const FDiurnalResolvedTimeEvent* Active = ActiveEntries.FindByPredicate(
			[&Reference](const FDiurnalResolvedTimeEvent& Entry)
			{
				return Entry.GetEntryReference() == Reference;
			}))
		{
			return !Active->bRuntimeAdded;
		}
		UDiurnalSchedule* Schedule = Reference.Schedule.LoadSynchronous();
		return IsValid(Schedule)
			&& Schedule->TimeEvents.ContainsByPredicate(
				[&Reference](const FDiurnalTimeEvent& Event)
				{
					return Event.EntryId == Reference.EntryId;
				});
	}

	bool IsKnownDisabledRangeReference(
		const FDiurnalScheduleEntryReference& Reference,
		const TArray<FDiurnalResolvedTimeRange>& ActiveEntries)
	{
		if (const FDiurnalResolvedTimeRange* Active = ActiveEntries.FindByPredicate(
			[&Reference](const FDiurnalResolvedTimeRange& Entry)
			{
				return Entry.GetEntryReference() == Reference;
			}))
		{
			return !Active->bRuntimeAdded;
		}
		UDiurnalSchedule* Schedule = Reference.Schedule.LoadSynchronous();
		return IsValid(Schedule)
			&& Schedule->TimeRanges.ContainsByPredicate(
				[&Reference](const FDiurnalTimeRange& Range)
				{
					return Range.EntryId == Reference.EntryId;
				});
	}

	bool ValidateGateOccurrences(
		const TArray<FDiurnalEventOccurrenceHandle>& Gates,
		const TArray<FDiurnalResolvedTimeEvent>& Events,
		const double Hours)
	{
		if (!DiurnalCycle::IsValidTotalGameHours(Hours))
		{
			return false;
		}
		const FDiurnalDateTime DateTime = FDiurnalDateTime::FromTotalHours(Hours);
		if (!FMath::IsNearlyEqual(Hours, DateTime.ToTotalHours(), 1.0e-9)
			&& !Gates.IsEmpty())
		{
			return false;
		}

		TSet<FGuid> SeenOccurrences;
		TSet<FDiurnalScheduleEntryReference> SeenEntries;
		for (const FDiurnalEventOccurrenceHandle& Gate : Gates)
		{
			if (!Gate.IsValid()
				|| Gate.OccurrenceTime != DateTime
				|| SeenOccurrences.Contains(Gate.OccurrenceId)
				|| SeenEntries.Contains(Gate.Entry))
			{
				return false;
			}
			const FDiurnalResolvedTimeEvent* Match = Events.FindByPredicate(
				[&Gate](const FDiurnalResolvedTimeEvent& Value)
				{
					return Value.GetEntryReference() == Gate.Entry;
				});
			if (!Match
				|| !Match->Event.IsBlocking()
				|| !Match->Event.OccursOnDay(DateTime.Day)
				|| Match->Event.TimeOfDay != DateTime.GetTimeOfDay())
			{
				return false;
			}
			SeenOccurrences.Add(Gate.OccurrenceId);
			SeenEntries.Add(Gate.Entry);
		}
		return true;
	}
}

bool UDiurnalCycleSubsystem::PrepareScheduleStateForRestore(
	const FDiurnalScheduleRuntimeState& State,
	const double TargetHours,
	TArray<FDiurnalScheduleEntryReference>& OutDisabledEvents,
	TArray<FDiurnalScheduleEntryReference>& OutDisabledRanges,
	TArray<FDiurnalEventOccurrenceHandle>& OutActiveGates,
	TArray<FDiurnalResolvedTimeEvent>& OutEvents,
	TArray<FDiurnalResolvedTimeRange>& OutRanges,
	TArray<TObjectPtr<UDiurnalSchedule>>& OutLoadedSchedules) const
{
	if (State.Version != DiurnalCycle::GCurrentScheduleStateVersion)
	{
		return false;
	}

	TArray<FDiurnalResolvedTimeEvent> AllEvents;
	TArray<FDiurnalResolvedTimeRange> AllRanges;
	TArray<TObjectPtr<UDiurnalSchedule>> AllLoaded;
	if (!CompileScheduleLayers(
			State.ActiveSchedules,
			State.RuntimeTimeEvents,
			State.RuntimeTimeRanges,
			{},
			{},
			AllEvents,
			AllRanges,
			AllLoaded))
	{
		return false;
	}

	OutDisabledEvents.Reset();
	OutDisabledRanges.Reset();
	TSet<FDiurnalScheduleEntryReference> Seen;
	for (const FDiurnalScheduleEntryReference& Reference : State.DisabledEventEntries)
	{
		if (!Reference.IsValid()
			|| Seen.Contains(Reference)
			|| !IsKnownDisabledEventReference(Reference, AllEvents))
		{
			return false;
		}
		Seen.Add(Reference);
		OutDisabledEvents.Add(Reference);
	}
	Seen.Reset();
	for (const FDiurnalScheduleEntryReference& Reference : State.DisabledRangeEntries)
	{
		if (!Reference.IsValid()
			|| Seen.Contains(Reference)
			|| !IsKnownDisabledRangeReference(Reference, AllRanges))
		{
			return false;
		}
		Seen.Add(Reference);
		OutDisabledRanges.Add(Reference);
	}

	if (!CompileScheduleLayers(
			State.ActiveSchedules,
			State.RuntimeTimeEvents,
			State.RuntimeTimeRanges,
			OutDisabledEvents,
			OutDisabledRanges,
			OutEvents,
			OutRanges,
			OutLoadedSchedules))
	{
		return false;
	}

	OutActiveGates.Reset();
	OutActiveGates = State.ActiveTimeGateOccurrences;

	return ValidateGateOccurrences(OutActiveGates, OutEvents, TargetHours);
}

FDiurnalClockState UDiurnalCycleSubsystem::CaptureClockState() const
{
	FDiurnalClockState State;
	State.TotalGameHours = TotalGameHours;
	State.TimeScale = TimeScale;
	State.RealSecondsPerGameHour = RealSecondsPerGameHour;
	State.bPaused = bPaused;
	return State;
}

FDiurnalScheduleRuntimeState UDiurnalCycleSubsystem::CaptureScheduleState() const
{
	FDiurnalScheduleRuntimeState State;
	State.Version = DiurnalCycle::GCurrentScheduleStateVersion;
	State.ActiveSchedules = ActiveScheduleReferences;
	State.RuntimeTimeEvents = RuntimeTimeEvents;
	State.RuntimeTimeRanges = RuntimeTimeRanges;
	State.DisabledEventEntries = DisabledEventEntries;
	State.DisabledRangeEntries = DisabledRangeEntries;
	State.ActiveTimeGateOccurrences = ActiveTimeGateOccurrences;
	return State;
}

FDiurnalCycleState UDiurnalCycleSubsystem::CaptureState() const
{
	FDiurnalCycleState State;
	State.Version = DiurnalCycle::GCurrentStateVersion;
	State.ClockState = CaptureClockState();
	State.ScheduleState = CaptureScheduleState();
	return State;
}

bool UDiurnalCycleSubsystem::TryRestoreClockState(const FDiurnalClockState& State)
{
	if (State.Version != DiurnalCycle::GCurrentClockStateVersion
		|| !DiurnalCycle::IsValidTotalGameHours(State.TotalGameHours)
		|| !DiurnalCycle::IsValidTimeScale(State.TimeScale)
		|| !DiurnalCycle::IsValidRealSecondsPerGameHour(State.RealSecondsPerGameHour))
	{
		UE_LOG(LogDiurnalCycle, Warning, TEXT("Rejected invalid day/night clock state."));
		return false;
	}
	if (!ValidateGateOccurrences(ActiveTimeGateOccurrences, ResolvedTimeEvents, State.TotalGameHours))
	{
		UE_LOG(LogDiurnalCycle, Warning, TEXT("Rejected clock state because active schedule gates are incompatible with its timestamp."));
		return false;
	}

	const double PreviousHours = TotalGameHours;
	const double PreviousScale = TimeScale;
	const bool bPreviousPaused = bPaused;
	const TArray<FDiurnalResolvedTimeRange> PreviousRanges = GetActiveTimeRangeDefinitions(GetDateTime());
	TotalGameHours = State.TotalGameHours;
	TimeScale = State.TimeScale;
	RealSecondsPerGameHour = State.RealSecondsPerGameHour;
	bPaused = State.bPaused;
	LastBroadcastGameSecond = static_cast<int64>(FMath::Floor(TotalGameHours * DiurnalCycle::GSecondsPerHour));
	if (PreviousScale != TimeScale) TimeScaleChangedEvent.Broadcast(PreviousScale, TimeScale);
	if (bPreviousPaused != bPaused) PauseStateChangedEvent.Broadcast(bPaused);
	BroadcastTimeRangeTransitions(PreviousRanges, GetActiveTimeRangeDefinitions(GetDateTime()), GetDateTime());
	BroadcastTimeChanged(PreviousHours, TotalGameHours, EDiurnalTimeChangeReason::StateRestored);
	return true;
}

bool UDiurnalCycleSubsystem::TryRestoreScheduleState(const FDiurnalScheduleRuntimeState& State)
{
	TArray<FDiurnalScheduleEntryReference> NewDisabledEvents;
	TArray<FDiurnalScheduleEntryReference> NewDisabledRanges;
	TArray<FDiurnalEventOccurrenceHandle> NewActiveGates;
	TArray<FDiurnalResolvedTimeEvent> Events;
	TArray<FDiurnalResolvedTimeRange> Ranges;
	TArray<TObjectPtr<UDiurnalSchedule>> Loaded;
	if (!PrepareScheduleStateForRestore(State, TotalGameHours, NewDisabledEvents, NewDisabledRanges, NewActiveGates, Events, Ranges, Loaded))
	{
		UE_LOG(LogDiurnalCycle, Warning, TEXT("Rejected invalid schedule runtime state (schema version %d)."), State.Version);
		return false;
	}

	const TArray<FDiurnalResolvedTimeRange> PreviousRanges = GetActiveTimeRangeDefinitions(GetDateTime());
	ActiveScheduleReferences = State.ActiveSchedules;
	RuntimeTimeEvents.Reset();
	for (const FDiurnalResolvedTimeEvent& Event : Events)
	{
		if (Event.bRuntimeAdded) RuntimeTimeEvents.Add(Event.Event);
	}
	RuntimeTimeRanges.Reset();
	for (const FDiurnalResolvedTimeRange& Range : Ranges)
	{
		if (Range.bRuntimeAdded) RuntimeTimeRanges.Add(Range.Range);
	}
	DisabledEventEntries = MoveTemp(NewDisabledEvents);
	DisabledRangeEntries = MoveTemp(NewDisabledRanges);
	ApplyCompiledSchedule(MoveTemp(Events), MoveTemp(Ranges), MoveTemp(Loaded), false);
	SetActiveTimeGates(NewActiveGates, GetDateTime(), true);
	BroadcastTimeRangeTransitions(PreviousRanges, GetActiveTimeRangeDefinitions(GetDateTime()), GetDateTime());
	return true;
}

bool UDiurnalCycleSubsystem::TryRestoreState(const FDiurnalCycleState& State)
{
	if (State.Version != DiurnalCycle::GCurrentStateVersion)
	{
		UE_LOG(LogDiurnalCycle, Warning, TEXT("Rejected diurnal state version %d."), State.Version);
		return false;
	}
	const FDiurnalClockState& Clock = State.ClockState;
	const FDiurnalScheduleRuntimeState& Schedule = State.ScheduleState;

	if (Clock.Version != DiurnalCycle::GCurrentClockStateVersion
		|| !DiurnalCycle::IsValidTotalGameHours(Clock.TotalGameHours)
		|| !DiurnalCycle::IsValidTimeScale(Clock.TimeScale)
		|| !DiurnalCycle::IsValidRealSecondsPerGameHour(Clock.RealSecondsPerGameHour))
	{
		return false;
	}

	TArray<FDiurnalScheduleEntryReference> NewDisabledEvents;
	TArray<FDiurnalScheduleEntryReference> NewDisabledRanges;
	TArray<FDiurnalEventOccurrenceHandle> NewActiveGates;
	TArray<FDiurnalResolvedTimeEvent> Events;
	TArray<FDiurnalResolvedTimeRange> Ranges;
	TArray<TObjectPtr<UDiurnalSchedule>> Loaded;
	if (!PrepareScheduleStateForRestore(Schedule, Clock.TotalGameHours, NewDisabledEvents, NewDisabledRanges, NewActiveGates, Events, Ranges, Loaded))
	{
		return false;
	}

	const double PreviousHours = TotalGameHours;
	const double PreviousScale = TimeScale;
	const bool bPreviousPaused = bPaused;
	const TArray<FDiurnalResolvedTimeRange> PreviousRanges = GetActiveTimeRangeDefinitions(GetDateTime());
	ActiveScheduleReferences = Schedule.ActiveSchedules;
	RuntimeTimeEvents.Reset();
	for (const FDiurnalResolvedTimeEvent& Event : Events)
	{
		if (Event.bRuntimeAdded) RuntimeTimeEvents.Add(Event.Event);
	}
	RuntimeTimeRanges.Reset();
	for (const FDiurnalResolvedTimeRange& Range : Ranges)
	{
		if (Range.bRuntimeAdded) RuntimeTimeRanges.Add(Range.Range);
	}
	DisabledEventEntries = MoveTemp(NewDisabledEvents);
	DisabledRangeEntries = MoveTemp(NewDisabledRanges);
	ApplyCompiledSchedule(MoveTemp(Events), MoveTemp(Ranges), MoveTemp(Loaded), false);
	TotalGameHours = Clock.TotalGameHours;
	TimeScale = Clock.TimeScale;
	RealSecondsPerGameHour = Clock.RealSecondsPerGameHour;
	bPaused = Clock.bPaused;
	LastBroadcastGameSecond = static_cast<int64>(FMath::Floor(TotalGameHours * DiurnalCycle::GSecondsPerHour));
	SetActiveTimeGates(NewActiveGates, GetDateTime(), true);
	if (PreviousScale != TimeScale) TimeScaleChangedEvent.Broadcast(PreviousScale, TimeScale);
	if (bPreviousPaused != bPaused) PauseStateChangedEvent.Broadcast(bPaused);
	BroadcastTimeRangeTransitions(PreviousRanges, GetActiveTimeRangeDefinitions(GetDateTime()), GetDateTime());
	BroadcastTimeChanged(PreviousHours, TotalGameHours, EDiurnalTimeChangeReason::StateRestored);
	return true;
}
