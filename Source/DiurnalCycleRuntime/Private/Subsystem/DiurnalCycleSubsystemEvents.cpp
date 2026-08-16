#include "Subsystem/DiurnalCycleSubsystem.h"

#include "DiurnalCycleLog.h"

#pragma region EventSchedule

TArray<FDiurnalResolvedTimeEvent> UDiurnalCycleSubsystem::FindTimeEventsByTag(
	const FGameplayTag EventTag) const
{
	TArray<FDiurnalResolvedTimeEvent> Result;
	if (!EventTag.IsValid())
	{
		return Result;
	}

	for (const FDiurnalResolvedTimeEvent& Resolved : ResolvedTimeEvents)
	{
		if (Resolved.Event.HasTagExact(EventTag))
		{
			Result.Add(Resolved);
		}
	}
	return Result;
}

TArray<FDiurnalResolvedTimeEvent> UDiurnalCycleSubsystem::FindTimeEventsByTagQuery(
	const FGameplayTagQuery& TagQuery) const
{
	TArray<FDiurnalResolvedTimeEvent> Result;
	for (const FDiurnalResolvedTimeEvent& Resolved : ResolvedTimeEvents)
	{
		if (TagQuery.IsEmpty() || TagQuery.Matches(Resolved.Event.EventTags))
		{
			Result.Add(Resolved);
		}
	}
	return Result;
}

bool UDiurnalCycleSubsystem::TryGetTimeEvent(
	const FDiurnalScheduleEntryReference& Reference,
	FDiurnalResolvedTimeEvent& OutTimeEvent) const
{
	OutTimeEvent = {};
	if (!Reference.IsValid())
	{
		return false;
	}
	const FDiurnalResolvedTimeEvent* Match = ResolvedTimeEvents.FindByPredicate(
		[&Reference](const FDiurnalResolvedTimeEvent& Candidate)
		{
			return Candidate.GetEntryReference() == Reference;
		});
	if (!Match)
	{
		return false;
	}
	OutTimeEvent = *Match;
	return true;
}

bool UDiurnalCycleSubsystem::HasTimeEvent(const FGameplayTag EventTag) const
{
	return EventTag.IsValid()
		&& ResolvedTimeEvents.ContainsByPredicate(
			[EventTag](const FDiurnalResolvedTimeEvent& Resolved)
			{
				return Resolved.Event.HasTagExact(EventTag);
			});
}

bool UDiurnalCycleSubsystem::TryAddTimeEvent(const FDiurnalTimeEvent& TimeEvent)
{
	FDiurnalScheduleEntryReference IgnoredReference;
	return TryAddTimeEvent(TimeEvent, IgnoredReference);
}

bool UDiurnalCycleSubsystem::TryAddTimeEvent(
	const FDiurnalTimeEvent& TimeEvent,
	FDiurnalScheduleEntryReference& OutReference)
{
	OutReference = {};
	FDiurnalTimeEvent AddedEvent = TimeEvent;
	if (!AddedEvent.IsValid())
	{
		UE_LOG(
			LogDiurnalCycle,
			Warning,
			TEXT("Rejected invalid event '%s': Time=%s Recurrence=%d Anchor=%d Interval=%d."),
			*AddedEvent.GetDisplayName().ToString(),
			*AddedEvent.TimeOfDay.ToString(),
			static_cast<int32>(AddedEvent.Recurrence.Mode),
			AddedEvent.Recurrence.AnchorDay,
			AddedEvent.Recurrence.IntervalDays);
		return false;
	}

	if (AddedEvent.EventName.IsNone())
	{
		const FGameplayTag FirstTag = AddedEvent.GetPrimaryTag();
		AddedEvent.EventName = FirstTag.IsValid()
			? FirstTag.GetTagLeafName()
			: FName(TEXT("Runtime Event"));
	}

	const auto IdCollides = [this](const FGuid& EntryId)
	{
		return ResolvedTimeEvents.ContainsByPredicate(
			[EntryId](const FDiurnalResolvedTimeEvent& Existing)
			{
				return Existing.SourceSchedule.IsNull()
					&& Existing.Event.EntryId == EntryId;
			});
	};
	if (!AddedEvent.EntryId.IsValid() || IdCollides(AddedEvent.EntryId))
	{
		do
		{
			AddedEvent.EntryId = FGuid::NewGuid();
		}
		while (IdCollides(AddedEvent.EntryId));
	}

	RuntimeTimeEvents.Add(AddedEvent);
	if (!RebuildCompiledSchedule(true))
	{
		RuntimeTimeEvents.Pop();
		return false;
	}

	OutReference.EntryId = AddedEvent.EntryId;
	return true;
}

