#include "Subsystem/DiurnalCycleSubsystem.h"

#include "DiurnalCycleLog.h"

#pragma region TimeRangeSchedule

bool UDiurnalCycleSubsystem::TryGetTimeRange(
	const FGameplayTag RangeTag,
	FDiurnalTimeRange& OutTimeRange) const
{
	if (!RangeTag.IsValid())
	{
		return false;
	}

	const FDiurnalTimeRange* TimeRange =
		TimeRanges.FindByPredicate(
			[RangeTag](
				const FDiurnalTimeRange& Candidate)
			{
				return Candidate.RangeTag
					== RangeTag;
			});

	if (!TimeRange)
	{
		return false;
	}

	OutTimeRange =
		*TimeRange;

	return true;
}

bool UDiurnalCycleSubsystem::HasTimeRange(
	const FGameplayTag RangeTag) const
{
	if (!RangeTag.IsValid())
	{
		return false;
	}

	return TimeRanges.ContainsByPredicate(
		[RangeTag](
			const FDiurnalTimeRange& Range)
		{
			return Range.RangeTag
				== RangeTag;
		});
}

bool UDiurnalCycleSubsystem::TryAddTimeRange(
	const FDiurnalTimeRange& TimeRange)
{
	if (!TimeRange.IsValid())
	{
		UE_LOG(
			LogDiurnalCycle,
			Warning,
			TEXT(
				"Rejected invalid time range '%s' "
				"(%s -> %s)."),
			*TimeRange.RangeTag.ToString(),
			*TimeRange.StartTime.ToString(),
			*TimeRange.EndTime.ToString());

		return false;
	}

	if (HasTimeRange(
			TimeRange.RangeTag))
	{
		UE_LOG(
			LogDiurnalCycle,
			Warning,
			TEXT(
				"Rejected time range '%s' because "
				"range tags must be unique."),
			*TimeRange.RangeTag.ToString());

		return false;
	}

	const FDiurnalTimeOfDay CurrentTimeOfDay =
		GetTimeOfDay();

	const TArray<FDiurnalTimeRange>
		PreviousActiveRanges =
			GetActiveTimeRangeDefinitions(
				CurrentTimeOfDay);

	const FDiurnalTimeRange AddedRange =
		TimeRange;

	TimeRanges.Add(
		AddedRange);

	UE_LOG(
		LogDiurnalCycle,
		Verbose,
		TEXT(
			"Added time range '%s' (%s -> %s)."),
		*AddedRange.RangeTag.ToString(),
		*AddedRange.StartTime.ToString(),
		*AddedRange.EndTime.ToString());

	TimeRangeAddedEvent.Broadcast(
		AddedRange);

	const TArray<FDiurnalTimeRange>
		CurrentActiveRanges =
			GetActiveTimeRangeDefinitions(
				CurrentTimeOfDay);

	BroadcastTimeRangeTransitions(
		PreviousActiveRanges,
		CurrentActiveRanges,
		GetDateTime());

	return true;
}

bool UDiurnalCycleSubsystem::RemoveTimeRange(
	const FGameplayTag RangeTag)
{
	if (!RangeTag.IsValid())
	{
		return false;
	}

	const int32 RangeIndex =
		TimeRanges.IndexOfByPredicate(
			[RangeTag](
				const FDiurnalTimeRange& Range)
			{
				return Range.RangeTag
					== RangeTag;
			});

	if (RangeIndex == INDEX_NONE)
	{
		return false;
	}

	const FDiurnalTimeOfDay CurrentTimeOfDay =
		GetTimeOfDay();

	const TArray<FDiurnalTimeRange>
		PreviousActiveRanges =
			GetActiveTimeRangeDefinitions(
				CurrentTimeOfDay);

	const FDiurnalTimeRange RemovedRange =
		TimeRanges[RangeIndex];

	TimeRanges.RemoveAt(
		RangeIndex);

	UE_LOG(
		LogDiurnalCycle,
		Verbose,
		TEXT(
			"Removed time range '%s'."),
		*RemovedRange.RangeTag.ToString());

	TimeRangeRemovedEvent.Broadcast(
		RemovedRange);

	const TArray<FDiurnalTimeRange>
		CurrentActiveRanges =
			GetActiveTimeRangeDefinitions(
				CurrentTimeOfDay);

	BroadcastTimeRangeTransitions(
		PreviousActiveRanges,
		CurrentActiveRanges,
		GetDateTime());

	return true;
}

