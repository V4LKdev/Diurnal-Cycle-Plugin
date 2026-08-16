#include "ScheduleEditor/DiurnalScheduleEditorViewState.h"

#include "DiurnalSchedule.h"

#define LOCTEXT_NAMESPACE "DiurnalScheduleEditorViewState"

namespace
{
	FString TagsSearchText(const FGameplayTagContainer& Tags)
	{
		TArray<FGameplayTag> Values = Tags.GetGameplayTagArray();
		FString Result;
		for (const FGameplayTag& Tag : Values)
		{
			Result += TEXT(" ");
			Result += Tag.ToString();
			Result += TEXT(" ");
			Result += Tag.GetTagLeafName().ToString();
		}
		return Result;
	}

	bool MatchesTokens(const FString& SearchText, const FString& SearchableText)
	{
		TArray<FString> Tokens;
		SearchText.TrimStartAndEnd().ParseIntoArrayWS(Tokens);
		for (const FString& Token : Tokens)
		{
			if (!SearchableText.Contains(Token, ESearchCase::IgnoreCase))
			{
				return false;
			}
		}
		return true;
	}

	int32 CompareGuid(const FGuid& Left, const FGuid& Right)
	{
		if (Left.A != Right.A) return Left.A < Right.A ? -1 : 1;
		if (Left.B != Right.B) return Left.B < Right.B ? -1 : 1;
		if (Left.C != Right.C) return Left.C < Right.C ? -1 : 1;
		if (Left.D != Right.D) return Left.D < Right.D ? -1 : 1;
		return 0;
	}

	int32 ChronologicalDay(const FDiurnalScheduleListItem& Item)
	{
		return Item.Type == EDiurnalScheduleSelectionType::Event && Item.bOneOff
			? Item.AnchorDay
			: 0;
	}

	int32 DayAndTimeGroup(const FDiurnalScheduleListItem& Item)
	{
		if (Item.Type == EDiurnalScheduleSelectionType::Range) return 2;
		return Item.bOneOff ? 1 : 0;
	}

	int32 CompareItems(
		const FDiurnalScheduleListItem& Left,
		const FDiurnalScheduleListItem& Right,
		const EDiurnalScheduleSortMode SortMode)
	{
		const auto CompareInt = [](const int32 A, const int32 B) { return A == B ? 0 : A < B ? -1 : 1; };
		int32 Result = 0;
		switch (SortMode)
		{
		case EDiurnalScheduleSortMode::Name:
			Result = Left.DisplayName.ToString().Compare(Right.DisplayName.ToString(), ESearchCase::IgnoreCase);
			break;
		case EDiurnalScheduleSortMode::Type:
			Result = CompareInt(static_cast<int32>(Left.Type), static_cast<int32>(Right.Type));
			if (Result == 0) Result = CompareInt(ChronologicalDay(Left), ChronologicalDay(Right));
			if (Result == 0) Result = CompareInt(Left.StartTime.ToSecondsIntoDay(), Right.StartTime.ToSecondsIntoDay());
			break;
		case EDiurnalScheduleSortMode::DayAndTime:
			Result = CompareInt(DayAndTimeGroup(Left), DayAndTimeGroup(Right));
			if (Result == 0) Result = CompareInt(ChronologicalDay(Left), ChronologicalDay(Right));
			if (Result == 0) Result = CompareInt(Left.StartTime.ToSecondsIntoDay(), Right.StartTime.ToSecondsIntoDay());
			break;
		case EDiurnalScheduleSortMode::TimeOfDay:
			Result = CompareInt(Left.StartTime.ToSecondsIntoDay(), Right.StartTime.ToSecondsIntoDay());
			if (Result == 0) Result = CompareInt(static_cast<int32>(Left.Type), static_cast<int32>(Right.Type));
			break;
		case EDiurnalScheduleSortMode::ManualOrder:
		default:
			Result = CompareInt(Left.ManualOrdinal, Right.ManualOrdinal);
			break;
		}
		if (Result == 0) Result = CompareInt(Left.ManualOrdinal, Right.ManualOrdinal);
		if (Result == 0) Result = CompareGuid(Left.EntryId, Right.EntryId);
		return Result;
	}
}

bool FDiurnalScheduleEditorFilter::IsActive() const
{
	return !SearchText.TrimStartAndEnd().IsEmpty()
		|| !bShowRepeatingEvents
		|| !bShowOnceEvents
		|| !bShowTimeRanges
		|| !bShowNotify
		|| !bShowBlocking;
}

void FDiurnalScheduleEditorFilter::Reset()
{
	SearchText.Reset();
	bShowRepeatingEvents = true;
	bShowOnceEvents = true;
	bShowTimeRanges = true;
	bShowNotify = true;
	bShowBlocking = true;
}

