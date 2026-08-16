#include "ScheduleBrowser/SDiurnalScheduleBrowser.h"

#include "DiurnalCycleSettings.h"
#include "DiurnalCycleEditorStyle.h"
#include "DiurnalSchedule.h"
#include "ScheduleEditor/DiurnalScheduleEditorPresentation.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"
#include "UObject/UObjectGlobals.h"

struct FDiurnalScheduleBrowserItem
{
	TWeakObjectPtr<UDiurnalSchedule> Source;
	FName DisplayName;
	FGameplayTagContainer Tags;
	FText Type;
	FText Time;
	FText Behavior;
	FText SourceName;
	FGuid EntryId;
	FLinearColor EditorColor = FLinearColor::Gray;
	int32 SourceLayer = 0;
	int32 ManualIndex = 0;
	int32 Day = 0;
	int32 StartSeconds = 0;
	bool bRange = false;
	bool bOneOff = false;
	bool bBlocking = false;

	FString TagsString() const
	{
		return Tags.ToStringSimple();
	}
};

#define LOCTEXT_NAMESPACE "SDiurnalScheduleBrowser"

namespace
{
	TSharedRef<SWidget> MakeTagChip(const FText& Label, const FText& Tooltip)
	{
		return SNew(SBorder).Padding(FMargin(5, 1)).BorderImage(FAppStyle::GetBrush("Brushes.Header"))
			.BorderBackgroundColor(FStyleColors::Panel).ToolTipText(Tooltip)
			[SNew(STextBlock).Text(Label).Font(FAppStyle::GetFontStyle("SmallFont"))];
	}

	class SDiurnalBrowserTagChips final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SDiurnalBrowserTagChips) {}
			SLATE_ARGUMENT(FGameplayTagContainer, Tags)
		SLATE_END_ARGS()

		void Construct(const FArguments& Args)
		{
			const FDiurnalTagChipProjection Projection = DiurnalScheduleEditor::ProjectTagChips(Args._Tags, SDiurnalScheduleBrowser::MaximumVisibleTagChips);
			ChildSlot
			[
				SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)
				[
					SAssignNew(Row, SHorizontalBox)
				]
			];
			if (Projection.VisibleTags.IsEmpty())
			{
				Row->AddSlot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(SDiurnalScheduleBrowser::GetEmptyTagsLabel()).ToolTipText(LOCTEXT("NoTagsTip", "No semantic tags")).ColorAndOpacity(FSlateColor::UseSubduedForeground())
				];
				return;
			}
			for (const FGameplayTag SemanticTag : Projection.VisibleTags)
			{
				Row->AddSlot().AutoWidth().Padding(0, 0, 3, 0)[MakeTagChip(FText::FromName(SemanticTag.GetTagLeafName()), FText::FromString(SemanticTag.ToString()))];
			}
			if (Projection.OverflowCount > 0)
			{
				Row->AddSlot().AutoWidth()[MakeTagChip(FText::FromString(FString::Printf(TEXT("+%d"), Projection.OverflowCount)), Projection.OverflowTooltip)];
			}
		}

	private:
		TSharedPtr<SHorizontalBox> Row;
	};

	class SDiurnalScheduleBrowserRow final : public SMultiColumnTableRow<TSharedPtr<FDiurnalScheduleBrowserItem>>
	{
	public:
		SLATE_BEGIN_ARGS(SDiurnalScheduleBrowserRow) {}
			SLATE_ARGUMENT(TSharedPtr<FDiurnalScheduleBrowserItem>, Item)
		SLATE_END_ARGS()

		void Construct(const FArguments& Args, const TSharedRef<STableViewBase>& Owner)
		{
			Item = Args._Item;
			SMultiColumnTableRow::Construct(
				FSuperRowType::FArguments()
				.Padding(FMargin(4, 2))
				.ToolTipText(FText::FromString(Item ? Item->TagsString() : FString())),
				Owner);
		}

		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
		{
			if (!Item) return SNew(STextBlock);
			if (ColumnName == TEXT("Name"))
			{
				return SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0, 1, 7, 1)
					[SNew(SBorder).Padding(FMargin(2, 0)).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Item->EditorColor)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 5, 0)
					[SNew(SImage).Image(FDiurnalCycleEditorStyle::Get().GetBrush(Item->bRange ? "DiurnalCycle.Entry.Range" : Item->bOneOff ? "DiurnalCycle.Entry.Once" : "DiurnalCycle.Entry.Repeating")).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
					+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
					[SNew(STextBlock).Text(FText::FromName(Item->DisplayName)).Font(FAppStyle::GetFontStyle("NormalFontBold"))];
			}
			if (ColumnName == TEXT("Tags"))
			{
				return SNew(SDiurnalBrowserTagChips).Tags(Item->Tags).Clipping(EWidgetClipping::ClipToBounds);
			}
			FText Text;
			if (ColumnName == TEXT("Type")) Text = Item->Type;
			else if (ColumnName == TEXT("Time")) Text = Item->Time;
			else if (ColumnName == TEXT("Behavior")) Text = Item->Behavior;
			else if (ColumnName == TEXT("Source")) Text = Item->SourceName;
			return SNew(SBox).Padding(FMargin(4, 0)).Clipping(EWidgetClipping::ClipToBounds)
			[
				SNew(STextBlock)
				.Text(Text)
				.Font(FAppStyle::GetFontStyle("NormalFont"))
				.Clipping(EWidgetClipping::ClipToBounds)
				.ColorAndOpacity(ColumnName == TEXT("Source") ? FSlateColor::UseSubduedForeground() : FSlateColor::UseForeground())
			];
		}

	private:
		TSharedPtr<FDiurnalScheduleBrowserItem> Item;
	};
}