bool UDiurnalCycleSubsystem::RemoveTimeEvent(
	const FDiurnalScheduleEntryReference& Reference)
{
	FDiurnalResolvedTimeEvent Resolved;
	if (!TryGetTimeEvent(Reference, Resolved))
	{
		return false;
	}

	const int32 RuntimeIndex = Resolved.bRuntimeAdded
		? RuntimeTimeEvents.IndexOfByPredicate(
			[&Reference](const FDiurnalTimeEvent& Event)
			{
				return Event.EntryId == Reference.EntryId;
			})
		: INDEX_NONE;
	FDiurnalTimeEvent RemovedRuntimeEvent;
	if (RuntimeIndex != INDEX_NONE)
	{
		RemovedRuntimeEvent = RuntimeTimeEvents[RuntimeIndex];
		RuntimeTimeEvents.RemoveAt(RuntimeIndex);
	}
	else
	{
		DisabledEventEntries.AddUnique(Reference);
	}

	if (!RebuildCompiledSchedule(true))
	{
		if (RuntimeIndex != INDEX_NONE)
		{
			RuntimeTimeEvents.Insert(RemovedRuntimeEvent, RuntimeIndex);
		}
		else
		{
			DisabledEventEntries.Remove(Reference);
		}
		return false;
	}
	return true;
}

int32 UDiurnalCycleSubsystem::RemoveTimeEventsByTag(const FGameplayTag EventTag)
{
	const TArray<FDiurnalResolvedTimeEvent> Matches = FindTimeEventsByTag(EventTag);
	if (Matches.IsEmpty()) return 0;
	const TArray<FDiurnalTimeEvent> PreviousRuntimeEvents = RuntimeTimeEvents;
	const TArray<FDiurnalScheduleEntryReference> PreviousDisabledEvents = DisabledEventEntries;
	for (const FDiurnalResolvedTimeEvent& Match : Matches)
	{
		const FDiurnalScheduleEntryReference Reference = Match.GetEntryReference();
		if (Match.bRuntimeAdded)
		{
			RuntimeTimeEvents.RemoveAll([&Reference](const FDiurnalTimeEvent& Event)
			{
				return Event.EntryId == Reference.EntryId;
			});
		}
		else
		{
			DisabledEventEntries.AddUnique(Reference);
		}
	}
	if (!RebuildCompiledSchedule(true))
	{
		RuntimeTimeEvents = PreviousRuntimeEvents;
		DisabledEventEntries = PreviousDisabledEvents;
		return 0;
	}
	return Matches.Num();
}

bool UDiurnalCycleSubsystem::ReenableTimeEvent(
	const FDiurnalScheduleEntryReference& Reference)
{
	if (!Reference.IsValid() || DisabledEventEntries.Remove(Reference) == 0)
	{
		return false;
	}
	if (!RebuildCompiledSchedule(true))
	{
		DisabledEventEntries.AddUnique(Reference);
		return false;
	}
	return true;
}

int32 UDiurnalCycleSubsystem::ReenableTimeEventsByTag(const FGameplayTag EventTag)
{
	if (!EventTag.IsValid())
	{
		return 0;
	}

	TArray<FDiurnalResolvedTimeEvent> AllEvents;
	TArray<FDiurnalResolvedTimeRange> IgnoredRanges;
	TArray<TObjectPtr<UDiurnalSchedule>> IgnoredSchedules;
	if (!CompileScheduleLayers(ActiveScheduleReferences, RuntimeTimeEvents, RuntimeTimeRanges, {}, DisabledRangeEntries, AllEvents, IgnoredRanges, IgnoredSchedules))
	{
		return 0;
	}
	TArray<FDiurnalScheduleEntryReference> Matches;
	for (const FDiurnalResolvedTimeEvent& Event : AllEvents)
	{
		const FDiurnalScheduleEntryReference Reference = Event.GetEntryReference();
		if (DisabledEventEntries.Contains(Reference) && Event.Event.HasTagExact(EventTag))
		{
			Matches.Add(Reference);
		}
	}
	if (Matches.IsEmpty()) return 0;
	const TArray<FDiurnalScheduleEntryReference> PreviousDisabledEvents = DisabledEventEntries;
	for (const FDiurnalScheduleEntryReference& Reference : Matches)
	{
		DisabledEventEntries.Remove(Reference);
	}
	if (!RebuildCompiledSchedule(true))
	{
		DisabledEventEntries = PreviousDisabledEvents;
		return 0;
	}
	return Matches.Num();
}