bool FDiurnalScheduleEditorFilter::MatchesEvent(
	const FDiurnalTimeEvent& Event,
	const FString& SourceName) const
{
	const FDiurnalRecurrence Recurrence = Event.Recurrence;
	if (Recurrence.Mode == EDiurnalRecurrenceMode::Once ? !bShowOnceEvents : !bShowRepeatingEvents) return false;
	if (Event.IsBlocking() ? !bShowBlocking : !bShowNotify) return false;
	const FString Searchable = FString::Printf(
		TEXT("%s event %s %s %s %s"),
		*Event.GetDisplayName().ToString(),
		Recurrence.Mode == EDiurnalRecurrenceMode::Once ? TEXT("once one-off event") : TEXT("repeating recurring every days event"),
		Event.IsBlocking() ? TEXT("blocking block time") : TEXT("notify notification"),
		*SourceName,
		*TagsSearchText(Event.EventTags));
	return MatchesTokens(SearchText, Searchable);
}

bool FDiurnalScheduleEditorFilter::MatchesRange(
	const FDiurnalTimeRange& Range,
	const FString& SourceName) const
{
	if (!bShowTimeRanges) return false;
	const FDiurnalRecurrence Recurrence = Range.Recurrence;
	const FString Searchable = FString::Printf(
		TEXT("%s time range %s active range active state %s %s"),
		*Range.GetDisplayName().ToString(),
		Recurrence.Mode == EDiurnalRecurrenceMode::Once ? TEXT("once one-off") : TEXT("repeating recurring every days"),
		*SourceName,
		*TagsSearchText(Range.RangeTags));
	return MatchesTokens(SearchText, Searchable);
}

FText DiurnalScheduleEditor::GetSortModeText(const EDiurnalScheduleSortMode SortMode)
{
	switch (SortMode)
	{
	case EDiurnalScheduleSortMode::Name: return LOCTEXT("SortName", "Name");
	case EDiurnalScheduleSortMode::Type: return LOCTEXT("SortType", "Type");
	case EDiurnalScheduleSortMode::DayAndTime: return LOCTEXT("SortDayTime", "Day and Time");
	case EDiurnalScheduleSortMode::TimeOfDay: return LOCTEXT("SortTime", "Time of Day");
	case EDiurnalScheduleSortMode::ManualOrder:
	default: return LOCTEXT("SortManual", "Manual Order");
	}
}

TArray<FDiurnalScheduleListItem> DiurnalScheduleEditor::BuildListItems(
	const UDiurnalSchedule& Schedule,
	const FDiurnalScheduleEditorFilter& Filter,
	const EDiurnalScheduleSortMode SortMode)
{
	TArray<FDiurnalScheduleListItem> Result;
	Result.Reserve(Schedule.TimeEvents.Num() + Schedule.TimeRanges.Num());
	const FString SourceName = Schedule.GetName();
	for (int32 Index = 0; Index < Schedule.TimeEvents.Num(); ++Index)
	{
		const FDiurnalTimeEvent& Event = Schedule.TimeEvents[Index];
		if (!Filter.MatchesEvent(Event, SourceName)) continue;
		FDiurnalScheduleListItem& Item = Result.AddDefaulted_GetRef();
		Item.Type = EDiurnalScheduleSelectionType::Event;
		Item.EntryId = Event.EntryId;
		Item.ManualIndex = Index;
		Item.ManualOrdinal = Index;
		Item.DisplayName = Event.GetDisplayName();
		Item.Tags = Event.EventTags;
		Item.StartTime = Event.TimeOfDay;
		Item.AnchorDay = Event.Recurrence.AnchorDay;
		Item.bOneOff = Event.Recurrence.Mode == EDiurnalRecurrenceMode::Once;
		Item.bBlocking = Event.IsBlocking();
	}
	for (int32 Index = 0; Index < Schedule.TimeRanges.Num(); ++Index)
	{
		const FDiurnalTimeRange& Range = Schedule.TimeRanges[Index];
		if (!Filter.MatchesRange(Range, SourceName)) continue;
		FDiurnalScheduleListItem& Item = Result.AddDefaulted_GetRef();
		Item.Type = EDiurnalScheduleSelectionType::Range;
		Item.EntryId = Range.EntryId;
		Item.ManualIndex = Index;
		Item.ManualOrdinal = Schedule.TimeEvents.Num() + Index;
		Item.DisplayName = Range.GetDisplayName();
		Item.Tags = Range.RangeTags;
		Item.StartTime = Range.StartTime;
		Item.AnchorDay = Range.Recurrence.AnchorDay;
		Item.bOneOff = Range.Recurrence.Mode == EDiurnalRecurrenceMode::Once;
	}
	Result.StableSort([SortMode](const FDiurnalScheduleListItem& Left, const FDiurnalScheduleListItem& Right)
	{
		return CompareItems(Left, Right, SortMode) < 0;
	});
	return Result;
}

#undef LOCTEXT_NAMESPACE