void SDiurnalScheduleBrowser::Construct(const FArguments&)
{
	SettingsChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(this, &SDiurnalScheduleBrowser::HandleObjectPropertyChanged);
	ObjectsReplacedHandle = FCoreUObjectDelegates::OnObjectsReplaced.AddSP(this, &SDiurnalScheduleBrowser::HandleObjectsReplaced);
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	AssetRemovedHandle = AssetRegistry.OnAssetRemoved().AddSP(this, &SDiurnalScheduleBrowser::HandleScheduleAssetChanged);
	AssetUpdatedHandle = AssetRegistry.OnAssetUpdated().AddSP(this, &SDiurnalScheduleBrowser::HandleScheduleAssetChanged);
	AssetRenamedHandle = AssetRegistry.OnAssetRenamed().AddSP(this, &SDiurnalScheduleBrowser::HandleScheduleAssetRenamed);
	ChildSlot
	[
		SNew(SBorder)
		.Padding(10)
		.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(2, 0, 2, 6)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(LOCTEXT("Title", "Schedule Browser")).Font(FAppStyle::GetFontStyle("HeadingSmall"))
				]
				+ SHorizontalBox::Slot().FillWidth(1)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0)
				[
					SNew(SImage).Image(FAppStyle::GetBrush("Level.LockedIcon16x")).ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(LOCTEXT("ReadOnly", "Read-only")).ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2, 0, 2, 10)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 20, 0)
				[
					SNew(STextBlock).Text(LOCTEXT("DefaultsSource", "Source: Project Defaults"))
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(STextBlock).Text(this, &SDiurnalScheduleBrowser::GetSourceStateText).ColorAndOpacity(FSlateColor(FStyleColors::AccentBlue))
				]
				+ SHorizontalBox::Slot().FillWidth(1)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").Text(LOCTEXT("Refresh", "Refresh")).ToolTipText(LOCTEXT("RefreshTip", "Refresh configured schedule layers.")).OnClicked(this, &SDiurnalScheduleBrowser::RefreshBrowser)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1)
				[
					SNew(SSearchBox)
						.HintText(LOCTEXT("Search", "Search name, tag, type, behavior, or source…"))
						.OnTextChanged(this, &SDiurnalScheduleBrowser::SetSearchText)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0, 0, 0)
				[
					SNew(SComboButton).ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.OnGetMenuContent(this, &SDiurnalScheduleBrowser::BuildSortMenu)
					.ButtonContent()[SNew(STextBlock).Text(this, &SDiurnalScheduleBrowser::GetSortText)]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0,0,12,0)[SNew(SCheckBox).IsChecked(ECheckBoxState::Checked).OnCheckStateChanged(this, &SDiurnalScheduleBrowser::SetFilter, FName(TEXT("Repeating")))[SNew(STextBlock).Text(LOCTEXT("RepeatingFilter", "Repeating"))]]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0,0,12,0)[SNew(SCheckBox).IsChecked(ECheckBoxState::Checked).OnCheckStateChanged(this, &SDiurnalScheduleBrowser::SetFilter, FName(TEXT("OneOff")))[SNew(STextBlock).Text(LOCTEXT("OneOffFilter", "One-off"))]]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0,0,20,0)[SNew(SCheckBox).IsChecked(ECheckBoxState::Checked).OnCheckStateChanged(this, &SDiurnalScheduleBrowser::SetFilter, FName(TEXT("Range")))[SNew(STextBlock).Text(LOCTEXT("RangeFilter", "Range"))]]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0,0,12,0)[SNew(SCheckBox).IsChecked(ECheckBoxState::Checked).OnCheckStateChanged(this, &SDiurnalScheduleBrowser::SetFilter, FName(TEXT("Notify")))[SNew(STextBlock).Text(LOCTEXT("NotifyFilter", "Notify"))]]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SCheckBox).IsChecked(ECheckBoxState::Checked).OnCheckStateChanged(this, &SDiurnalScheduleBrowser::SetFilter, FName(TEXT("Blocking")))[SNew(STextBlock).Text(LOCTEXT("BlockingFilter", "Blocking"))]]
			]
			+ SVerticalBox::Slot().FillHeight(1)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SAssignNew(ListView, SListView<TSharedPtr<FDiurnalScheduleBrowserItem>>)
					.ListItemsSource(&FilteredItems)
					.SelectionMode(ESelectionMode::Single)
					.OnGenerateRow_Lambda([](TSharedPtr<FDiurnalScheduleBrowserItem> Item, const TSharedRef<STableViewBase>& Owner)
					{
						return SNew(SDiurnalScheduleBrowserRow, Owner).Item(Item);
					})
					.OnSelectionChanged(this, &SDiurnalScheduleBrowser::SelectionChanged)
					.OnMouseButtonDoubleClick(this, &SDiurnalScheduleBrowser::DoubleClick)
					.HeaderRow
					(
						SNew(SHeaderRow)
						+ SHeaderRow::Column(TEXT("Name")).DefaultLabel(LOCTEXT("NameColumn", "Name")).FillWidth(.23f)
						+ SHeaderRow::Column(TEXT("Tags")).DefaultLabel(LOCTEXT("TagsColumn", "Tags")).FillWidth(.18f)
						+ SHeaderRow::Column(TEXT("Type")).DefaultLabel(LOCTEXT("TypeColumn", "Type")).FillWidth(.13f)
						+ SHeaderRow::Column(TEXT("Time")).DefaultLabel(LOCTEXT("TimeColumn", "Time")).FillWidth(.16f)
						+ SHeaderRow::Column(TEXT("Behavior")).DefaultLabel(LOCTEXT("BehaviorColumn", "Behavior")).FillWidth(.13f)
						+ SHeaderRow::Column(TEXT("Source")).DefaultLabel(LOCTEXT("SourceColumn", "Source")).FillWidth(.17f)
					)
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(this, &SDiurnalScheduleBrowser::GetEmptyText)
					.AutoWrapText(true)
					.Justification(ETextJustify::Center)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.Visibility(this, &SDiurnalScheduleBrowser::GetEmptyVisibility)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
			[
				SNew(SBorder)
				.Padding(FMargin(8, 5))
				.BorderImage(FAppStyle::GetBrush("Brushes.Recessed"))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(this, &SDiurnalScheduleBrowser::GetSelectionSummary).ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton).Text(LOCTEXT("OpenSource", "Open Source Schedule")).IsEnabled(this, &SDiurnalScheduleBrowser::CanOpenSelectedSource).OnClicked(this, &SDiurnalScheduleBrowser::OpenSelectedSource)
					]
				]
			]
		]
	];
	Rebuild();
}

