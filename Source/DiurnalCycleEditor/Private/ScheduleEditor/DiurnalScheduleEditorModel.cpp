#include "ScheduleEditor/DiurnalScheduleEditorModel.h"

#include "DiurnalSchedule.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "DiurnalScheduleEditorModel"

FDiurnalScheduleEditorModel::FDiurnalScheduleEditorModel(UDiurnalSchedule* InSchedule)
	: Schedule(InSchedule)
{
	check(IsValid(InSchedule));
}

void FDiurnalScheduleEditorModel::SelectEntry(const EDiurnalScheduleSelectionType Type, const FGuid Id)
{
	if (SelectionType == Type && SelectedId == Id && SelectedDay == INDEX_NONE) return;
	SelectionType = Type;
	SelectedId = Id;
	SelectedDay = INDEX_NONE;
	Changed.Broadcast();
}

void FDiurnalScheduleEditorModel::SelectEntryOccurrence(const EDiurnalScheduleSelectionType Type, const FGuid Id, const int32 Day)
{
	const int32 OccurrenceDay = FMath::Max(1, Day);
	if (SelectionType == Type && SelectedId == Id && SelectedDay == OccurrenceDay) return;
	SelectionType = Type;
	SelectedId = Id;
	SelectedDay = OccurrenceDay;
	Changed.Broadcast();
}

void FDiurnalScheduleEditorModel::ClearEntrySelection()
{
	if (SelectionType == EDiurnalScheduleSelectionType::None && !SelectedId.IsValid()) return;
	SelectionType = EDiurnalScheduleSelectionType::None;
	SelectedId.Invalidate();
	Changed.Broadcast();
}

void FDiurnalScheduleEditorModel::SelectDay(const int32 Day)
{
	const int32 NewDay = FMath::Max(1, Day);
	const bool bChanged = SelectedDay != NewDay || SelectionType != EDiurnalScheduleSelectionType::None;
	SelectedDay = NewDay;
	SelectionType = EDiurnalScheduleSelectionType::None;
	SelectedId.Invalidate();
	if (bChanged) Changed.Broadcast();
}

void FDiurnalScheduleEditorModel::ClearAllSelection()
{
	const bool bChanged = SelectedDay != INDEX_NONE || SelectionType != EDiurnalScheduleSelectionType::None || SelectedId.IsValid();
	SelectedDay = INDEX_NONE;
	SelectionType = EDiurnalScheduleSelectionType::None;
	SelectedId.Invalidate();
	if (bChanged) Changed.Broadcast();
}

FGuid FDiurnalScheduleEditorModel::AddRepeatingEvent()
{
	UDiurnalSchedule* Asset = Schedule.Get();
	if (!Asset)
	{
		return {};
	}

	const FScopedTransaction Transaction(LOCTEXT("AddEvent", "Add Schedule Event"));
	Asset->Modify();
	FDiurnalTimeEvent& Event = Asset->TimeEvents.AddDefaulted_GetRef();
	Event.EntryId = FGuid::NewGuid();
	Event.Recurrence = FDiurnalRecurrence::Repeating();
	SelectionType = EDiurnalScheduleSelectionType::Event;
	SelectedId = Event.EntryId;
	SelectedDay = INDEX_NONE;
	Asset->MarkPackageDirty();
	Asset->PostEditChange();
	NotifyChanged();
	return SelectedId;
}

FGuid FDiurnalScheduleEditorModel::AddRange()
{
	UDiurnalSchedule* Asset = Schedule.Get();
	if (!Asset)
	{
		return {};
	}

	const FScopedTransaction Transaction(LOCTEXT("AddRange", "Add Schedule Range"));
	Asset->Modify();
	FDiurnalTimeRange& Range = Asset->TimeRanges.AddDefaulted_GetRef();
	Range.EntryId = FGuid::NewGuid();
	Range.EndTime = FDiurnalTimeOfDay(1);
	Range.Recurrence = FDiurnalRecurrence::Repeating();
	SelectionType = EDiurnalScheduleSelectionType::Range;
	SelectedId = Range.EntryId;
	SelectedDay = INDEX_NONE;
	Asset->MarkPackageDirty();
	Asset->PostEditChange();
	NotifyChanged();
	return SelectedId;
}

FGuid FDiurnalScheduleEditorModel::AddOnceEventAt(const int32 Day, const FDiurnalTimeOfDay& TimeOfDay)
{
	UDiurnalSchedule* Asset = Schedule.Get();
	if (!Asset || Day < 1 || !TimeOfDay.IsValid()) return {};
	const FScopedTransaction Transaction(LOCTEXT("AddOnceEventAt", "Add One-off Schedule Event"));
	Asset->Modify();
	FDiurnalTimeEvent& Event = Asset->TimeEvents.AddDefaulted_GetRef();
	Event.EntryId = FGuid::NewGuid();
	Event.Recurrence = FDiurnalRecurrence::Once(Day);
	Event.TimeOfDay = TimeOfDay;
	Asset->RepairEntries();
	SelectionType = EDiurnalScheduleSelectionType::Event;
	SelectedId = Event.EntryId;
	SelectedDay = Day;
	Asset->MarkPackageDirty();
	Asset->PostEditChange();
	NotifyChanged();
	return SelectedId;
}

