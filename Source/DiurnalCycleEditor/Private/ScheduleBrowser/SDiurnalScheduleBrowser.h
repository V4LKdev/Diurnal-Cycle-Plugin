#pragma once

#include "CoreMinimal.h"
#include "ScheduleEditor/DiurnalScheduleEditorViewState.h"
#include "Widgets/SCompoundWidget.h"

struct FDiurnalScheduleBrowserItem;
struct FAssetData;

/** Read-only merged view of the project's authored default schedule layers. */
class DIURNALCYCLEEDITOR_API SDiurnalScheduleBrowser final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDiurnalScheduleBrowser) {}
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual ~SDiurnalScheduleBrowser() override;

	static FText GetEventTypeLabel(bool bOneOff);
	static FText GetEmptyTagsLabel();
	static constexpr int32 MaximumVisibleTagChips = 2;
	bool IsConfigurationInvalid() const { return bInvalidConfiguration; }

private:
	void Rebuild();
	void SetSearchText(const FText& Text);
	void SetFilter(ECheckBoxState State, FName Filter);
	TSharedRef<SWidget> BuildSortMenu();
	void SetSortMode(EDiurnalScheduleSortMode InSortMode);
	FText GetSortText() const;
	void SelectionChanged(TSharedPtr<FDiurnalScheduleBrowserItem> Item, ESelectInfo::Type SelectInfo);
	void DoubleClick(TSharedPtr<FDiurnalScheduleBrowserItem> Item);
	FReply OpenSelectedSource();
	FReply RefreshBrowser();
	bool CanOpenSelectedSource() const;
	FText GetSourceStateText() const;
	FText GetSelectionSummary() const;
	FText GetEmptyText() const;
	EVisibility GetEmptyVisibility() const;
	void HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);
	void HandleObjectsReplaced(const TMap<UObject*, UObject*>& Replacements);
	void HandleScheduleAssetChanged(const FAssetData& AssetData);
	void HandleScheduleAssetRenamed(const FAssetData& AssetData, const FString& OldObjectPath);
	bool IsRelevantSchedulePath(const FSoftObjectPath& ObjectPath) const;

	FText SearchText;
	TArray<TSharedPtr<FDiurnalScheduleBrowserItem>> AllItems;
	TArray<TSharedPtr<FDiurnalScheduleBrowserItem>> FilteredItems;
	TSharedPtr<SListView<TSharedPtr<FDiurnalScheduleBrowserItem>>> ListView;
	TSharedPtr<FDiurnalScheduleBrowserItem> SelectedItem;
	bool bShowRepeating = true;
	bool bShowOneOff = true;
	bool bShowRanges = true;
	bool bShowNotify = true;
	bool bShowBlocking = true;
	bool bInvalidConfiguration = false;
	EDiurnalScheduleSortMode SortMode = EDiurnalScheduleSortMode::DayAndTime;
	FDelegateHandle SettingsChangedHandle;
	FDelegateHandle ObjectsReplacedHandle;
	FDelegateHandle AssetRemovedHandle;
	FDelegateHandle AssetRenamedHandle;
	FDelegateHandle AssetUpdatedHandle;
};