SDiurnalScheduleBrowser::~SDiurnalScheduleBrowser()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(SettingsChangedHandle);
	FCoreUObjectDelegates::OnObjectsReplaced.Remove(ObjectsReplacedHandle);
	if (FAssetRegistryModule* Module = FModuleManager::GetModulePtr<FAssetRegistryModule>(TEXT("AssetRegistry")))
	{
		IAssetRegistry& AssetRegistry = Module->Get();
		AssetRegistry.OnAssetRemoved().Remove(AssetRemovedHandle);
		AssetRegistry.OnAssetUpdated().Remove(AssetUpdatedHandle);
		AssetRegistry.OnAssetRenamed().Remove(AssetRenamedHandle);
	}
}

FText SDiurnalScheduleBrowser::GetEventTypeLabel(const bool bOneOff)
{
	return bOneOff ? LOCTEXT("Once", "One-off Event") : LOCTEXT("Repeating", "Repeating Event");
}

FText SDiurnalScheduleBrowser::GetEmptyTagsLabel()
{
	return LOCTEXT("NoTags", "—");
}

void SDiurnalScheduleBrowser::Rebuild()
{
	const FGuid PreviousEntryId = SelectedItem ? SelectedItem->EntryId : FGuid();
	const TWeakObjectPtr<UDiurnalSchedule> PreviousSource = SelectedItem ? SelectedItem->Source : nullptr;
	AllItems.Reset();
	SelectedItem.Reset();
	const UDiurnalCycleSettings* Settings = GetDefault<UDiurnalCycleSettings>();
	int32 FirstDuplicateIndex = INDEX_NONE;
	int32 DuplicateIndex = INDEX_NONE;
	FSoftObjectPath DuplicatePath;
	bInvalidConfiguration = DiurnalCycle::FindDuplicateScheduleReference(Settings->DefaultSchedules, FirstDuplicateIndex, DuplicateIndex, DuplicatePath);
	if (bInvalidConfiguration)
	{
		SetSearchText(SearchText);
		return;
	}
	int32 SourceLayer = 0;
	for (const TSoftObjectPtr<UDiurnalSchedule>& Ref : Settings->DefaultSchedules)
	{
		if (UDiurnalSchedule* Schedule = Ref.LoadSynchronous())
		{
			for (int32 EventIndex = 0; EventIndex < Schedule->TimeEvents.Num(); ++EventIndex)
			{
				const FDiurnalTimeEvent& Event = Schedule->TimeEvents[EventIndex];
				const FDiurnalRecurrence Recurrence = Event.Recurrence;
				TSharedRef<FDiurnalScheduleBrowserItem> Item = MakeShared<FDiurnalScheduleBrowserItem>();
				Item->Source = Schedule; Item->DisplayName = Event.GetDisplayName(); Item->Tags = Event.EventTags;
				Item->EntryId = Event.EntryId; Item->EditorColor = DiurnalScheduleEditor::GetEditorColor(Event); Item->SourceLayer = SourceLayer; Item->ManualIndex = EventIndex; Item->Day = Recurrence.AnchorDay; Item->StartSeconds = Event.TimeOfDay.ToSecondsIntoDay();
				Item->bOneOff = Recurrence.Mode == EDiurnalRecurrenceMode::Once; Item->bBlocking = Event.IsBlocking(); Item->Type = GetEventTypeLabel(Item->bOneOff);
				Item->Time = FText::FromString(Item->bOneOff ? FString::Printf(TEXT("Day %d · %s"), Recurrence.AnchorDay, *Event.TimeOfDay.ToString()) : FString::Printf(TEXT("Every %d day%s · %s"), Recurrence.IntervalDays, Recurrence.IntervalDays == 1 ? TEXT("") : TEXT("s"), *Event.TimeOfDay.ToString()));
				Item->Behavior = Event.IsBlocking() ? LOCTEXT("Blocking", "Blocking") : LOCTEXT("Notify", "Notify"); Item->SourceName = FText::FromString(Schedule->GetName()); AllItems.Add(Item);
			}
			for (int32 RangeIndex = 0; RangeIndex < Schedule->TimeRanges.Num(); ++RangeIndex)
			{
				const FDiurnalTimeRange& Range = Schedule->TimeRanges[RangeIndex];
				const FDiurnalRecurrence Recurrence = Range.Recurrence;
				TSharedRef<FDiurnalScheduleBrowserItem> Item = MakeShared<FDiurnalScheduleBrowserItem>();
				Item->Source = Schedule; Item->DisplayName = Range.GetDisplayName(); Item->Tags = Range.RangeTags; Item->bRange = true;
				Item->EntryId = Range.EntryId; Item->EditorColor = DiurnalScheduleEditor::GetEditorColor(Range); Item->SourceLayer = SourceLayer; Item->ManualIndex = Schedule->TimeEvents.Num() + RangeIndex; Item->StartSeconds = Range.StartTime.ToSecondsIntoDay();
				Item->Day = Recurrence.AnchorDay; Item->bOneOff = Recurrence.Mode == EDiurnalRecurrenceMode::Once; Item->Type = LOCTEXT("Range", "Time Range"); Item->Time = FText::FromString(FString::Printf(TEXT("%s · %s–%s"), Item->bOneOff ? *FString::Printf(TEXT("Day %d"), Recurrence.AnchorDay) : *FString::Printf(TEXT("Every %d day%s"), Recurrence.IntervalDays, Recurrence.IntervalDays == 1 ? TEXT("") : TEXT("s")), *Range.StartTime.ToString(), *Range.EndTime.ToString())); Item->Behavior = LOCTEXT("ActiveState", "Active State"); Item->SourceName = FText::FromString(Schedule->GetName()); AllItems.Add(Item);
			}
		}
		++SourceLayer;
	}
	SetSearchText(SearchText);
	if (const TSharedPtr<FDiurnalScheduleBrowserItem>* RestoredItem = AllItems.FindByPredicate([&](const TSharedPtr<FDiurnalScheduleBrowserItem>& Item)
	{
		return Item && Item->EntryId == PreviousEntryId && Item->Source == PreviousSource;
	})) SelectedItem = *RestoredItem;
	if (ListView && SelectedItem) ListView->SetSelection(SelectedItem);
}

