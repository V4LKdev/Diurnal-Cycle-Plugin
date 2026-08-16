#pragma once

#include "CoreMinimal.h"
#include "ScheduleEditor/DiurnalScheduleEditorViewState.h"
#include "Widgets/SCompoundWidget.h"

class FDiurnalScheduleEditorModel;
class FUICommandList;
class SDiurnalScheduleOverview;
class SDiurnalScheduleWeekView;
class SSearchBox;
class SWidgetSwitcher;
struct FPropertyChangedEvent;

enum class EDiurnalScheduleEditorViewMode : uint8
{
	List,
	Timeline
};

/** Shared controls and the implemented List/Timeline authoring projections. */
class DIURNALCYCLEEDITOR_API SDiurnalScheduleWorkspace final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDiurnalScheduleWorkspace) {}
		SLATE_ARGUMENT(TSharedPtr<FDiurnalScheduleEditorModel>, Model)
		SLATE_ARGUMENT(TSharedPtr<FUICommandList>, Commands)
	SLATE_END_ARGS()

	virtual ~SDiurnalScheduleWorkspace() override;
	void Construct(const FArguments& Args);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	void RequestRenameSelected();
	void FocusEntry(EDiurnalScheduleSelectionType Type, FGuid EntryId);
	void SetViewMode(EDiurnalScheduleEditorViewMode InViewMode);
	void ClearSelectionForCurrentView();
	EDiurnalScheduleEditorViewMode GetViewMode() const { return ViewMode; }

private:
	enum class EFilterOption : uint8
	{
		Repeating,
		Once,
		Range,
		Notify,
		Blocking
	};

	TSharedRef<SWidget> BuildSortMenu();
	TSharedRef<SWidget> BuildFilterMenu();
	void SetSearchText(const FText& Text);
	void SetSortMode(EDiurnalScheduleSortMode SortMode);
	void ToggleFilter(EFilterOption Option);
	bool IsFilterEnabled(EFilterOption Option) const;
	FReply ClearFilters();
	FText GetSortText() const;
	FText GetFilterText() const;
	ECheckBoxState IsViewChecked(EDiurnalScheduleEditorViewMode Mode) const;
	EVisibility GetListControlsVisibility() const;
	EVisibility GetTimelineControlsVisibility() const;
	FReply CurrentTimeline();
	void SetHourHeight(float Value);
	float GetHourHeight() const;
	FReply SetVisibleDayPreset(int32 Days);
	FReply ResetTimelineView();
	FText GetTimelineRangeText() const;
	FText GetRuntimeMarkerText() const;
	void NormalizeSelectionForView();
	void HandleSettingsChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);

	TSharedPtr<FDiurnalScheduleEditorModel> Model;
	TSharedPtr<SDiurnalScheduleOverview> ListView;
	TSharedPtr<SDiurnalScheduleWeekView> WeekView;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<SWidgetSwitcher> ViewSwitcher;
	EDiurnalScheduleEditorViewMode ViewMode = EDiurnalScheduleEditorViewMode::Timeline;
	FDelegateHandle SettingsChangedHandle;
};
