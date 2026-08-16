#pragma once

#include "CoreMinimal.h"
#include "ScheduleEditor/DiurnalScheduleEditorModel.h"
#include "Widgets/SCompoundWidget.h"

class FUICommandList;
struct FDiurnalScheduleOverviewItem;

/** Focused list-mode authoring surface shared with the Week-capable workspace. */
class DIURNALCYCLEEDITOR_API SDiurnalScheduleOverview final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDiurnalScheduleOverview) {}
		SLATE_ARGUMENT(TSharedPtr<FDiurnalScheduleEditorModel>, Model)
		SLATE_ARGUMENT(TSharedPtr<FUICommandList>, Commands)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);
	void RequestRenameSelected();
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	void Refresh();
	TSharedRef<ITableRow> GenerateRow(TSharedPtr<FDiurnalScheduleOverviewItem> Item, const TSharedRef<STableViewBase>& Owner);
	void SelectionChanged(TSharedPtr<FDiurnalScheduleOverviewItem> Item, ESelectInfo::Type SelectInfo);
	void ItemScrolledIntoView(TSharedPtr<FDiurnalScheduleOverviewItem> Item, const TSharedPtr<ITableRow>& Row);
	TSharedPtr<SWidget> BuildContextMenu() const;
	FReply HandleKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) const;
	bool VerifyName(const FText& Text, FText& OutError) const;
	void CommitName(const FText& Text, ETextCommit::Type CommitType, TSharedPtr<FDiurnalScheduleOverviewItem> Item);
	FText GetSummary() const;
	FText GetEmptyTitle() const;
	FText GetEmptyDescription() const;
	EVisibility GetEmptyVisibility() const;
	EVisibility GetNoEntriesActionsVisibility() const;
	EVisibility GetFilteredActionsVisibility() const;
	FReply AddEventFromEmpty();
	FReply AddRangeFromEmpty();
	FReply ClearFilters();

	TSharedPtr<FDiurnalScheduleEditorModel> Model;
	TSharedPtr<FUICommandList> Commands;
	TArray<TSharedPtr<FDiurnalScheduleOverviewItem>> Items;
	TSharedPtr<SListView<TSharedPtr<FDiurnalScheduleOverviewItem>>> ListView;
	EDiurnalScheduleSelectionType PendingRenameType = EDiurnalScheduleSelectionType::None;
	FGuid PendingRenameId;
};
