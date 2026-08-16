#include "Subsystem/DiurnalCycleSubsystem.h"

#include "DiurnalCycleLog.h"

#pragma region TimeRangeSchedule

TArray<FDiurnalResolvedTimeRange> UDiurnalCycleSubsystem::FindTimeRangesByTag(
	const FGameplayTag RangeTag) const
{
	TArray<FDiurnalResolvedTimeRange> Result;
	if (!RangeTag.IsValid())
	{
		return Result;
	}
	for (const FDiurnalResolvedTimeRange& Resolved : ResolvedTimeRanges)
	{
		if (Resolved.Range.HasTagExact(RangeTag))
		{
			Result.Add(Resolved);
		}
	}
	return Result;
}

TArray<FDiurnalResolvedTimeRange> UDiurnalCycleSubsystem::FindTimeRangesByTagQuery(
	const FGameplayTagQuery& TagQuery) const
{
	TArray<FDiurnalResolvedTimeRange> Result;
	for (const FDiurnalResolvedTimeRange& Resolved : ResolvedTimeRanges)
	{
		if (TagQuery.IsEmpty() || TagQuery.Matches(Resolved.Range.RangeTags))
		{
			Result.Add(Resolved);
		}
	}
	return Result;
}

bool UDiurnalCycleSubsystem::TryGetTimeRange(
	const FDiurnalScheduleEntryReference& Reference,
	FDiurnalResolvedTimeRange& OutTimeRange) const
{
	OutTimeRange = {};
	if (!Reference.IsValid())
	{
		return false;
	}
	const FDiurnalResolvedTimeRange* Match = ResolvedTimeRanges.FindByPredicate(
		[&Reference](const FDiurnalResolvedTimeRange& Candidate)
		{
			return Candidate.GetEntryReference() == Reference;
		});
	if (!Match)
	{
		return false;
	}
	OutTimeRange = *Match;
	return true;
}

bool UDiurnalCycleSubsystem::HasTimeRange(const FGameplayTag RangeTag) const
{
	return RangeTag.IsValid()
		&& ResolvedTimeRanges.ContainsByPredicate(
			[RangeTag](const FDiurnalResolvedTimeRange& Resolved)
			{
				return Resolved.Range.HasTagExact(RangeTag);
			});
}

bool UDiurnalCycleSubsystem::TryAddTimeRange(const FDiurnalTimeRange& TimeRange)
{
	FDiurnalScheduleEntryReference IgnoredReference;
	return TryAddTimeRange(TimeRange, IgnoredReference);
}

bool UDiurnalCycleSubsystem::TryAddTimeRange(
	const FDiurnalTimeRange& TimeRange,
	FDiurnalScheduleEntryReference& OutReference)
{
	OutReference = {};
	FDiurnalTimeRange AddedRange = TimeRange;
	if (!AddedRange.IsValid())
	{
		UE_LOG(
			LogDiurnalCycle,
			Warning,
			TEXT("Rejected invalid time range '%s' (%s -> %s)."),
			*AddedRange.GetDisplayName().ToString(),
			*AddedRange.StartTime.ToString(),
			*AddedRange.EndTime.ToString());
		return false;
	}

	if (AddedRange.RangeName.IsNone())
	{
		const FGameplayTag FirstTag = AddedRange.GetPrimaryTag();
		AddedRange.RangeName = FirstTag.IsValid()
			? FirstTag.GetTagLeafName()
			: FName(TEXT("Runtime Range"));
	}

	const auto IdCollides = [this](const FGuid& EntryId)
	{
		return ResolvedTimeRanges.ContainsByPredicate(
			[EntryId](const FDiurnalResolvedTimeRange& Existing)
			{
				return Existing.SourceSchedule.IsNull()
					&& Existing.Range.EntryId == EntryId;
			});
	};
	if (!AddedRange.EntryId.IsValid() || IdCollides(AddedRange.EntryId))
	{
		do
		{
			AddedRange.EntryId = FGuid::NewGuid();
		}
		while (IdCollides(AddedRange.EntryId));
	}

	RuntimeTimeRanges.Add(AddedRange);
	if (!RebuildCompiledSchedule(true))
	{
		RuntimeTimeRanges.Pop();
		return false;
	}
	OutReference.EntryId = AddedRange.EntryId;
	return true;
}