void SDiurnalScheduleBrowser::SetSearchText(const FText& Text)
{
	SearchText = Text;
	FilteredItems.Reset();
	TArray<FString> QueryTokens;
	Text.ToString().TrimStartAndEnd().ToLower().ParseIntoArrayWS(QueryTokens);
	for (const TSharedPtr<FDiurnalScheduleBrowserItem>& Item : AllItems)
	{
		FString TagLeaves;
		for (const FGameplayTag SemanticTag : Item->Tags.GetGameplayTagArray()) TagLeaves += TEXT(" ") + SemanticTag.GetTagLeafName().ToString();
		const FString Haystack = (Item->DisplayName.ToString() + TEXT(" ") + Item->TagsString() + TagLeaves + TEXT(" ") + Item->Type.ToString() + TEXT(" ") + Item->Behavior.ToString() + TEXT(" ") + Item->SourceName.ToString()).ToLower();
		const bool bTypeVisible = Item->bRange ? bShowRanges : Item->bOneOff ? bShowOneOff : bShowRepeating;
		const bool bBehaviorVisible = Item->bRange || (Item->bBlocking ? bShowBlocking : bShowNotify);
		bool bSearchMatches = true;
		for (const FString& Token : QueryTokens) bSearchMatches &= Haystack.Contains(Token);
		if (bTypeVisible && bBehaviorVisible && bSearchMatches) FilteredItems.Add(Item);
	}
	FilteredItems.StableSort([this](const TSharedPtr<FDiurnalScheduleBrowserItem>& Left, const TSharedPtr<FDiurnalScheduleBrowserItem>& Right)
	{
		if (!Left || !Right) return Left.IsValid();
		auto TieBreak = [&]
		{
			if (Left->SourceLayer != Right->SourceLayer) return Left->SourceLayer < Right->SourceLayer;
			if (Left->ManualIndex != Right->ManualIndex) return Left->ManualIndex < Right->ManualIndex;
			return Left->EntryId.ToString() < Right->EntryId.ToString();
		};
		switch (SortMode)
		{
		case EDiurnalScheduleSortMode::Name:
		{
			const int32 Compare = Left->DisplayName.ToString().Compare(Right->DisplayName.ToString(), ESearchCase::IgnoreCase);
			if (Compare != 0) return Compare < 0;
			break;
		}
		case EDiurnalScheduleSortMode::Type:
			if (Left->bRange != Right->bRange) return !Left->bRange;
			if (Left->StartSeconds != Right->StartSeconds) return Left->StartSeconds < Right->StartSeconds;
			break;
		case EDiurnalScheduleSortMode::DayAndTime:
			if (Left->bRange != Right->bRange) return !Left->bRange;
			if (!Left->bRange && Left->bOneOff != Right->bOneOff) return !Left->bOneOff;
			if (Left->Day != Right->Day) return Left->Day < Right->Day;
			if (Left->StartSeconds != Right->StartSeconds) return Left->StartSeconds < Right->StartSeconds;
			break;
		case EDiurnalScheduleSortMode::TimeOfDay:
			if (Left->StartSeconds != Right->StartSeconds) return Left->StartSeconds < Right->StartSeconds;
			break;
		case EDiurnalScheduleSortMode::ManualOrder:
		default: break;
		}
		return TieBreak();
	});
	if (ListView) ListView->RequestListRefresh();
}