bool UDiurnalCycleSubsystem::IsTimeOfDayInRange(
	const FGameplayTag RangeTag,
	const FDiurnalTimeOfDay& TimeOfDay) const
{
	if (!TimeOfDay.IsValid())
	{
		return false;
	}

	FDiurnalTimeRange TimeRange;

	return TryGetTimeRange(
			RangeTag,
			TimeRange)
		&& TimeRange.Contains(
			TimeOfDay);
}

bool UDiurnalCycleSubsystem::IsCurrentTimeInRange(
	const FGameplayTag RangeTag) const
{
	return IsTimeOfDayInRange(
		RangeTag,
		GetTimeOfDay());
}

TArray<FGameplayTag>
UDiurnalCycleSubsystem::GetActiveTimeRanges() const
{
	const TArray<FDiurnalTimeRange>
		ActiveRangeDefinitions =
			GetActiveTimeRangeDefinitions(
				GetTimeOfDay());

	TArray<FGameplayTag> ActiveRangeTags;

	ActiveRangeTags.Reserve(
		ActiveRangeDefinitions.Num());

	for (const FDiurnalTimeRange& Range :
		 ActiveRangeDefinitions)
	{
		ActiveRangeTags.Add(
			Range.RangeTag);
	}

	return ActiveRangeTags;
}

#pragma endregion

#pragma region TimeRangeProcessing

TArray<FDiurnalTimeRange>
UDiurnalCycleSubsystem::GetActiveTimeRangeDefinitions(
	const FDiurnalTimeOfDay& TimeOfDay) const
{
	checkf(
		TimeOfDay.IsValid(),
		TEXT(
			"GetActiveTimeRangeDefinitions requires "
			"a valid time of day."));

	TArray<FDiurnalTimeRange> Result;

	for (const FDiurnalTimeRange& Range :
		 TimeRanges)
	{
		if (Range.Contains(
				TimeOfDay))
		{
			Result.Add(
				Range);
		}
	}

	return Result;
}

void UDiurnalCycleSubsystem::BroadcastTimeRangeTransitions(
	const TArray<FDiurnalTimeRange>& PreviousActiveRanges,
	const TArray<FDiurnalTimeRange>& CurrentActiveRanges,
	const FDiurnalDateTime& CurrentDateTime)
{
	const auto ContainsTag =
		[](
			const TArray<FDiurnalTimeRange>& Ranges,
			const FGameplayTag RangeTag)
		{
			return Ranges.ContainsByPredicate(
				[RangeTag](
					const FDiurnalTimeRange& Range)
				{
					return Range.RangeTag
						== RangeTag;
				});
		};

	/*
	 * Exits are emitted first. Both arrays are snapshots, so callbacks may
	 * freely mutate the runtime range schedule without changing this batch.
	 */
	for (const FDiurnalTimeRange& PreviousRange :
		 PreviousActiveRanges)
	{
		if (ContainsTag(
				CurrentActiveRanges,
				PreviousRange.RangeTag))
		{
			continue;
		}

		TimeRangeExitedEvent.Broadcast(
			PreviousRange,
			CurrentDateTime);
	}

	for (const FDiurnalTimeRange& CurrentRange :
		 CurrentActiveRanges)
	{
		if (ContainsTag(
				PreviousActiveRanges,
				CurrentRange.RangeTag))
		{
			continue;
		}

		TimeRangeEnteredEvent.Broadcast(
			CurrentRange,
			CurrentDateTime);
	}
}

#pragma endregion