bool UDiurnalCycleSubsystem::RemoveTimeRange(
	const FDiurnalScheduleEntryReference& Reference)
{
	FDiurnalResolvedTimeRange Resolved;
	if (!TryGetTimeRange(Reference, Resolved))
	{
		return false;
	}
	const int32 RuntimeIndex = Resolved.bRuntimeAdded
		? RuntimeTimeRanges.IndexOfByPredicate(
			[&Reference](const FDiurnalTimeRange& Range)
			{
				return Range.EntryId == Reference.EntryId;
			})
		: INDEX_NONE;
	FDiurnalTimeRange RemovedRuntimeRange;
	if (RuntimeIndex != INDEX_NONE)
	{
		RemovedRuntimeRange = RuntimeTimeRanges[RuntimeIndex];
		RuntimeTimeRanges.RemoveAt(RuntimeIndex);
	}
	else
	{
		DisabledRangeEntries.AddUnique(Reference);
	}

	if (!RebuildCompiledSchedule(true))
	{
		if (RuntimeIndex != INDEX_NONE)
		{
			RuntimeTimeRanges.Insert(RemovedRuntimeRange, RuntimeIndex);
		}
		else
		{
			DisabledRangeEntries.Remove(Reference);
		}
		return false;
	}
	return true;
}

int32 UDiurnalCycleSubsystem::RemoveTimeRangesByTag(const FGameplayTag RangeTag)
{
	const TArray<FDiurnalResolvedTimeRange> Matches = FindTimeRangesByTag(RangeTag);
	if (Matches.IsEmpty()) return 0;
	const TArray<FDiurnalTimeRange> PreviousRuntimeRanges = RuntimeTimeRanges;
	const TArray<FDiurnalScheduleEntryReference> PreviousDisabledRanges = DisabledRangeEntries;
	for (const FDiurnalResolvedTimeRange& Match : Matches)
	{
		const FDiurnalScheduleEntryReference Reference = Match.GetEntryReference();
		if (Match.bRuntimeAdded)
		{
			RuntimeTimeRanges.RemoveAll([&Reference](const FDiurnalTimeRange& Range)
			{
				return Range.EntryId == Reference.EntryId;
			});
		}
		else
		{
			DisabledRangeEntries.AddUnique(Reference);
		}
	}
	if (!RebuildCompiledSchedule(true))
	{
		RuntimeTimeRanges = PreviousRuntimeRanges;
		DisabledRangeEntries = PreviousDisabledRanges;
		return 0;
	}
	return Matches.Num();
}

bool UDiurnalCycleSubsystem::ReenableTimeRange(
	const FDiurnalScheduleEntryReference& Reference)
{
	if (!Reference.IsValid() || DisabledRangeEntries.Remove(Reference) == 0)
	{
		return false;
	}
	if (!RebuildCompiledSchedule(true))
	{
		DisabledRangeEntries.AddUnique(Reference);
		return false;
	}
	return true;
}

int32 UDiurnalCycleSubsystem::ReenableTimeRangesByTag(const FGameplayTag RangeTag)
{
	if (!RangeTag.IsValid())
	{
		return 0;
	}
	TArray<FDiurnalResolvedTimeEvent> IgnoredEvents;
	TArray<FDiurnalResolvedTimeRange> AllRanges;
	TArray<TObjectPtr<UDiurnalSchedule>> IgnoredSchedules;
	if (!CompileScheduleLayers(ActiveScheduleReferences, RuntimeTimeEvents, RuntimeTimeRanges, DisabledEventEntries, {}, IgnoredEvents, AllRanges, IgnoredSchedules))
	{
		return 0;
	}
	TArray<FDiurnalScheduleEntryReference> Matches;
	for (const FDiurnalResolvedTimeRange& Range : AllRanges)
	{
		const FDiurnalScheduleEntryReference Reference = Range.GetEntryReference();
		if (DisabledRangeEntries.Contains(Reference) && Range.Range.HasTagExact(RangeTag))
		{
			Matches.Add(Reference);
		}
	}
	if (Matches.IsEmpty()) return 0;
	const TArray<FDiurnalScheduleEntryReference> PreviousDisabledRanges = DisabledRangeEntries;
	for (const FDiurnalScheduleEntryReference& Reference : Matches)
	{
		DisabledRangeEntries.Remove(Reference);
	}
	if (!RebuildCompiledSchedule(true))
	{
		DisabledRangeEntries = PreviousDisabledRanges;
		return 0;
	}
	return Matches.Num();
}

