#include "DiurnalSchedule.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace
{
	FName MakeUniqueDefaultName(const TCHAR* BaseLabel, const TSet<FName>& UsedNames)
	{
		const FName BaseName(BaseLabel);
		if (!UsedNames.Contains(BaseName)) return BaseName;
		for (int32 Suffix = 2; Suffix < MAX_int32; ++Suffix)
		{
			const FName Candidate(*FString::Printf(TEXT("%s %d"), BaseLabel, Suffix));
			if (!UsedNames.Contains(Candidate)) return Candidate;
		}
		return BaseName;
	}
}

bool UDiurnalSchedule::RepairEntries()
{
	bool bChanged = false;
	TSet<FGuid> SeenIds;
	TSet<FName> UsedNames;

	for (FDiurnalTimeEvent& Event : TimeEvents)
	{
		if (!Event.EntryId.IsValid() || SeenIds.Contains(Event.EntryId))
		{
			Event.EntryId = FGuid::NewGuid();
			bChanged = true;
		}
		SeenIds.Add(Event.EntryId);

		if (Event.EventName.IsNone())
		{
			const FGameplayTag Tag = Event.GetPrimaryTag();
			Event.EventName = Tag.IsValid()
				? Tag.GetTagLeafName()
				: MakeUniqueDefaultName(TEXT("New Event"), UsedNames);
			bChanged = true;
		}
		UsedNames.Add(Event.EventName);
	}

	UsedNames.Reset();
	for (FDiurnalTimeRange& Range : TimeRanges)
	{
		if (!Range.EntryId.IsValid() || SeenIds.Contains(Range.EntryId))
		{
			Range.EntryId = FGuid::NewGuid();
			bChanged = true;
		}
		SeenIds.Add(Range.EntryId);

		if (Range.RangeName.IsNone())
		{
			const FGameplayTag Tag = Range.GetPrimaryTag();
			Range.RangeName = Tag.IsValid()
				? Tag.GetTagLeafName()
				: MakeUniqueDefaultName(TEXT("New Range"), UsedNames);
			bChanged = true;
		}
		UsedNames.Add(Range.RangeName);
	}

	return bChanged;
}

void UDiurnalSchedule::GetValidationIssues(TArray<FDiurnalScheduleValidationIssue>& OutIssues) const
{
	OutIssues.Reset();
	TSet<FGuid> EntryIds;
	TSet<FName> EventNames;
	TSet<FName> RangeNames;
	const auto Add = [&OutIssues](const EDiurnalScheduleIssueSeverity Severity,
		const EDiurnalScheduleIssueEntryType Type, const FGuid EntryId, const FText& Message)
	{
		FDiurnalScheduleValidationIssue& Issue = OutIssues.AddDefaulted_GetRef();
		Issue.Severity = Severity;
		Issue.EntryType = Type;
		Issue.EntryId = EntryId;
		Issue.Message = Message;
	};

	for (int32 Index = 0; Index < TimeEvents.Num(); ++Index)
	{
		const FDiurnalTimeEvent& Event = TimeEvents[Index];
		const FText Name = FText::FromName(Event.GetDisplayName());
		if (Event.EventName.IsNone())
		{
			Add(EDiurnalScheduleIssueSeverity::Error, EDiurnalScheduleIssueEntryType::Event, Event.EntryId,
				FText::Format(NSLOCTEXT("DiurnalSchedule", "MissingEventName", "Event [{0}] has no display name."), FText::AsNumber(Index)));
		}
		else if (EventNames.Contains(Event.EventName))
		{
			Add(EDiurnalScheduleIssueSeverity::Warning, EDiurnalScheduleIssueEntryType::Event, Event.EntryId,
				FText::Format(NSLOCTEXT("DiurnalSchedule", "DuplicateEventName", "Event display name '{0}' is duplicated. Names are presentation-only."), Name));
		}
		EventNames.Add(Event.EventName);
		if (!Event.IsValid())
		{
			Add(EDiurnalScheduleIssueSeverity::Error, EDiurnalScheduleIssueEntryType::Event, Event.EntryId,
				FText::Format(NSLOCTEXT("DiurnalSchedule", "InvalidEvent", "Event '{0}' has invalid timing or recurrence data."), Name));
		}
		if (!Event.EntryId.IsValid() || EntryIds.Contains(Event.EntryId))
		{
			Add(EDiurnalScheduleIssueSeverity::Error, EDiurnalScheduleIssueEntryType::Event, Event.EntryId,
				FText::Format(NSLOCTEXT("DiurnalSchedule", "InvalidEventId", "Event '{0}' has an invalid or duplicated internal identity."), Name));
		}
		EntryIds.Add(Event.EntryId);
	}

	for (int32 Index = 0; Index < TimeRanges.Num(); ++Index)
	{
		const FDiurnalTimeRange& Range = TimeRanges[Index];
		const FText Name = FText::FromName(Range.GetDisplayName());
		if (Range.RangeName.IsNone())
		{
			Add(EDiurnalScheduleIssueSeverity::Error, EDiurnalScheduleIssueEntryType::Range, Range.EntryId,
				FText::Format(NSLOCTEXT("DiurnalSchedule", "MissingRangeName", "Time range [{0}] has no display name."), FText::AsNumber(Index)));
		}
		else if (RangeNames.Contains(Range.RangeName))
		{
			Add(EDiurnalScheduleIssueSeverity::Warning, EDiurnalScheduleIssueEntryType::Range, Range.EntryId,
				FText::Format(NSLOCTEXT("DiurnalSchedule", "DuplicateRangeName", "Time range display name '{0}' is duplicated. Names are presentation-only."), Name));
		}
		RangeNames.Add(Range.RangeName);
		if (!Range.IsValid())
		{
			Add(EDiurnalScheduleIssueSeverity::Error, EDiurnalScheduleIssueEntryType::Range, Range.EntryId,
				FText::Format(NSLOCTEXT("DiurnalSchedule", "InvalidRange", "Time range '{0}' has invalid timing or recurrence data."), Name));
		}
		if (!Range.EntryId.IsValid() || EntryIds.Contains(Range.EntryId))
		{
			Add(EDiurnalScheduleIssueSeverity::Error, EDiurnalScheduleIssueEntryType::Range, Range.EntryId,
				FText::Format(NSLOCTEXT("DiurnalSchedule", "InvalidRangeId", "Time range '{0}' has an invalid or duplicated internal identity."), Name));
		}
		EntryIds.Add(Range.EntryId);
	}
}

#if WITH_EDITOR

void UDiurnalSchedule::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	RepairEntries();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UDiurnalSchedule::PostEditUndo()
{
	Super::PostEditUndo();
	RepairEntries();
}

EDataValidationResult UDiurnalSchedule::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult SuperResult = Super::IsDataValid(Context);
	bool bValid = SuperResult != EDataValidationResult::Invalid;
	TArray<FDiurnalScheduleValidationIssue> Issues;
	GetValidationIssues(Issues);
	for (const FDiurnalScheduleValidationIssue& Issue : Issues)
	{
		if (Issue.Severity == EDiurnalScheduleIssueSeverity::Error)
		{
			Context.AddError(Issue.Message);
			bValid = false;
		}
		else
		{
			Context.AddWarning(Issue.Message);
		}
	}

	return bValid ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}

#endif
