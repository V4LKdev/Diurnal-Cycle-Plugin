#include "Subsystem/DiurnalCycleSubsystem.h"

#include "DiurnalCycleLog.h"
#include "DiurnalCycleSettings.h"
#include "DiurnalSchedule.h"
namespace
{
	template <typename ResolvedType>
	bool ContainsReference(
		const TArray<ResolvedType>& Entries,
		const FDiurnalScheduleEntryReference& Reference)
	{
		return Entries.ContainsByPredicate(
			[&Reference](const ResolvedType& Entry)
			{
				return Entry.GetEntryReference() == Reference;
			});
	}
}

bool UDiurnalCycleSubsystem::CompileScheduleLayers(
	const TArray<TSoftObjectPtr<UDiurnalSchedule>>& ScheduleReferences,
	const TArray<FDiurnalTimeEvent>& InRuntimeEvents,
	const TArray<FDiurnalTimeRange>& InRuntimeRanges,
	const TArray<FDiurnalScheduleEntryReference>& InDisabledEvents,
	const TArray<FDiurnalScheduleEntryReference>& InDisabledRanges,
	TArray<FDiurnalResolvedTimeEvent>& OutEvents,
	TArray<FDiurnalResolvedTimeRange>& OutRanges,
	TArray<TObjectPtr<UDiurnalSchedule>>& OutLoadedSchedules) const
{
	OutEvents.Reset();
	OutRanges.Reset();
	OutLoadedSchedules.Reset();
	TSet<FDiurnalScheduleEntryReference> SeenEventReferences;
	TSet<FDiurnalScheduleEntryReference> SeenRangeReferences;

	const auto AddEvent = [&](const FDiurnalTimeEvent& Event, const int32 EntryIndex, const TSoftObjectPtr<UDiurnalSchedule>& Source, const bool bRuntime)
	{
		FDiurnalTimeEvent Normalized = Event;
		if (Normalized.EventName.IsNone())
		{
			const FGameplayTag FirstTag = Normalized.GetPrimaryTag();
			Normalized.EventName = FirstTag.IsValid()
				? FirstTag.GetTagLeafName()
				: FName(*FString::Printf(TEXT("Runtime Event %d"), EntryIndex + 1));
		}
		FDiurnalScheduleEntryReference Reference;
		Reference.Schedule = Source;
		Reference.EntryId = Normalized.EntryId;
		if (!Normalized.IsValid() || !Reference.IsValid() || SeenEventReferences.Contains(Reference))
		{
			UE_LOG(LogDiurnalCycle, Warning, TEXT("Rejected schedule composition: invalid or duplicated event reference '%s'."), *Reference.ToString());
			return false;
		}
		SeenEventReferences.Add(Reference);
		if (!InDisabledEvents.Contains(Reference))
		{
			FDiurnalResolvedTimeEvent& Resolved = OutEvents.AddDefaulted_GetRef();
			Resolved.Event = MoveTemp(Normalized);
			Resolved.SourceSchedule = Source;
			Resolved.bRuntimeAdded = bRuntime;
		}
		return true;
	};

	const auto AddRange = [&](const FDiurnalTimeRange& Range, const int32 EntryIndex, const TSoftObjectPtr<UDiurnalSchedule>& Source, const bool bRuntime)
	{
		FDiurnalTimeRange Normalized = Range;
		if (Normalized.RangeName.IsNone())
		{
			const FGameplayTag FirstTag = Normalized.GetPrimaryTag();
			Normalized.RangeName = FirstTag.IsValid()
				? FirstTag.GetTagLeafName()
				: FName(*FString::Printf(TEXT("Runtime Range %d"), EntryIndex + 1));
		}
		FDiurnalScheduleEntryReference Reference;
		Reference.Schedule = Source;
		Reference.EntryId = Normalized.EntryId;
		if (!Normalized.IsValid() || !Reference.IsValid() || SeenRangeReferences.Contains(Reference))
		{
			UE_LOG(LogDiurnalCycle, Warning, TEXT("Rejected schedule composition: invalid or duplicated range reference '%s'."), *Reference.ToString());
			return false;
		}
		SeenRangeReferences.Add(Reference);
		if (!InDisabledRanges.Contains(Reference))
		{
			FDiurnalResolvedTimeRange& Resolved = OutRanges.AddDefaulted_GetRef();
			Resolved.Range = MoveTemp(Normalized);
			Resolved.SourceSchedule = Source;
			Resolved.bRuntimeAdded = bRuntime;
		}
		return true;
	};

	int32 FirstDuplicateIndex = INDEX_NONE;
	int32 DuplicateIndex = INDEX_NONE;
	FSoftObjectPath DuplicatePath;
	if (DiurnalCycle::FindDuplicateScheduleReference(ScheduleReferences, FirstDuplicateIndex, DuplicateIndex, DuplicatePath))
	{
		UE_LOG(LogDiurnalCycle, Error, TEXT("Rejected schedule composition atomically: schedule reference at index %d duplicates index %d ('%s')."), DuplicateIndex, FirstDuplicateIndex, *DuplicatePath.ToString());
		return false;
	}

	for (int32 ScheduleIndex = 0; ScheduleIndex < ScheduleReferences.Num(); ++ScheduleIndex)
	{
		const TSoftObjectPtr<UDiurnalSchedule>& Reference = ScheduleReferences[ScheduleIndex];
		const FSoftObjectPath Path = Reference.ToSoftObjectPath();
		if (Path.IsNull())
		{
			UE_LOG(LogDiurnalCycle, Error, TEXT("Rejected schedule composition atomically: schedule reference at index %d is empty."), ScheduleIndex);
			return false;
		}
		UDiurnalSchedule* Schedule = Reference.Get();
		if (!IsValid(Schedule))
		{
			UE_LOG(LogDiurnalCycle, Log,
				TEXT("Synchronously loading schedule '%s' during schedule activation."),
				*Path.ToString());
			Schedule = Reference.LoadSynchronous();
		}
		if (!IsValid(Schedule))
		{
			UE_LOG(LogDiurnalCycle, Warning, TEXT("Rejected schedule composition: could not load '%s'."), *Path.ToString());
			return false;
		}
		OutLoadedSchedules.Add(Schedule);
		for (int32 Index = 0; Index < Schedule->TimeEvents.Num(); ++Index)
		{
			if (!AddEvent(Schedule->TimeEvents[Index], Index, Reference, false)) return false;
		}
		for (int32 Index = 0; Index < Schedule->TimeRanges.Num(); ++Index)
		{
			if (!AddRange(Schedule->TimeRanges[Index], Index, Reference, false)) return false;
		}
	}

	for (int32 Index = 0; Index < InRuntimeEvents.Num(); ++Index)
	{
		if (!AddEvent(InRuntimeEvents[Index], Index, nullptr, true)) return false;
	}
	for (int32 Index = 0; Index < InRuntimeRanges.Num(); ++Index)
	{
		if (!AddRange(InRuntimeRanges[Index], Index, nullptr, true)) return false;
	}

	OutEvents.StableSort([](const FDiurnalResolvedTimeEvent& Left, const FDiurnalResolvedTimeEvent& Right)
	{
		return Left.Event.TimeOfDay < Right.Event.TimeOfDay;
	});
	return true;
}

