#pragma once

#include "CoreMinimal.h"
#include "ScheduleEditor/DiurnalScheduleEditorViewState.h"

class UDiurnalSchedule;

class DIURNALCYCLEEDITOR_API FDiurnalScheduleEditorModel final : public TSharedFromThis<FDiurnalScheduleEditorModel>
{
public:
	explicit FDiurnalScheduleEditorModel(UDiurnalSchedule* InSchedule);
	UDiurnalSchedule* GetSchedule() const { return Schedule.Get(); }
	EDiurnalScheduleSelectionType GetSelectionType() const { return SelectionType; }
	FGuid GetSelectedId() const { return SelectedId; }
	void SelectEntry(EDiurnalScheduleSelectionType Type, FGuid Id);
	void SelectEntryOccurrence(EDiurnalScheduleSelectionType Type, FGuid Id, int32 Day);
	void ClearEntrySelection();
	void SelectDay(int32 Day);
	void ClearAllSelection();
	int32 GetSelectedDay() const { return SelectedDay; }
	FGuid AddRepeatingEvent();
	FGuid AddRange();
	FGuid AddOnceEventAt(int32 Day, const FDiurnalTimeOfDay& TimeOfDay);
	/** Creates a range at a time. AnchorDay > 0 creates a one-off range; zero creates a repeating range. */
	FGuid AddRangeAt(const FDiurnalTimeOfDay& StartTime, int32 DurationMinutes = 60, int32 AnchorDay = 0);
	bool DuplicateSelected();
	bool DeleteSelected();
	bool MoveSelected(int32 Direction);
	bool RenameSelected(FName NewName);
	bool CanMoveSelected(int32 Direction) const;
	EDiurnalScheduleSortMode GetSortMode() const { return SortMode; }
	void SetSortMode(EDiurnalScheduleSortMode InSortMode);
	bool IsManualOrder() const { return SortMode == EDiurnalScheduleSortMode::ManualOrder; }
	const FDiurnalScheduleEditorFilter& GetFilter() const { return Filter; }
	void SetFilter(const FDiurnalScheduleEditorFilter& InFilter);
	void ClearFilter();
	void HandleUndoRedo();
	/** Repaint/reproject values without reconstructing the property inspector. */
	void NotifyInteractiveValueChanged();
	FSimpleMulticastDelegate& OnChanged() { return Changed; }
	FSimpleMulticastDelegate& OnVisualChanged() { return VisualChanged; }
private:
	int32 FindSelectedIndex() const;
	FName MakeUniqueCopyName(FName SourceName) const;
	void NotifyChanged();
	TWeakObjectPtr<UDiurnalSchedule> Schedule;
	EDiurnalScheduleSelectionType SelectionType = EDiurnalScheduleSelectionType::None;
	FGuid SelectedId;
	int32 SelectedDay = INDEX_NONE;
	EDiurnalScheduleSortMode SortMode = EDiurnalScheduleSortMode::ManualOrder;
	FDiurnalScheduleEditorFilter Filter;
	FSimpleMulticastDelegate Changed;
	FSimpleMulticastDelegate VisualChanged;
};