#pragma endregion

#pragma region OccurrenceQueries

bool UDiurnalCycleSubsystem::TryGetNextTimeEvent(
	FDiurnalTimeEvent& OutEvent,
	FDiurnalDateTime& OutOccurrenceTime) const
{
	OutEvent = {};
	OutOccurrenceTime = {};
	double BestOccurrenceHours = TNumericLimits<double>::Max();
	const FDiurnalTimeEvent* BestEvent = nullptr;
	for (const FDiurnalTimeEvent& Event : TimeEvents)
	{
		double OccurrenceHours = 0.0;
		if (TryGetNextOccurrenceHours(Event, TotalGameHours, OccurrenceHours)
			&& OccurrenceHours < BestOccurrenceHours)
		{
			BestOccurrenceHours = OccurrenceHours;
			BestEvent = &Event;
		}
	}
	if (!BestEvent)
	{
		return false;
	}
	OutEvent = *BestEvent;
	OutOccurrenceTime = FDiurnalDateTime::FromTotalHours(BestOccurrenceHours);
	return true;
}

bool UDiurnalCycleSubsystem::TryGetNextOccurrence(
	const FGameplayTag EventTag,
	FDiurnalDateTime& OutOccurrenceTime) const
{
	OutOccurrenceTime = {};
	double BestOccurrenceHours = TNumericLimits<double>::Max();
	bool bFound = false;
	for (const FDiurnalResolvedTimeEvent& Match : FindTimeEventsByTag(EventTag))
	{
		double OccurrenceHours = 0.0;
		if (TryGetNextOccurrenceHours(Match.Event, TotalGameHours, OccurrenceHours)
			&& OccurrenceHours < BestOccurrenceHours)
		{
			BestOccurrenceHours = OccurrenceHours;
			bFound = true;
		}
	}
	if (!bFound)
	{
		return false;
	}
	OutOccurrenceTime = FDiurnalDateTime::FromTotalHours(BestOccurrenceHours);
	return true;
}

bool UDiurnalCycleSubsystem::TryGetNextOccurrence(
	const FDiurnalScheduleEntryReference& Reference,
	FDiurnalDateTime& OutOccurrenceTime) const
{
	OutOccurrenceTime = {};
	FDiurnalResolvedTimeEvent Resolved;
	double OccurrenceHours = 0.0;
	if (!TryGetTimeEvent(Reference, Resolved)
		|| !TryGetNextOccurrenceHours(Resolved.Event, TotalGameHours, OccurrenceHours))
	{
		return false;
	}
	OutOccurrenceTime = FDiurnalDateTime::FromTotalHours(OccurrenceHours);
	return true;
}

bool UDiurnalCycleSubsystem::TryGetNextOccurrenceHours(
	const FDiurnalTimeEvent& TimeEvent,
	const double FromHours,
	double& OutOccurrenceHours) const
{
	OutOccurrenceHours = 0.0;
	checkf(TimeEvent.IsValid(), TEXT("TryGetNextOccurrenceHours requires a valid event."));
	checkf(DiurnalCycle::IsValidTotalGameHours(FromHours), TEXT("TryGetNextOccurrenceHours requires valid source hours."));

	const double TimeOfDayHours = TimeEvent.TimeOfDay.ToHours();
	double CandidateHours = 0.0;
	const FDiurnalRecurrence Recurrence = TimeEvent.Recurrence;
	if (Recurrence.Mode == EDiurnalRecurrenceMode::Once)
	{
		CandidateHours = static_cast<double>(Recurrence.AnchorDay - 1)
			* DiurnalCycle::GHoursPerDay + TimeOfDayHours;
	}
	else
	{
		const int64 CurrentDay = static_cast<int64>(
			FMath::Floor(FromHours / DiurnalCycle::GHoursPerDay)) + 1;
		int64 CandidateDay = FMath::Max<int64>(Recurrence.AnchorDay, CurrentDay);
		const int64 Offset = (CandidateDay - Recurrence.AnchorDay) % Recurrence.IntervalDays;
		if (Offset != 0) CandidateDay += Recurrence.IntervalDays - Offset;
		CandidateHours = static_cast<double>(CandidateDay - 1)
			* DiurnalCycle::GHoursPerDay + TimeOfDayHours;
		if (CandidateHours <= FromHours)
		{
			CandidateHours += static_cast<double>(Recurrence.IntervalDays) * DiurnalCycle::GHoursPerDay;
		}
	}

	if (CandidateHours <= FromHours
		|| !DiurnalCycle::IsValidTotalGameHours(CandidateHours))
	{
		return false;
	}
	OutOccurrenceHours = CandidateHours;
	return true;
}