void UDiurnalCycleSubsystem::ApplyCompiledSchedule(
	TArray<FDiurnalResolvedTimeEvent>&& NewEvents,
	TArray<FDiurnalResolvedTimeRange>&& NewRanges,
	TArray<TObjectPtr<UDiurnalSchedule>>&& NewLoadedSchedules,
	const bool bBroadcastTransitions)
{
	const FDiurnalDateTime CurrentDateTime = GetDateTime();
	const TArray<FDiurnalResolvedTimeRange> PreviousActiveRanges = GetActiveTimeRangeDefinitions(CurrentDateTime);
	const TArray<FDiurnalResolvedTimeEvent> PreviousEvents = ResolvedTimeEvents;
	const TArray<FDiurnalResolvedTimeRange> PreviousRanges = ResolvedTimeRanges;

	ResolvedTimeEvents = MoveTemp(NewEvents);
	ResolvedTimeRanges = MoveTemp(NewRanges);
	ActiveScheduleAssets = MoveTemp(NewLoadedSchedules);
	TimeEvents.Reset(ResolvedTimeEvents.Num());
	for (const FDiurnalResolvedTimeEvent& Resolved : ResolvedTimeEvents) TimeEvents.Add(Resolved.Event);
	TimeRanges.Reset(ResolvedTimeRanges.Num());
	for (const FDiurnalResolvedTimeRange& Resolved : ResolvedTimeRanges) TimeRanges.Add(Resolved.Range);

	if (!bBroadcastTransitions)
	{
		return;
	}

	for (const FDiurnalResolvedTimeEvent& Previous : PreviousEvents)
	{
		if (!ContainsReference(ResolvedTimeEvents, Previous.GetEntryReference()))
		{
			TimeEventRemovedEvent.Broadcast(Previous.Event);
		}
	}
	for (const FDiurnalResolvedTimeEvent& Current : ResolvedTimeEvents)
	{
		if (!ContainsReference(PreviousEvents, Current.GetEntryReference()))
		{
			TimeEventAddedEvent.Broadcast(Current.Event);
		}
	}
	for (const FDiurnalResolvedTimeRange& Previous : PreviousRanges)
	{
		if (!ContainsReference(ResolvedTimeRanges, Previous.GetEntryReference()))
		{
			TimeRangeRemovedEvent.Broadcast(Previous.Range);
		}
	}
	for (const FDiurnalResolvedTimeRange& Current : ResolvedTimeRanges)
	{
		if (!ContainsReference(PreviousRanges, Current.GetEntryReference()))
		{
			TimeRangeAddedEvent.Broadcast(Current.Range);
		}
	}

	TArray<FDiurnalEventOccurrenceHandle> NewGates;
	for (const FDiurnalEventOccurrenceHandle& ExistingGate : ActiveTimeGateOccurrences)
	{
		if (ContainsReference(ResolvedTimeEvents, ExistingGate.Entry))
		{
			NewGates.Add(ExistingGate);
		}
	}
	for (const FDiurnalEventOccurrenceHandle& ScheduledGate : GetScheduledTimeGatesAt(GetDateTime()))
	{
		const bool bWasPreviouslyAuthored = ContainsReference(PreviousEvents, ScheduledGate.Entry);
		if (!bWasPreviouslyAuthored)
		{
			NewGates.Add(ScheduledGate);
		}
	}
	SetActiveTimeGates(NewGates, GetDateTime(), true);
	BroadcastTimeRangeTransitions(PreviousActiveRanges, GetActiveTimeRangeDefinitions(CurrentDateTime), CurrentDateTime);
}