FGuid FDiurnalScheduleEditorModel::AddRangeAt(const FDiurnalTimeOfDay& StartTime, const int32 DurationMinutes, const int32 AnchorDay)
{
	UDiurnalSchedule* Asset = Schedule.Get();
	if (!Asset || !StartTime.IsValid() || DurationMinutes <= 0 || DurationMinutes >= 24 * 60) return {};
	const int32 StartSecond = StartTime.ToSecondsIntoDay();
	const int32 EndSecond = (StartSecond + DurationMinutes * 60) % static_cast<int32>(DiurnalCycle::GSecondsPerDay);
	const FScopedTransaction Transaction(LOCTEXT("AddRangeAt", "Add Schedule Time Range"));
	Asset->Modify();
	FDiurnalTimeRange& Range = Asset->TimeRanges.AddDefaulted_GetRef();
	Range.EntryId = FGuid::NewGuid();
	Range.StartTime = StartTime;
	Range.EndTime = FDiurnalTimeOfDay(EndSecond / 3600, (EndSecond / 60) % 60, EndSecond % 60);
	Range.Recurrence = AnchorDay > 0 ? FDiurnalRecurrence::Once(AnchorDay) : FDiurnalRecurrence::Repeating();
	Asset->RepairEntries();
	SelectionType = EDiurnalScheduleSelectionType::Range;
	SelectedId = Range.EntryId;
	SelectedDay = AnchorDay > 0 ? AnchorDay : INDEX_NONE;
	Asset->MarkPackageDirty();
	Asset->PostEditChange();
	NotifyChanged();
	return SelectedId;
}

int32 FDiurnalScheduleEditorModel::FindSelectedIndex() const
{
	const UDiurnalSchedule* Asset = Schedule.Get();
	if (!Asset)
	{
		return INDEX_NONE;
	}
	if (SelectionType == EDiurnalScheduleSelectionType::Event)
	{
		return Asset->TimeEvents.IndexOfByPredicate([this](const FDiurnalTimeEvent& Value)
		{
			return Value.EntryId == SelectedId;
		});
	}
	if (SelectionType == EDiurnalScheduleSelectionType::Range)
	{
		return Asset->TimeRanges.IndexOfByPredicate([this](const FDiurnalTimeRange& Value)
		{
			return Value.EntryId == SelectedId;
		});
	}
	return INDEX_NONE;
}

bool FDiurnalScheduleEditorModel::DuplicateSelected()
{
	UDiurnalSchedule* Asset = Schedule.Get();
	const int32 Index = FindSelectedIndex();
	if (!Asset || Index == INDEX_NONE)
	{
		return false;
	}

	const FScopedTransaction Transaction(LOCTEXT("Duplicate", "Duplicate Schedule Entry"));
	Asset->Modify();
	if (SelectionType == EDiurnalScheduleSelectionType::Event)
	{
		FDiurnalTimeEvent Copy = Asset->TimeEvents[Index];
		Copy.EntryId = FGuid::NewGuid();
		Copy.EventName = MakeUniqueCopyName(Copy.GetDisplayName());
		Asset->TimeEvents.Insert(Copy, Index + 1);
		SelectedId = Copy.EntryId;
	}
	else
	{
		FDiurnalTimeRange Copy = Asset->TimeRanges[Index];
		Copy.EntryId = FGuid::NewGuid();
		Copy.RangeName = MakeUniqueCopyName(Copy.GetDisplayName());
		Asset->TimeRanges.Insert(Copy, Index + 1);
		SelectedId = Copy.EntryId;
	}
	Asset->MarkPackageDirty();
	Asset->PostEditChange();
	NotifyChanged();
	return true;
}

bool FDiurnalScheduleEditorModel::DeleteSelected()
{
	UDiurnalSchedule* Asset = Schedule.Get();
	const int32 Index = FindSelectedIndex();
	if (!Asset || Index == INDEX_NONE)
	{
		return false;
	}

	const FScopedTransaction Transaction(LOCTEXT("Delete", "Delete Schedule Entry"));
	Asset->Modify();
	if (SelectionType == EDiurnalScheduleSelectionType::Event)
	{
		Asset->TimeEvents.RemoveAt(Index);
	}
	else
	{
		Asset->TimeRanges.RemoveAt(Index);
	}
	SelectionType = EDiurnalScheduleSelectionType::None;
	SelectedId.Invalidate();
	Asset->MarkPackageDirty();
	Asset->PostEditChange();
	NotifyChanged();
	return true;
}