#pragma endregion

#pragma region EventProcessing

void UDiurnalCycleSubsystem::SortTimeEvents()
{
	TimeEvents.StableSort(
		[](const FDiurnalTimeEvent& Left, const FDiurnalTimeEvent& Right)
		{
			return Left.TimeOfDay < Right.TimeOfDay;
		});
}

void UDiurnalCycleSubsystem::DispatchCrossedTimeEvents(
	const double PreviousHours,
	const double CurrentHours)
{
	checkf(CurrentHours >= PreviousHours, TEXT("DispatchCrossedTimeEvents requires forward time."));
	if (ResolvedTimeEvents.IsEmpty() || CurrentHours == PreviousHours)
	{
		return;
	}

	struct FPendingDiurnalOccurrence
	{
		FDiurnalResolvedTimeEvent Resolved;
		FDiurnalEventOccurrenceHandle Handle;
	};
	TArray<FPendingDiurnalOccurrence> PendingOccurrences;
	const int64 FirstDayIndex = static_cast<int64>(FMath::Floor(PreviousHours / DiurnalCycle::GHoursPerDay));
	const int64 LastDayIndex = static_cast<int64>(FMath::Floor(CurrentHours / DiurnalCycle::GHoursPerDay));

	for (int64 DayIndex = FirstDayIndex; DayIndex <= LastDayIndex; ++DayIndex)
	{
		const int32 OccurrenceDay = static_cast<int32>(DayIndex + 1);
		const double DayStartHours = static_cast<double>(DayIndex) * DiurnalCycle::GHoursPerDay;
		for (const FDiurnalResolvedTimeEvent& Resolved : ResolvedTimeEvents)
		{
			const FDiurnalTimeEvent& Event = Resolved.Event;
			if (!Event.OccursOnDay(OccurrenceDay))
			{
				continue;
			}
			const double OccurrenceHours = DayStartHours + Event.TimeOfDay.ToHours();
			if (OccurrenceHours <= PreviousHours || OccurrenceHours > CurrentHours)
			{
				continue;
			}

			FPendingDiurnalOccurrence& Pending = PendingOccurrences.AddDefaulted_GetRef();
			Pending.Resolved = Resolved;
			Pending.Handle.Entry = Resolved.GetEntryReference();
			Pending.Handle.OccurrenceTime = FDiurnalDateTime(OccurrenceDay, Event.TimeOfDay);
			Pending.Handle.OccurrenceId = FGuid::NewGuid();
		}
	}

	bool bFinalGateStateApplied = false;
	const FDiurnalDateTime FinalDateTime = GetDateTime();
	for (const FPendingDiurnalOccurrence& Pending : PendingOccurrences)
	{
		if (!bFinalGateStateApplied && Pending.Handle.OccurrenceTime == FinalDateTime)
		{
			TArray<FDiurnalEventOccurrenceHandle> ScheduledGates;
			for (const FPendingDiurnalOccurrence& Candidate : PendingOccurrences)
			{
				if (Candidate.Handle.OccurrenceTime == FinalDateTime
					&& Candidate.Resolved.Event.IsBlocking())
				{
					ScheduledGates.Add(Candidate.Handle);
				}
			}
			if (!ScheduledGates.IsEmpty())
			{
				SetActiveTimeGates(ScheduledGates, FinalDateTime, true);
			}
			bFinalGateStateApplied = true;
		}

		TimeEventOccurrenceEvent.Broadcast(
			Pending.Handle,
			Pending.Resolved.Event,
			Pending.Handle.OccurrenceTime);
		TimeEventTriggeredEvent.Broadcast(
			Pending.Resolved.Event,
			Pending.Handle.OccurrenceTime);
	}
}

#pragma endregion
