#pragma once

#include "CoreMinimal.h"
#include "ScheduleEditor/DiurnalScheduleEditorViewState.h"
#include "Widgets/SCompoundWidget.h"

class FDiurnalScheduleEditorModel;
class FMenuBuilder;
class SBox;
class SHorizontalBox;
class SInlineEditableTextBlock;
class UDiurnalSchedule;
class UPackage;
template<typename FilterType> class SBasicFilterBar;
struct FAssetData;
struct FDiurnalScheduleBrowserItem;

enum class EDiurnalScheduleBrowserMode : uint8
{
	CombinedSchedule,
	ScheduleAssets
};

/** Authored schedule browser with a combined-entry view and an all-assets view. */
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
	void SetMode(ECheckBoxState State, EDiurnalScheduleBrowserMode NewMode);
	ECheckBoxState IsModeChecked(EDiurnalScheduleBrowserMode TestMode) const;
	void RebuildBody();
	TSharedRef<SWidget> BuildBody();
	TSharedRef<SWidget> BuildEntryPanel();
	TSharedRef<SWidget> BuildAssetPicker();

	void Rebuild();
	void PopulateSchedule(UDiurnalSchedule& Schedule, int32 SourceLayer);
	void SetSearchText(const FText& Text);
	void HandleEntryFilterChanged();
	EVisibility GetEntryFilterVisibility() const;
	EVisibility GetDefaultFilterVisibility() const;
	TSharedRef<SWidget> BuildSortMenu();
	void SetSortMode(EDiurnalScheduleSortMode InSortMode);
	FText GetSortText() const;

	TSharedRef<ITableRow> GenerateRow(
		TSharedPtr<FDiurnalScheduleBrowserItem> Item,
		const TSharedRef<STableViewBase>& Owner);
	void SelectionChanged(TSharedPtr<FDiurnalScheduleBrowserItem> Item, ESelectInfo::Type SelectInfo);
	void ItemScrolledIntoView(TSharedPtr<FDiurnalScheduleBrowserItem> Item, const TSharedPtr<ITableRow>& Row);
	void DoubleClick(TSharedPtr<FDiurnalScheduleBrowserItem> Item);
	TSharedPtr<SWidget> BuildContextMenu();
	void PopulateAddTargetMenu(FMenuBuilder& Menu, bool bRange);
	FReply HandleListKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent);
	bool VerifyName(const FText& Text, FText& OutError) const;
	void CommitName(const FText& Text, ETextCommit::Type CommitType, TSharedPtr<FDiurnalScheduleBrowserItem> Item);
	void RequestRenameSelected();
	void RebuildInspector();
	void SelectExact(UDiurnalSchedule* Source, FGuid EntryId, bool bRequestRename = false);

	void AssetSelected(const FAssetData& AssetData);
	void AssetDoubleClicked(const FAssetData& AssetData);
	void ExtendAssetPickerTopBar(TSharedRef<SHorizontalBox> TopBar);
	TSharedPtr<SWidget> BuildAssetContextMenu(const TArray<FAssetData>& SelectedAssets);
	FReply CreateScheduleAssetFromPicker();
	UDiurnalSchedule* CreateScheduleAsset();
	void RenameSelectedScheduleAsset();
	void DuplicateScheduleAsset(FAssetData SourceAsset);
	void DeleteScheduleAssets(TArray<FAssetData> Assets);
	void OpenScheduleAssets(TArray<FAssetData> Assets);
	UDiurnalSchedule* GetEditingSchedule() const;
	TSharedPtr<FDiurnalScheduleEditorModel> MakeEditingModel() const;
	void AddEntryToSchedule(UDiurnalSchedule* Schedule, bool bRange);
	void CreateDefaultScheduleWithEntry(bool bRange);

	FReply DuplicateSelected();
	FReply DeleteSelected();
	FReply OpenSelectedSource();
	FReply SaveSchedules();
	FReply RefreshBrowser();
	FReply OpenProjectSettings();
	bool CanSaveSchedules() const;
	void GatherDirtySchedulePackages(TArray<UPackage*>& OutPackages) const;
	bool CanEditSelected() const;
	bool CanOpenSelectedSource() const;

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
	TSharedPtr<SBasicFilterBar<TSharedPtr<FDiurnalScheduleBrowserItem>>> EntryFilterBar;
	TSharedPtr<SWidget> AssetPickerWidget;
	TSharedPtr<FDiurnalScheduleBrowserItem> SelectedItem;
	TSharedPtr<FDiurnalScheduleEditorModel> InspectorModel;
	TSharedPtr<SBox> BodyHost;
	TSharedPtr<SBox> InspectorHost;
	TWeakObjectPtr<UDiurnalSchedule> SelectedAsset;
	TSet<TWeakObjectPtr<UDiurnalSchedule>> TouchedSchedules;
	TWeakObjectPtr<UDiurnalSchedule> PendingSelectionSource;
	FGuid PendingSelectionId;
	bool bPendingRename = false;
	bool bRestoringSelection = false;
	bool bInvalidConfiguration = false;
	EDiurnalScheduleBrowserMode Mode = EDiurnalScheduleBrowserMode::CombinedSchedule;
	EDiurnalScheduleSortMode SortMode = EDiurnalScheduleSortMode::DayAndTime;
	FDelegateHandle SettingsChangedHandle;
	FDelegateHandle ObjectsReplacedHandle;
	FDelegateHandle AssetRemovedHandle;
	FDelegateHandle AssetRenamedHandle;
	FDelegateHandle AssetUpdatedHandle;
};