bool FDiurnalScheduleEditorModel::MoveSelected(const int32 Direction)
{
	if (!IsManualOrder()) return false;
	UDiurnalSchedule* Asset = Schedule.Get(); const int32 Index = FindSelectedIndex(); if (!Asset || Index == INDEX_NONE || Direction == 0) return false;
	const int32 Count = SelectionType == EDiurnalScheduleSelectionType::Event ? Asset->TimeEvents.Num() : Asset->TimeRanges.Num();
	const int32 Destination = FMath::Clamp(Index + FMath::Sign(Direction), 0, Count - 1); if (Destination == Index) return false;
	const FScopedTransaction Transaction(LOCTEXT("Reorder", "Reorder Schedule Entry")); Asset->Modify();
	if (SelectionType == EDiurnalScheduleSelectionType::Event) Asset->TimeEvents.Swap(Index, Destination); else Asset->TimeRanges.Swap(Index, Destination);
	Asset->MarkPackageDirty(); Asset->PostEditChange(); NotifyChanged(); return true;
}

bool FDiurnalScheduleEditorModel::RenameSelected(const FName NewName)
{
	UDiurnalSchedule* Asset = Schedule.Get(); const int32 Index = FindSelectedIndex();
	if (!Asset || Index == INDEX_NONE || NewName.IsNone()) return false;
	const FName ExistingName = SelectionType == EDiurnalScheduleSelectionType::Event ? Asset->TimeEvents[Index].EventName : Asset->TimeRanges[Index].RangeName;
	if (ExistingName == NewName) return true;
	const FScopedTransaction Transaction(LOCTEXT("Rename", "Rename Schedule Entry")); Asset->Modify();
	if (SelectionType == EDiurnalScheduleSelectionType::Event) Asset->TimeEvents[Index].EventName = NewName;
	else Asset->TimeRanges[Index].RangeName = NewName;
	Asset->MarkPackageDirty(); Asset->PostEditChange(); NotifyChanged(); return true;
}

bool FDiurnalScheduleEditorModel::CanMoveSelected(const int32 Direction) const
{
	if (!IsManualOrder()) return false;
	const UDiurnalSchedule* Asset = Schedule.Get(); const int32 Index = FindSelectedIndex();
	if (!Asset || Index == INDEX_NONE || Direction == 0) return false;
	const int32 Count = SelectionType == EDiurnalScheduleSelectionType::Event ? Asset->TimeEvents.Num() : Asset->TimeRanges.Num();
	return Index + FMath::Sign(Direction) >= 0 && Index + FMath::Sign(Direction) < Count;
}

void FDiurnalScheduleEditorModel::SetSortMode(const EDiurnalScheduleSortMode InSortMode)
{
	if (SortMode == InSortMode) return;
	SortMode = InSortMode;
	Changed.Broadcast();
}

void FDiurnalScheduleEditorModel::SetFilter(const FDiurnalScheduleEditorFilter& InFilter)
{
	Filter = InFilter;
	Changed.Broadcast();
}

void FDiurnalScheduleEditorModel::ClearFilter()
{
	if (!Filter.IsActive()) return;
	Filter.Reset();
	Changed.Broadcast();
}

FName FDiurnalScheduleEditorModel::MakeUniqueCopyName(const FName SourceName) const
{
	const UDiurnalSchedule* Asset = Schedule.Get();
	if (!Asset) return FName(TEXT("Copy"));
	TSet<FName> Names;
	if (SelectionType == EDiurnalScheduleSelectionType::Event) for (const FDiurnalTimeEvent& Event : Asset->TimeEvents) Names.Add(Event.EventName);
	else for (const FDiurnalTimeRange& Range : Asset->TimeRanges) Names.Add(Range.RangeName);
	const FString Base = FString::Printf(TEXT("%s Copy"), *SourceName.ToString());
	FName Candidate(*Base);
	for (int32 Suffix = 2; Names.Contains(Candidate); ++Suffix) Candidate = FName(*FString::Printf(TEXT("%s %d"), *Base, Suffix));
	return Candidate;
}

void FDiurnalScheduleEditorModel::HandleUndoRedo()
{
	if (FindSelectedIndex() == INDEX_NONE) { SelectionType = EDiurnalScheduleSelectionType::None; SelectedId.Invalidate(); }
	NotifyChanged();
}

void FDiurnalScheduleEditorModel::NotifyInteractiveValueChanged()
{
	VisualChanged.Broadcast();
}

void FDiurnalScheduleEditorModel::NotifyChanged() { if (UDiurnalSchedule* Asset = Schedule.Get()) Asset->RepairEntries(); Changed.Broadcast(); }

#undef LOCTEXT_NAMESPACE