void SDiurnalScheduleBrowser::SetFilter(const ECheckBoxState State, const FName Filter)
{
	const bool bEnabled = State == ECheckBoxState::Checked;
	if (Filter == TEXT("Repeating")) bShowRepeating = bEnabled; else if (Filter == TEXT("OneOff")) bShowOneOff = bEnabled; else if (Filter == TEXT("Range")) bShowRanges = bEnabled; else if (Filter == TEXT("Notify")) bShowNotify = bEnabled; else if (Filter == TEXT("Blocking")) bShowBlocking = bEnabled;
	SetSearchText(SearchText);
}

TSharedRef<SWidget> SDiurnalScheduleBrowser::BuildSortMenu()
{
	FMenuBuilder Menu(true, nullptr);
	for (const EDiurnalScheduleSortMode Mode : { EDiurnalScheduleSortMode::ManualOrder, EDiurnalScheduleSortMode::Name, EDiurnalScheduleSortMode::Type, EDiurnalScheduleSortMode::DayAndTime, EDiurnalScheduleSortMode::TimeOfDay })
	{
		Menu.AddMenuEntry(DiurnalScheduleEditor::GetSortModeText(Mode), FText::GetEmpty(), FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SDiurnalScheduleBrowser::SetSortMode, Mode), FCanExecuteAction(), FIsActionChecked::CreateLambda([this, Mode] { return SortMode == Mode; })), NAME_None, EUserInterfaceActionType::RadioButton);
	}
	return Menu.MakeWidget();
}