bool UDiurnalCycleSubsystem::RebuildCompiledSchedule(const bool bBroadcastTransitions)
{
	TArray<FDiurnalResolvedTimeEvent> NewEvents;
	TArray<FDiurnalResolvedTimeRange> NewRanges;
	TArray<TObjectPtr<UDiurnalSchedule>> LoadedSchedules;
	if (!CompileScheduleLayers(ActiveScheduleReferences, RuntimeTimeEvents, RuntimeTimeRanges, DisabledEventEntries, DisabledRangeEntries, NewEvents, NewRanges, LoadedSchedules))
	{
		return false;
	}
	ApplyCompiledSchedule(MoveTemp(NewEvents), MoveTemp(NewRanges), MoveTemp(LoadedSchedules), bBroadcastTransitions);
	return true;
}

bool UDiurnalCycleSubsystem::TrySetActiveSchedules(const TArray<TSoftObjectPtr<UDiurnalSchedule>>& NewSchedules)
{
	TArray<FDiurnalResolvedTimeEvent> NewEvents;
	TArray<FDiurnalResolvedTimeRange> NewRanges;
	TArray<TObjectPtr<UDiurnalSchedule>> LoadedSchedules;
	if (!CompileScheduleLayers(NewSchedules, RuntimeTimeEvents, RuntimeTimeRanges, DisabledEventEntries, DisabledRangeEntries, NewEvents, NewRanges, LoadedSchedules)) return false;
	ActiveScheduleReferences = NewSchedules;
	ApplyCompiledSchedule(MoveTemp(NewEvents), MoveTemp(NewRanges), MoveTemp(LoadedSchedules), true);
	return true;
}

bool UDiurnalCycleSubsystem::TryActivateSchedule(UDiurnalSchedule* Schedule)
{
	if (!IsValid(Schedule)) return false;
	TArray<TSoftObjectPtr<UDiurnalSchedule>> NewSchedules = ActiveScheduleReferences;
	const TSoftObjectPtr<UDiurnalSchedule> Reference(Schedule);
	if (NewSchedules.Contains(Reference)) return true;
	NewSchedules.Add(Reference);
	return TrySetActiveSchedules(NewSchedules);
}

bool UDiurnalCycleSubsystem::DeactivateSchedule(UDiurnalSchedule* Schedule)
{
	if (!IsValid(Schedule)) return false;
	TArray<TSoftObjectPtr<UDiurnalSchedule>> NewSchedules = ActiveScheduleReferences;
	if (NewSchedules.Remove(TSoftObjectPtr<UDiurnalSchedule>(Schedule)) == 0) return false;
	return TrySetActiveSchedules(NewSchedules);
}