bool UDiurnalCycleSubsystem::IsTimeOfDayInRange(
	const FGameplayTag RangeTag,
	const FDiurnalTimeOfDay& TimeOfDay) const
{
	if (!TimeOfDay.IsValid())
	{
		return false;
	}
	for (const FDiurnalResolvedTimeRange& Match : FindTimeRangesByTag(RangeTag))
	{
		if (Match.Range.Contains(TimeOfDay))
		{
			return true;
		}
	}
	return false;
}

bool UDiurnalCycleSubsystem::IsCurrentTimeInRange(const FGameplayTag RangeTag) const
{
	const FDiurnalDateTime DateTime = GetDateTime();
	for (const FDiurnalResolvedTimeRange& Match : FindTimeRangesByTag(RangeTag))
	{
		if (Match.Range.ContainsOnDay(DateTime.Day, DateTime.GetTimeOfDay())) return true;
	}
	return false;
}

TArray<FGameplayTag> UDiurnalCycleSubsystem::GetActiveTimeRangeTags() const
{
	TArray<FGameplayTag> ActiveTags;
	for (const FDiurnalResolvedTimeRange& Resolved : GetActiveTimeRangeDefinitions(GetDateTime()))
	{
		for (const FGameplayTag Tag : Resolved.Range.RangeTags.GetGameplayTagArray())
		{
			ActiveTags.AddUnique(Tag);
		}
	}
	return ActiveTags;
}

TArray<FDiurnalScheduleEntryReference> UDiurnalCycleSubsystem::GetActiveTimeRangeEntries() const
{
	TArray<FDiurnalScheduleEntryReference> Result;
	for (const FDiurnalResolvedTimeRange& Resolved : GetActiveTimeRangeDefinitions(GetDateTime()))
	{
		Result.Add(Resolved.GetEntryReference());
	}
	return Result;
}

#pragma endregion

#pragma region TimeRangeProcessing

TArray<FDiurnalResolvedTimeRange> UDiurnalCycleSubsystem::GetActiveTimeRangeDefinitions(
	const FDiurnalDateTime& DateTime) const
{
	checkf(DateTime.IsValid(), TEXT("GetActiveTimeRangeDefinitions requires a valid date-time."));
	TArray<FDiurnalResolvedTimeRange> Result;
	for (const FDiurnalResolvedTimeRange& Resolved : ResolvedTimeRanges)
	{
		if (Resolved.Range.ContainsOnDay(DateTime.Day, DateTime.GetTimeOfDay()))
		{
			Result.Add(Resolved);
		}
	}
	return Result;
}

void UDiurnalCycleSubsystem::BroadcastTimeRangeTransitions(
	const TArray<FDiurnalResolvedTimeRange>& PreviousActiveRanges,
	const TArray<FDiurnalResolvedTimeRange>& CurrentActiveRanges,
	const FDiurnalDateTime& CurrentDateTime)
{
	const auto ContainsReference = [](
		const TArray<FDiurnalResolvedTimeRange>& Ranges,
		const FDiurnalScheduleEntryReference& Reference)
	{
		return Ranges.ContainsByPredicate(
			[&Reference](const FDiurnalResolvedTimeRange& Range)
			{
				return Range.GetEntryReference() == Reference;
			});
	};

	for (const FDiurnalResolvedTimeRange& Previous : PreviousActiveRanges)
	{
		const FDiurnalScheduleEntryReference Reference = Previous.GetEntryReference();
		if (!ContainsReference(CurrentActiveRanges, Reference))
		{
			TimeRangeEntryExitedEvent.Broadcast(Reference, Previous.Range, CurrentDateTime);
			TimeRangeExitedEvent.Broadcast(Previous.Range, CurrentDateTime);
		}
	}
	for (const FDiurnalResolvedTimeRange& Current : CurrentActiveRanges)
	{
		const FDiurnalScheduleEntryReference Reference = Current.GetEntryReference();
		if (!ContainsReference(PreviousActiveRanges, Reference))
		{
			TimeRangeEntryEnteredEvent.Broadcast(Reference, Current.Range, CurrentDateTime);
			TimeRangeEnteredEvent.Broadcast(Current.Range, CurrentDateTime);
		}
	}
}

#pragma endregion