void SDiurnalScheduleBrowser::SetSortMode(const EDiurnalScheduleSortMode InSortMode)
{
	SortMode = InSortMode;
	SetSearchText(SearchText);
}

FText SDiurnalScheduleBrowser::GetSortText() const
{
	return FText::Format(LOCTEXT("SortValue", "Sort: {0}"), DiurnalScheduleEditor::GetSortModeText(SortMode));
}

void SDiurnalScheduleBrowser::SelectionChanged(TSharedPtr<FDiurnalScheduleBrowserItem> Item, ESelectInfo::Type)
{
	SelectedItem = MoveTemp(Item);
}

void SDiurnalScheduleBrowser::DoubleClick(TSharedPtr<FDiurnalScheduleBrowserItem> Item)
{
	SelectedItem = MoveTemp(Item);
	OpenSelectedSource();
}

FReply SDiurnalScheduleBrowser::OpenSelectedSource()
{
	if (GEditor && SelectedItem && SelectedItem->Source.IsValid()) GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(SelectedItem->Source.Get());
	return FReply::Handled();
}

FReply SDiurnalScheduleBrowser::RefreshBrowser()
{
	Rebuild();
	return FReply::Handled();
}

bool SDiurnalScheduleBrowser::CanOpenSelectedSource() const
{
	return SelectedItem && SelectedItem->Source.IsValid();
}

FText SDiurnalScheduleBrowser::GetSourceStateText() const
{
	const UDiurnalCycleSettings* Settings = GetDefault<UDiurnalCycleSettings>();
	if (bInvalidConfiguration) return LOCTEXT("InvalidSource", "Invalid project schedule configuration");
	if (Settings->DefaultSchedules.IsEmpty()) return LOCTEXT("NoSource", "Not configured");
	return Settings->DefaultSchedules.Num() == 1
		? LOCTEXT("OneSchedule", "1 schedule")
		: FText::Format(LOCTEXT("LayerSource", "{0} schedules · merged"), Settings->DefaultSchedules.Num());
}

FText SDiurnalScheduleBrowser::GetEmptyText() const
{
	return bInvalidConfiguration
		? LOCTEXT("InvalidEmpty", "Default Schedules contains the same schedule more than once. Remove the duplicate in Project Settings before browsing the composed schedule.")
		: LOCTEXT("Empty", "No entries match this view. Configure Default Schedules in Project Settings or adjust the current filters.");
}

FText SDiurnalScheduleBrowser::GetSelectionSummary() const
{
	if (!SelectedItem) return LOCTEXT("SelectSummary", "Select an entry to inspect its source. Double-click asset-owned rows to open their schedule.");
	return FText::Format(LOCTEXT("AssetSummary", "{0} is authored by {1}. This merged browser is read-only."), FText::FromName(SelectedItem->DisplayName), SelectedItem->SourceName);
}

EVisibility SDiurnalScheduleBrowser::GetEmptyVisibility() const
{
	return FilteredItems.IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed;
}

void SDiurnalScheduleBrowser::HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
{
	if (Object != GetMutableDefault<UDiurnalCycleSettings>())
	{
		const UDiurnalSchedule* ChangedSchedule = Cast<UDiurnalSchedule>(Object);
		if (!ChangedSchedule || !AllItems.ContainsByPredicate([ChangedSchedule](const TSharedPtr<FDiurnalScheduleBrowserItem>& Item)
		{
			return Item && Item->Source.Get() == ChangedSchedule;
		})) return;
		Rebuild();
		return;
	}
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	const FName MemberPropertyName = PropertyChangedEvent.GetMemberPropertyName();
	const FName DefaultsName = GET_MEMBER_NAME_CHECKED(UDiurnalCycleSettings, DefaultSchedules);
	if (PropertyName == DefaultsName || MemberPropertyName == DefaultsName)
	{
		Rebuild();
	}
}

void SDiurnalScheduleBrowser::HandleObjectsReplaced(const TMap<UObject*, UObject*>& Replacements)
{
	for (const TPair<UObject*, UObject*>& Pair : Replacements)
	{
		const UDiurnalSchedule* PreviousSchedule = Cast<UDiurnalSchedule>(Pair.Key);
		const UDiurnalSchedule* ReplacementSchedule = Cast<UDiurnalSchedule>(Pair.Value);
		if ((PreviousSchedule && IsRelevantSchedulePath(FSoftObjectPath(PreviousSchedule)))
			|| (ReplacementSchedule && IsRelevantSchedulePath(FSoftObjectPath(ReplacementSchedule))))
		{
			Rebuild();
			return;
		}
	}
}

void SDiurnalScheduleBrowser::HandleScheduleAssetChanged(const FAssetData& AssetData)
{
	if (AssetData.AssetClassPath == UDiurnalSchedule::StaticClass()->GetClassPathName()
		&& IsRelevantSchedulePath(AssetData.GetSoftObjectPath()))
	{
		Rebuild();
	}
}

void SDiurnalScheduleBrowser::HandleScheduleAssetRenamed(const FAssetData& AssetData, const FString& OldObjectPath)
{
	if (AssetData.AssetClassPath == UDiurnalSchedule::StaticClass()->GetClassPathName()
		&& (IsRelevantSchedulePath(AssetData.GetSoftObjectPath())
			|| IsRelevantSchedulePath(FSoftObjectPath(OldObjectPath))))
	{
		Rebuild();
	}
}

bool SDiurnalScheduleBrowser::IsRelevantSchedulePath(const FSoftObjectPath& ObjectPath) const
{
	if (ObjectPath.IsNull())
	{
		return false;
	}

	const UDiurnalCycleSettings* Settings = GetDefault<UDiurnalCycleSettings>();
	if (Settings->DefaultSchedules.ContainsByPredicate([&ObjectPath](const TSoftObjectPtr<UDiurnalSchedule>& Reference)
	{
		return Reference.ToSoftObjectPath() == ObjectPath;
	}))
	{
		return true;
	}

	return AllItems.ContainsByPredicate([&ObjectPath](const TSharedPtr<FDiurnalScheduleBrowserItem>& Item)
	{
		return Item && Item->Source.IsValid() && FSoftObjectPath(Item->Source.Get()) == ObjectPath;
	});
}

#undef LOCTEXT_NAMESPACE
