#include "ScheduleBrowser/SDiurnalScheduleBrowser.h"

#include "DiurnalCycleEditor.h"
#include "DiurnalCycleEditorStyle.h"
#include "DiurnalCycleSettings.h"
#include "DiurnalSchedule.h"
#include "DiurnalScheduleFactory.h"
#include "ScheduleEditor/DiurnalScheduleEditorModel.h"
#include "ScheduleEditor/DiurnalScheduleEditorPresentation.h"
#include "ScheduleEditor/DiurnalScheduleEditorToolkit.h"
#include "ScheduleEditor/Widgets/SDiurnalScheduleInspector.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AssetViewUtils.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "Filters/GenericFilter.h"
#include "Filters/SBasicFilterBar.h"
#include "FileHelpers.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IAssetTools.h"
#include "IContentBrowserSingleton.h"
#include "Misc/PackageName.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

struct FDiurnalScheduleBrowserItem
{
	TWeakObjectPtr<UDiurnalSchedule> Source;
	TWeakPtr<SInlineEditableTextBlock> InlineName;
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

	FString TagsString() const { return Tags.ToStringSimple(); }
	EDiurnalScheduleSelectionType GetSelectionType() const
	{
		return bRange ? EDiurnalScheduleSelectionType::Range : EDiurnalScheduleSelectionType::Event;
	}
};

#define LOCTEXT_NAMESPACE "SDiurnalScheduleBrowser"

namespace
{
	TSharedRef<SWidget> MakeTagChip(const FText& Label, const FText& Tooltip)
	{
		return SNew(SBorder)
			.Padding(FMargin(5, 1))
			.BorderImage(FAppStyle::GetBrush("Brushes.Header"))
			.BorderBackgroundColor(FStyleColors::Panel)
			.ToolTipText(Tooltip)
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
			const FDiurnalTagChipProjection Projection = DiurnalScheduleEditor::ProjectTagChips(
				Args._Tags, SDiurnalScheduleBrowser::MaximumVisibleTagChips);
			TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
			if (Projection.VisibleTags.IsEmpty())
			{
				Row->AddSlot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(SDiurnalScheduleBrowser::GetEmptyTagsLabel())
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				];
			}
			for (const FGameplayTag SemanticTag : Projection.VisibleTags)
			{
				Row->AddSlot().AutoWidth().Padding(0, 0, 3, 0)
				[
					MakeTagChip(
						FText::FromName(SemanticTag.GetTagLeafName()),
						FText::FromString(SemanticTag.ToString()))
				];
			}
			if (Projection.OverflowCount > 0)
			{
				Row->AddSlot().AutoWidth()
				[
					MakeTagChip(
						FText::FromString(FString::Printf(TEXT("+%d"), Projection.OverflowCount)),
						Projection.OverflowTooltip)
				];
			}
			ChildSlot[SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)[Row]];
		}
	};

	class SDiurnalScheduleBrowserRow final
		: public SMultiColumnTableRow<TSharedPtr<FDiurnalScheduleBrowserItem>>
	{
	public:
		SLATE_BEGIN_ARGS(SDiurnalScheduleBrowserRow) {}
			SLATE_ARGUMENT(TSharedPtr<FDiurnalScheduleBrowserItem>, Item)
			SLATE_EVENT(FOnVerifyTextChanged, OnVerifyName)
			SLATE_EVENT(FOnTextCommitted, OnNameCommitted)
		SLATE_END_ARGS()

		void Construct(const FArguments& Args, const TSharedRef<STableViewBase>& Owner)
		{
			Item = Args._Item;
			OnVerifyName = Args._OnVerifyName;
			OnNameCommitted = Args._OnNameCommitted;
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
				TSharedPtr<SInlineEditableTextBlock> NameWidget;
				TSharedRef<SWidget> Result = SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0, 1, 7, 1)
					[
						SNew(SBorder)
						.Padding(FMargin(2, 0))
						.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
						.BorderBackgroundColor(Item->EditorColor)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 5, 0)
					[
						SNew(SBox)
						.WidthOverride(16)
						.HeightOverride(16)
						.ToolTipText(Item->bOneOff ? LOCTEXT("OnceOccurrenceIcon", "One-off occurrence") : LOCTEXT("RepeatingOccurrenceIcon", "Repeating occurrence"))
						[
							SNew(SImage)
							.Image(FDiurnalCycleEditorStyle::Get().GetBrush(
								FDiurnalCycleEditorStyle::GetOccurrenceIconName(Item->bOneOff)))
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
					[
						SAssignNew(NameWidget, SInlineEditableTextBlock)
						.Text(FText::FromName(Item->DisplayName))
						.Font(FAppStyle::GetFontStyle("NormalFontBold"))
						.OnVerifyTextChanged(OnVerifyName)
						.OnTextCommitted(OnNameCommitted)
					];
				Item->InlineName = NameWidget;
				return Result;
			}
			if (ColumnName == TEXT("Tags"))
			{
				return SNew(SDiurnalBrowserTagChips).Tags(Item->Tags);
			}
			if (ColumnName == TEXT("Behavior"))
			{
				return SNew(SBox).Padding(FMargin(4, 0))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 5, 0)
					[
						SNew(SImage)
						.Image(FDiurnalCycleEditorStyle::Get().GetBrush(Item->bRange
							? "DiurnalCycle.Entry.Range"
							: Item->bBlocking ? "DiurnalCycle.Entry.Blocking" : "DiurnalCycle.Entry.Notify"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(Item->Behavior)
					]
				];
			}

			FText Text;
			if (ColumnName == TEXT("Type")) Text = Item->Type;
			else if (ColumnName == TEXT("Time")) Text = Item->Time;
			else if (ColumnName == TEXT("Source")) Text = Item->SourceName;
			return SNew(SBox).Padding(FMargin(4, 0)).Clipping(EWidgetClipping::ClipToBounds)
			[
				SNew(STextBlock)
				.Text(Text)
				.Clipping(EWidgetClipping::ClipToBounds)
				.ColorAndOpacity(
					ColumnName == TEXT("Source")
						? FSlateColor::UseSubduedForeground()
						: FSlateColor::UseForeground())
			];
		}

	private:
		TSharedPtr<FDiurnalScheduleBrowserItem> Item;
		FOnVerifyTextChanged OnVerifyName;
		FOnTextCommitted OnNameCommitted;
	};
}

void SDiurnalScheduleBrowser::Construct(const FArguments&)
{
	SettingsChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(
		this, &SDiurnalScheduleBrowser::HandleObjectPropertyChanged);
	ObjectsReplacedHandle = FCoreUObjectDelegates::OnObjectsReplaced.AddSP(
		this, &SDiurnalScheduleBrowser::HandleObjectsReplaced);
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
		TEXT("AssetRegistry")).Get();
	AssetRemovedHandle = AssetRegistry.OnAssetRemoved().AddSP(
		this, &SDiurnalScheduleBrowser::HandleScheduleAssetChanged);
	AssetUpdatedHandle = AssetRegistry.OnAssetUpdated().AddSP(
		this, &SDiurnalScheduleBrowser::HandleScheduleAssetChanged);
	AssetRenamedHandle = AssetRegistry.OnAssetRenamed().AddSP(
		this, &SDiurnalScheduleBrowser::HandleScheduleAssetRenamed);

	using FBrowserItemPtr = TSharedPtr<FDiurnalScheduleBrowserItem>;
	using FBrowserFilter = FGenericFilter<FBrowserItemPtr>;
	TArray<TSharedRef<FFilterBase<FBrowserItemPtr>>> Filters;
	const TSharedRef<FFilterCategory> TypeCategory = MakeShared<FFilterCategory>(
		LOCTEXT("TypeFilterCategory", "Entry Type"),
		LOCTEXT("TypeFilterCategoryTip", "Filter entries by authored type."));
	const TSharedRef<FFilterCategory> RecurrenceCategory = MakeShared<FFilterCategory>(
		LOCTEXT("RecurrenceFilterCategory", "Recurrence"),
		LOCTEXT("RecurrenceFilterCategoryTip", "Filter entries by recurrence mode."));
	const TSharedRef<FFilterCategory> BehaviorCategory = MakeShared<FFilterCategory>(
		LOCTEXT("BehaviorFilterCategory", "Event Behavior"),
		LOCTEXT("BehaviorFilterCategoryTip", "Filter event entries by behavior."));
	const auto AddFilter = [&Filters](
		const TSharedRef<FFilterCategory>& Category,
		const TCHAR* Name,
		const FText& Label,
		const FText& Tooltip,
		FBrowserFilter::FOnItemFiltered Predicate)
	{
		const TSharedRef<FBrowserFilter> Filter = MakeShared<FBrowserFilter>(Category, Name, Label, MoveTemp(Predicate));
		Filter->SetToolTipText(Tooltip);
		Filter->SetColor(FLinearColor(0.18f, 0.55f, 0.92f));
		Filters.Add(Filter);
	};
	AddFilter(TypeCategory, TEXT("Events"), LOCTEXT("EventsFilter", "Events"),
		LOCTEXT("EventsFilterTip", "Show event entries."),
		FBrowserFilter::FOnItemFiltered::CreateLambda([](const FBrowserItemPtr Item) { return Item && !Item->bRange; }));
	AddFilter(TypeCategory, TEXT("Ranges"), LOCTEXT("RangesFilter", "Time Ranges"),
		LOCTEXT("RangesFilterTip", "Show time-range entries."),
		FBrowserFilter::FOnItemFiltered::CreateLambda([](const FBrowserItemPtr Item) { return Item && Item->bRange; }));
	AddFilter(RecurrenceCategory, TEXT("Once"), LOCTEXT("OnceFilter", "Once"),
		LOCTEXT("OnceFilterTip", "Show entries that occur once."),
		FBrowserFilter::FOnItemFiltered::CreateLambda([](const FBrowserItemPtr Item) { return Item && Item->bOneOff; }));
	AddFilter(RecurrenceCategory, TEXT("Repeating"), LOCTEXT("RepeatingFilter", "Repeating"),
		LOCTEXT("RepeatingFilterTip", "Show repeating entries."),
		FBrowserFilter::FOnItemFiltered::CreateLambda([](const FBrowserItemPtr Item) { return Item && !Item->bOneOff; }));
	AddFilter(BehaviorCategory, TEXT("Notify"), LOCTEXT("NotifyFilter", "Notify"),
		LOCTEXT("NotifyFilterTip", "Show non-blocking event entries."),
		FBrowserFilter::FOnItemFiltered::CreateLambda([](const FBrowserItemPtr Item) { return Item && !Item->bRange && !Item->bBlocking; }));
	AddFilter(BehaviorCategory, TEXT("Blocking"), LOCTEXT("BlockingFilter", "Blocking"),
		LOCTEXT("BlockingFilterTip", "Show blocking event entries."),
		FBrowserFilter::FOnItemFiltered::CreateLambda([](const FBrowserItemPtr Item) { return Item && !Item->bRange && Item->bBlocking; }));

	SAssignNew(EntryFilterBar, SBasicFilterBar<FBrowserItemPtr>)
		.CustomFilters(Filters)
		.UseSectionsForCategories(true)
		.FilterPillStyle(EFilterPillStyle::Default)
		.OnFilterChanged(this, &SDiurnalScheduleBrowser::HandleEntryFilterChanged);
	const TSharedRef<SWidget> EntryFilterButton =
		SBasicFilterBar<FBrowserItemPtr>::MakeAddFilterButton(EntryFilterBar.ToSharedRef());

	ChildSlot
	[
		SNew(SBorder)
		.Padding(10)
		.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(2, 0, 2, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 14, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Title", "Schedule Browser"))
					.Font(FAppStyle::GetFontStyle("HeadingSmall"))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 2, 0)
				[
					SNew(SCheckBox)
					.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
					.IsChecked(this, &SDiurnalScheduleBrowser::IsModeChecked, EDiurnalScheduleBrowserMode::CombinedSchedule)
					.OnCheckStateChanged(this, &SDiurnalScheduleBrowser::SetMode, EDiurnalScheduleBrowserMode::CombinedSchedule)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 5, 0)
						[SNew(SImage).Image(FDiurnalCycleEditorStyle::Get().GetBrush("DiurnalCycle.Toolbar.Schedule"))]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[SNew(STextBlock).Text(LOCTEXT("CombinedMode", "Combined Schedule"))]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
					.IsChecked(this, &SDiurnalScheduleBrowser::IsModeChecked, EDiurnalScheduleBrowserMode::ScheduleAssets)
					.OnCheckStateChanged(this, &SDiurnalScheduleBrowser::SetMode, EDiurnalScheduleBrowserMode::ScheduleAssets)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 5, 0)
						[SNew(SImage).Image(FDiurnalCycleEditorStyle::Get().GetBrush("DiurnalCycle.ScheduleIcon"))]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[SNew(STextBlock).Text(LOCTEXT("AssetsMode", "Schedule Assets"))]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0)
				[
					SNew(SBox).WidthOverride(24).HeightOverride(24)
					[
						SNew(SButton)
						.IsFocusable(false)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.ContentPadding(0)
						.HAlign(HAlign_Center).VAlign(VAlign_Center)
						.ToolTipText(LOCTEXT("SaveTip", "Save schedules"))
						.IsEnabled(this, &SDiurnalScheduleBrowser::CanSaveSchedules)
						.OnClicked(this, &SDiurnalScheduleBrowser::SaveSchedules)
						[
							SNew(SBox).WidthOverride(16).HeightOverride(16)
							[SNew(SImage).Image(FAppStyle::GetBrush("Icons.Save"))]
						]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0)
				[
					SNew(SBox).WidthOverride(24).HeightOverride(24)
					[
						SNew(SButton)
						.IsFocusable(false)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.ContentPadding(0)
						.HAlign(HAlign_Center).VAlign(VAlign_Center)
						.ToolTipText(LOCTEXT("RefreshTip", "Refresh"))
						.OnClicked(this, &SDiurnalScheduleBrowser::RefreshBrowser)
						[
							SNew(SBox).WidthOverride(16).HeightOverride(16)
							[SNew(SImage).Image(FAppStyle::GetBrush("Icons.Refresh"))]
						]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0)
				[
					SNew(SBox).WidthOverride(24).HeightOverride(24)
					[
						SNew(SButton)
						.IsFocusable(false)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.ContentPadding(0)
						.HAlign(HAlign_Center).VAlign(VAlign_Center)
						.ToolTipText(LOCTEXT("ProjectSettingsTip", "Settings"))
						.OnClicked(this, &SDiurnalScheduleBrowser::OpenProjectSettings)
						[
							SNew(SBox).WidthOverride(16).HeightOverride(16)
							[SNew(SImage).Image(FAppStyle::GetBrush("Icons.Settings"))]
						]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1)
				[
					SNew(SSearchBox)
					.HintText(LOCTEXT("Search", "Search entries by name, tag, type, behavior, or source…"))
					.OnTextChanged(this, &SDiurnalScheduleBrowser::SetSearchText)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0, 0, 0)
				[
					SNew(SBox)
					.Visibility(this, &SDiurnalScheduleBrowser::GetDefaultFilterVisibility)
					[EntryFilterButton]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0, 0, 0)
				[
					SNew(SComboButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.OnGetMenuContent(this, &SDiurnalScheduleBrowser::BuildSortMenu)
					.ButtonContent()[SNew(STextBlock).Text(this, &SDiurnalScheduleBrowser::GetSortText)]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
			[
				SNew(SBox)
				.Visibility(this, &SDiurnalScheduleBrowser::GetEntryFilterVisibility)
				[EntryFilterBar.ToSharedRef()]
			]
			+ SVerticalBox::Slot().FillHeight(1)
			[
				SAssignNew(BodyHost, SBox)
			]
		]
	];

	RebuildBody();
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

FText SDiurnalScheduleBrowser::GetEntryTypeLabel(const bool bRange, const bool bOneOff)
{
	if (bRange)
	{
		return bOneOff
			? LOCTEXT("OnceRangeType", "One-off Time Range")
			: LOCTEXT("RepeatingRangeType", "Repeating Time Range");
	}
	return bOneOff
		? LOCTEXT("OnceEventType", "One-off Event")
		: LOCTEXT("RepeatingEventType", "Repeating Event");
}

FText SDiurnalScheduleBrowser::GetEmptyTagsLabel()
{
	return LOCTEXT("NoTags", "None");
}

void SDiurnalScheduleBrowser::SetMode(
	const ECheckBoxState State,
	const EDiurnalScheduleBrowserMode NewMode)
{
	if (State != ECheckBoxState::Checked || Mode == NewMode) return;
	if (NewMode == EDiurnalScheduleBrowserMode::ScheduleAssets && !SelectedAsset.IsValid() && SelectedItem)
	{
		SelectedAsset = SelectedItem->Source;
	}
	Mode = NewMode;
	SelectedItem.Reset();
	RebuildBody();
	Rebuild();
}

ECheckBoxState SDiurnalScheduleBrowser::IsModeChecked(
	const EDiurnalScheduleBrowserMode TestMode) const
{
	return Mode == TestMode ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SDiurnalScheduleBrowser::RebuildBody()
{
	if (!BodyHost) return;
	BodyHost->SetContent(BuildBody());
	RebuildInspector();
}

TSharedRef<SWidget> SDiurnalScheduleBrowser::BuildBody()
{
	const TSharedRef<SWidget> Entries = BuildEntryPanel();
	SAssignNew(InspectorHost, SBox);
	if (Mode == EDiurnalScheduleBrowserMode::ScheduleAssets)
	{
		return SNew(SSplitter)
			+ SSplitter::Slot().Value(0.24f)[BuildAssetPicker()]
			+ SSplitter::Slot().Value(0.48f)[Entries]
			+ SSplitter::Slot().Value(0.28f)[InspectorHost.ToSharedRef()];
	}
	return SNew(SSplitter)
		+ SSplitter::Slot().Value(0.70f)[Entries]
		+ SSplitter::Slot().Value(0.30f)[InspectorHost.ToSharedRef()];
}

TSharedRef<SWidget> SDiurnalScheduleBrowser::BuildEntryPanel()
{
	return SNew(SBorder)
		.Padding(4)
		.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SAssignNew(ListView, SListView<TSharedPtr<FDiurnalScheduleBrowserItem>>)
				.ListItemsSource(&FilteredItems)
				.SelectionMode(ESelectionMode::Single)
				.OnGenerateRow(this, &SDiurnalScheduleBrowser::GenerateRow)
				.OnSelectionChanged(this, &SDiurnalScheduleBrowser::SelectionChanged)
				.OnItemScrolledIntoView(this, &SDiurnalScheduleBrowser::ItemScrolledIntoView)
				.OnMouseButtonDoubleClick(this, &SDiurnalScheduleBrowser::DoubleClick)
				.OnContextMenuOpening(this, &SDiurnalScheduleBrowser::BuildContextMenu)
				.OnKeyDownHandler(this, &SDiurnalScheduleBrowser::HandleListKeyDown)
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
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(24)
			[
				SNew(STextBlock)
				.Text(this, &SDiurnalScheduleBrowser::GetEmptyText)
				.AutoWrapText(true)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Visibility(this, &SDiurnalScheduleBrowser::GetEmptyVisibility)
			]
		];
}

TSharedRef<SWidget> SDiurnalScheduleBrowser::BuildAssetPicker()
{
	FAssetPickerConfig Config;
	Config.Filter.ClassPaths.Add(UDiurnalSchedule::StaticClass()->GetClassPathName());
	Config.Filter.bRecursiveClasses = true;
	Config.SelectionMode = ESelectionMode::Single;
	Config.InitialAssetViewType = EAssetViewType::List;
	Config.bAllowNullSelection = false;
	Config.bAllowDragging = true;
	Config.bAllowRename = true;
	Config.bShowBottomToolbar = false;
	Config.bForceShowPluginContent = true;
	Config.SaveSettingsName = TEXT("DiurnalScheduleBrowserAssets");
	Config.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SDiurnalScheduleBrowser::AssetSelected);
	Config.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &SDiurnalScheduleBrowser::AssetDoubleClicked);
	Config.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateSP(this, &SDiurnalScheduleBrowser::BuildAssetContextMenu);
	Config.OnExtendAssetPickerTopBar.BindSP(this, &SDiurnalScheduleBrowser::ExtendAssetPickerTopBar);
	if (SelectedAsset.IsValid()) Config.InitialAssetSelection = FAssetData(SelectedAsset.Get());
	FContentBrowserModule& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	AssetPickerWidget = ContentBrowser.Get().CreateAssetPicker(Config);
	return SNew(SBorder)
		.Padding(4)
		.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
		[AssetPickerWidget.ToSharedRef()];
}

void SDiurnalScheduleBrowser::Rebuild()
{
	const FGuid PreviousEntryId = PendingSelectionId.IsValid()
		? PendingSelectionId
		: SelectedItem ? SelectedItem->EntryId : FGuid();
	const TWeakObjectPtr<UDiurnalSchedule> PreviousSource = PendingSelectionSource.IsValid()
		? PendingSelectionSource
		: SelectedItem ? SelectedItem->Source : nullptr;
	const bool bSelectionWasPending = PendingSelectionId.IsValid();
	PendingSelectionId.Invalidate();
	PendingSelectionSource.Reset();
	AllItems.Reset();
	SelectedItem.Reset();
	bInvalidConfiguration = false;

	if (Mode == EDiurnalScheduleBrowserMode::CombinedSchedule)
	{
		const UDiurnalCycleSettings* Settings = GetDefault<UDiurnalCycleSettings>();
		int32 FirstDuplicateIndex = INDEX_NONE;
		int32 DuplicateIndex = INDEX_NONE;
		FSoftObjectPath DuplicatePath;
		bInvalidConfiguration = DiurnalCycle::FindDuplicateScheduleReference(
			Settings->DefaultSchedules, FirstDuplicateIndex, DuplicateIndex, DuplicatePath);
		if (!bInvalidConfiguration)
		{
			int32 SourceLayer = 0;
			for (const TSoftObjectPtr<UDiurnalSchedule>& Reference : Settings->DefaultSchedules)
			{
				if (UDiurnalSchedule* Schedule = Reference.LoadSynchronous())
				{
					PopulateSchedule(*Schedule, SourceLayer);
				}
				++SourceLayer;
			}
		}
	}
	else if (UDiurnalSchedule* Schedule = SelectedAsset.Get())
	{
		PopulateSchedule(*Schedule, 0);
	}

	SetSearchText(SearchText);
	if (const TSharedPtr<FDiurnalScheduleBrowserItem>* Restored = FilteredItems.FindByPredicate(
		[&](const TSharedPtr<FDiurnalScheduleBrowserItem>& Item)
		{
			return Item && Item->EntryId == PreviousEntryId && Item->Source == PreviousSource;
		}))
	{
		SelectedItem = *Restored;
	}

	if (ListView)
	{
		ListView->RequestListRefresh();
		if (SelectedItem)
		{
			bRestoringSelection = true;
			ListView->SetSelection(SelectedItem, ESelectInfo::Direct);
			bRestoringSelection = false;
			if (bPendingRename)
			{
				ListView->RequestScrollIntoView(SelectedItem);
			}
		}
		else
		{
			ListView->ClearSelection();
			bPendingRename = false;
		}
	}
	if (bSelectionWasPending || !SelectedItem) RebuildInspector();
}

void SDiurnalScheduleBrowser::PopulateSchedule(
	UDiurnalSchedule& Schedule,
	const int32 SourceLayer)
{
	for (int32 EventIndex = 0; EventIndex < Schedule.TimeEvents.Num(); ++EventIndex)
	{
		const FDiurnalTimeEvent& Event = Schedule.TimeEvents[EventIndex];
		const FDiurnalRecurrence Recurrence = Event.Recurrence;
		TSharedRef<FDiurnalScheduleBrowserItem> Item = MakeShared<FDiurnalScheduleBrowserItem>();
		Item->Source = &Schedule;
		Item->DisplayName = Event.GetDisplayName();
		Item->Tags = Event.EventTags;
		Item->EntryId = Event.EntryId;
		Item->EditorColor = DiurnalScheduleEditor::GetEditorColor(Event);
		Item->SourceLayer = SourceLayer;
		Item->ManualIndex = EventIndex;
		Item->Day = Recurrence.AnchorDay;
		Item->StartSeconds = Event.TimeOfDay.ToSecondsIntoDay();
		Item->bOneOff = Recurrence.Mode == EDiurnalRecurrenceMode::Once;
		Item->bBlocking = Event.IsBlocking();
		Item->Type = GetEntryTypeLabel(false, Item->bOneOff);
		Item->Time = FText::FromString(Item->bOneOff
			? FString::Printf(TEXT("Day %d, %s"), Recurrence.AnchorDay, *Event.TimeOfDay.ToString())
			: FString::Printf(TEXT("Every %d day%s from Day %d, %s"), Recurrence.IntervalDays,
				Recurrence.IntervalDays == 1 ? TEXT("") : TEXT("s"), Recurrence.AnchorDay, *Event.TimeOfDay.ToString()));
		Item->Behavior = Event.IsBlocking() ? LOCTEXT("Blocking", "Blocking") : LOCTEXT("Notify", "Notify");
		Item->SourceName = FText::FromString(Schedule.GetName());
		AllItems.Add(Item);
	}

	for (int32 RangeIndex = 0; RangeIndex < Schedule.TimeRanges.Num(); ++RangeIndex)
	{
		const FDiurnalTimeRange& Range = Schedule.TimeRanges[RangeIndex];
		const FDiurnalRecurrence Recurrence = Range.Recurrence;
		TSharedRef<FDiurnalScheduleBrowserItem> Item = MakeShared<FDiurnalScheduleBrowserItem>();
		Item->Source = &Schedule;
		Item->DisplayName = Range.GetDisplayName();
		Item->Tags = Range.RangeTags;
		Item->bRange = true;
		Item->EntryId = Range.EntryId;
		Item->EditorColor = DiurnalScheduleEditor::GetEditorColor(Range);
		Item->SourceLayer = SourceLayer;
		Item->ManualIndex = Schedule.TimeEvents.Num() + RangeIndex;
		Item->StartSeconds = Range.StartTime.ToSecondsIntoDay();
		Item->Day = Recurrence.AnchorDay;
		Item->bOneOff = Recurrence.Mode == EDiurnalRecurrenceMode::Once;
		Item->Type = GetEntryTypeLabel(true, Item->bOneOff);
		const FString RecurrenceText = Item->bOneOff
			? FString::Printf(TEXT("Day %d"), Recurrence.AnchorDay)
			: FString::Printf(TEXT("Every %d day%s from Day %d"), Recurrence.IntervalDays,
				Recurrence.IntervalDays == 1 ? TEXT("") : TEXT("s"), Recurrence.AnchorDay);
		Item->Time = FText::FromString(FString::Printf(
			TEXT("%s, %s to %s"), *RecurrenceText,
			*Range.StartTime.ToString(), *Range.EndTime.ToString()));
		Item->Behavior = LOCTEXT("ActiveState", "Active State");
		Item->SourceName = FText::FromString(Schedule.GetName());
		AllItems.Add(Item);
	}
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
		for (const FGameplayTag SemanticTag : Item->Tags.GetGameplayTagArray())
		{
			TagLeaves += TEXT(" ") + SemanticTag.GetTagLeafName().ToString();
		}
		const FString Haystack = (
			Item->DisplayName.ToString() + TEXT(" ") + Item->TagsString() + TagLeaves + TEXT(" ")
			+ Item->Type.ToString() + TEXT(" ") + Item->Behavior.ToString() + TEXT(" ")
			+ Item->SourceName.ToString()).ToLower();
		const bool bPassesEntryFilters = Mode != EDiurnalScheduleBrowserMode::CombinedSchedule
			|| !EntryFilterBar
			|| EntryFilterBar->GetAllActiveFilters()->PassesAllFilters(Item);
		bool bSearchMatches = true;
		for (const FString& Token : QueryTokens) bSearchMatches &= Haystack.Contains(Token);
		if (bPassesEntryFilters && bSearchMatches) FilteredItems.Add(Item);
	}

	FilteredItems.StableSort([this](
		const TSharedPtr<FDiurnalScheduleBrowserItem>& Left,
		const TSharedPtr<FDiurnalScheduleBrowserItem>& Right)
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
			const int32 Compare = Left->DisplayName.ToString().Compare(
				Right->DisplayName.ToString(), ESearchCase::IgnoreCase);
			if (Compare != 0) return Compare < 0;
			break;
		}
		case EDiurnalScheduleSortMode::Type:
			if (Left->bRange != Right->bRange) return !Left->bRange;
			if (Left->bOneOff != Right->bOneOff) return !Left->bOneOff;
			if (Left->Day != Right->Day) return Left->Day < Right->Day;
			if (Left->StartSeconds != Right->StartSeconds) return Left->StartSeconds < Right->StartSeconds;
			break;
		case EDiurnalScheduleSortMode::DayAndTime:
			if (Left->bOneOff != Right->bOneOff) return !Left->bOneOff;
			if (Left->bRange != Right->bRange) return !Left->bRange;
			if (Left->Day != Right->Day) return Left->Day < Right->Day;
			if (Left->StartSeconds != Right->StartSeconds) return Left->StartSeconds < Right->StartSeconds;
			break;
		case EDiurnalScheduleSortMode::TimeOfDay:
			if (Left->StartSeconds != Right->StartSeconds) return Left->StartSeconds < Right->StartSeconds;
			break;
		case EDiurnalScheduleSortMode::ManualOrder:
		default:
			break;
		}
		return TieBreak();
	});
	if (SelectedItem && !FilteredItems.Contains(SelectedItem))
	{
		SelectedItem.Reset();
		if (ListView) ListView->ClearSelection();
		RebuildInspector();
	}
	if (ListView) ListView->RequestListRefresh();
}

void SDiurnalScheduleBrowser::HandleEntryFilterChanged()
{
	SetSearchText(SearchText);
}

EVisibility SDiurnalScheduleBrowser::GetEntryFilterVisibility() const
{
	return Mode == EDiurnalScheduleBrowserMode::CombinedSchedule
		&& EntryFilterBar
		&& EntryFilterBar->HasAnyFilters()
		? EVisibility::Visible
		: EVisibility::Collapsed;
}

EVisibility SDiurnalScheduleBrowser::GetDefaultFilterVisibility() const
{
	return Mode == EDiurnalScheduleBrowserMode::CombinedSchedule
		? EVisibility::Visible
		: EVisibility::Collapsed;
}

TSharedRef<SWidget> SDiurnalScheduleBrowser::BuildSortMenu()
{
	FMenuBuilder Menu(true, nullptr);
	for (const EDiurnalScheduleSortMode Sort : {
		EDiurnalScheduleSortMode::ManualOrder,
		EDiurnalScheduleSortMode::Name,
		EDiurnalScheduleSortMode::Type,
		EDiurnalScheduleSortMode::DayAndTime,
		EDiurnalScheduleSortMode::TimeOfDay })
	{
		Menu.AddMenuEntry(
			DiurnalScheduleEditor::GetSortModeText(Sort), FText::GetEmpty(), FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SDiurnalScheduleBrowser::SetSortMode, Sort),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, Sort] { return SortMode == Sort; })),
			NAME_None, EUserInterfaceActionType::RadioButton);
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
	return FText::Format(
		LOCTEXT("SortValue", "Sort: {0}"),
		DiurnalScheduleEditor::GetSortModeText(SortMode));
}

TSharedRef<ITableRow> SDiurnalScheduleBrowser::GenerateRow(
	TSharedPtr<FDiurnalScheduleBrowserItem> Item,
	const TSharedRef<STableViewBase>& Owner)
{
	return SNew(SDiurnalScheduleBrowserRow, Owner)
		.Item(Item)
		.OnVerifyName(this, &SDiurnalScheduleBrowser::VerifyName)
		.OnNameCommitted_Lambda([this, Item](const FText& Text, const ETextCommit::Type CommitType)
		{
			CommitName(Text, CommitType, Item);
		});
}

void SDiurnalScheduleBrowser::SelectionChanged(
	TSharedPtr<FDiurnalScheduleBrowserItem> Item,
	ESelectInfo::Type)
{
	SelectedItem = MoveTemp(Item);
	if (!bRestoringSelection) RebuildInspector();
}

void SDiurnalScheduleBrowser::ItemScrolledIntoView(
	TSharedPtr<FDiurnalScheduleBrowserItem> Item,
	const TSharedPtr<ITableRow>&)
{
	if (!bPendingRename || !Item || Item != SelectedItem) return;
	bPendingRename = false;
	if (const TSharedPtr<SInlineEditableTextBlock> InlineName = Item->InlineName.Pin())
	{
		InlineName->EnterEditingMode();
	}
}

void SDiurnalScheduleBrowser::DoubleClick(TSharedPtr<FDiurnalScheduleBrowserItem> Item)
{
	SelectedItem = MoveTemp(Item);
	OpenSelectedSource();
}

TSharedPtr<SWidget> SDiurnalScheduleBrowser::BuildContextMenu()
{
	if (Mode == EDiurnalScheduleBrowserMode::ScheduleAssets && !GetEditingSchedule()) return nullptr;
	FMenuBuilder Menu(true, nullptr);
	Menu.BeginSection(TEXT("CreateEntry"), LOCTEXT("CreateEntrySection", "Add Entry"));
	if (UDiurnalSchedule* TargetSchedule = GetEditingSchedule())
	{
		Menu.AddMenuEntry(
			LOCTEXT("AddEventMenu", "Add Event"),
			FText::Format(LOCTEXT("AddEventMenuTip", "Add an event to {0}."), FText::FromString(TargetSchedule->GetName())),
			FSlateIcon(FDiurnalCycleEditorStyle::GetStyleSetName(), FDiurnalCycleEditorStyle::GetOccurrenceIconName(false)),
			FUIAction(FExecuteAction::CreateSP(this, &SDiurnalScheduleBrowser::AddEntryToSchedule, TargetSchedule, false)));
		Menu.AddMenuEntry(
			LOCTEXT("AddRangeMenu", "Add Time Range"),
			FText::Format(LOCTEXT("AddRangeMenuTip", "Add a time range to {0}."), FText::FromString(TargetSchedule->GetName())),
			FSlateIcon(FDiurnalCycleEditorStyle::GetStyleSetName(), FName("DiurnalCycle.Entry.Range")),
			FUIAction(FExecuteAction::CreateSP(this, &SDiurnalScheduleBrowser::AddEntryToSchedule, TargetSchedule, true)));
	}
	else
	{
		Menu.AddSubMenu(
			LOCTEXT("AddEventToMenu", "Add Event To"),
			LOCTEXT("AddEventToMenuTip", "Choose a default schedule for the new event."),
			FNewMenuDelegate::CreateSP(this, &SDiurnalScheduleBrowser::PopulateAddTargetMenu, false),
			false,
			FSlateIcon(FDiurnalCycleEditorStyle::GetStyleSetName(), FDiurnalCycleEditorStyle::GetOccurrenceIconName(false)));
		Menu.AddSubMenu(
			LOCTEXT("AddRangeToMenu", "Add Time Range To"),
			LOCTEXT("AddRangeToMenuTip", "Choose a default schedule for the new time range."),
			FNewMenuDelegate::CreateSP(this, &SDiurnalScheduleBrowser::PopulateAddTargetMenu, true),
			false,
			FSlateIcon(FDiurnalCycleEditorStyle::GetStyleSetName(), FName("DiurnalCycle.Entry.Range")));
	}
	Menu.EndSection();

	if (SelectedItem)
	{
		Menu.BeginSection(TEXT("EntryActions"), LOCTEXT("EntryActionsSection", "Entry"));
		Menu.AddMenuEntry(LOCTEXT("RenameMenu", "Rename"), LOCTEXT("RenameMenuTip", "Rename this authored entry."), FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Edit")),
			FUIAction(FExecuteAction::CreateLambda([this] { RequestRenameSelected(); }), FCanExecuteAction::CreateSP(this, &SDiurnalScheduleBrowser::CanEditSelected)));
		Menu.AddMenuEntry(LOCTEXT("DuplicateMenu", "Duplicate"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Duplicate")),
			FUIAction(FExecuteAction::CreateLambda([this] { DuplicateSelected(); }), FCanExecuteAction::CreateSP(this, &SDiurnalScheduleBrowser::CanEditSelected)));
		Menu.AddMenuEntry(LOCTEXT("DeleteMenu", "Delete"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Delete")),
			FUIAction(FExecuteAction::CreateLambda([this] { DeleteSelected(); }), FCanExecuteAction::CreateSP(this, &SDiurnalScheduleBrowser::CanEditSelected)));
		Menu.EndSection();
		Menu.BeginSection(TEXT("Source"), LOCTEXT("SourceSection", "Source"));
		Menu.AddMenuEntry(LOCTEXT("OpenSourceMenu", "Open Source Schedule"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.FolderOpen")),
			FUIAction(FExecuteAction::CreateLambda([this] { OpenSelectedSource(); }), FCanExecuteAction::CreateSP(this, &SDiurnalScheduleBrowser::CanOpenSelectedSource)));
		Menu.EndSection();
	}
	return Menu.MakeWidget();
}

void SDiurnalScheduleBrowser::PopulateAddTargetMenu(FMenuBuilder& Menu, const bool bRange)
{
	const UDiurnalCycleSettings* Settings = GetDefault<UDiurnalCycleSettings>();
	TSet<FSoftObjectPath> AddedPaths;
	for (const TSoftObjectPtr<UDiurnalSchedule>& Reference : Settings->DefaultSchedules)
	{
		const FSoftObjectPath Path = Reference.ToSoftObjectPath();
		if (Path.IsNull() || AddedPaths.Contains(Path)) continue;
		AddedPaths.Add(Path);
		if (UDiurnalSchedule* Schedule = Reference.LoadSynchronous())
		{
			Menu.AddMenuEntry(
				FText::FromString(Schedule->GetName()),
				FText::Format(
					bRange ? LOCTEXT("AddRangeTargetTip", "Add a time range to {0}.") : LOCTEXT("AddEventTargetTip", "Add an event to {0}."),
					FText::FromString(Schedule->GetPathName())),
				FSlateIcon(FDiurnalCycleEditorStyle::GetStyleSetName(), TEXT("DiurnalCycle.ScheduleIcon")),
				FUIAction(FExecuteAction::CreateSP(this, &SDiurnalScheduleBrowser::AddEntryToSchedule, Schedule, bRange)));
		}
	}
	if (!AddedPaths.IsEmpty()) Menu.AddMenuSeparator();
	Menu.AddMenuEntry(
		LOCTEXT("CreateDefaultSchedule", "Create New Default Schedule…"),
		LOCTEXT("CreateDefaultScheduleTip", "Create a default schedule for the new entry."),
		FSlateIcon(FDiurnalCycleEditorStyle::GetStyleSetName(), TEXT("DiurnalCycle.ScheduleIcon")),
		FUIAction(FExecuteAction::CreateSP(this, &SDiurnalScheduleBrowser::CreateDefaultScheduleWithEntry, bRange)));
}

FReply SDiurnalScheduleBrowser::HandleListKeyDown(const FGeometry&, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey() == EKeys::F2 && CanEditSelected())
	{
		RequestRenameSelected();
		return FReply::Handled();
	}
	if (KeyEvent.GetKey() == EKeys::Delete && CanEditSelected())
	{
		DeleteSelected();
		return FReply::Handled();
	}
	if (KeyEvent.IsControlDown() && KeyEvent.GetKey() == EKeys::D && CanEditSelected())
	{
		DuplicateSelected();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

bool SDiurnalScheduleBrowser::VerifyName(const FText& Text, FText& OutError) const
{
	const FString Candidate = Text.ToString().TrimStartAndEnd();
	if (Candidate.IsEmpty() || FName(*Candidate).IsNone())
	{
		OutError = LOCTEXT("NameRequired", "Enter a name other than None.");
		return false;
	}
	return true;
}

void SDiurnalScheduleBrowser::CommitName(
	const FText& Text,
	const ETextCommit::Type CommitType,
	TSharedPtr<FDiurnalScheduleBrowserItem> Item)
{
	if (CommitType == ETextCommit::OnCleared || !Item || !Item->Source.IsValid()) return;
	const FString Candidate = Text.ToString().TrimStartAndEnd();
	if (Candidate.IsEmpty() || FName(*Candidate).IsNone()) return;
	TSharedPtr<FDiurnalScheduleEditorModel> Model = MakeShared<FDiurnalScheduleEditorModel>(Item->Source.Get());
	Model->SelectEntry(Item->GetSelectionType(), Item->EntryId);
	Model->RenameSelected(FName(*Candidate));
	SelectExact(Item->Source.Get(), Item->EntryId);
	Rebuild();
}

void SDiurnalScheduleBrowser::RequestRenameSelected()
{
	if (!SelectedItem) return;
	if (const TSharedPtr<SInlineEditableTextBlock> InlineName = SelectedItem->InlineName.Pin())
	{
		InlineName->EnterEditingMode();
	}
	else if (ListView)
	{
		bPendingRename = true;
		ListView->RequestScrollIntoView(SelectedItem);
	}
}

void SDiurnalScheduleBrowser::RebuildInspector()
{
	if (!InspectorHost) return;
	UDiurnalSchedule* Schedule = SelectedItem && SelectedItem->Source.IsValid()
		? SelectedItem->Source.Get()
		: Mode == EDiurnalScheduleBrowserMode::ScheduleAssets ? SelectedAsset.Get() : nullptr;
	if (!Schedule)
	{
		InspectorModel.Reset();
		InspectorHost->SetContent(
			SNew(SBorder)
			.Padding(16)
			.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
			[
				SNew(STextBlock)
				.Text(LOCTEXT("InspectorEmpty", "Select an entry to edit it."))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]);
		return;
	}

	InspectorModel = MakeShared<FDiurnalScheduleEditorModel>(Schedule);
	if (SelectedItem)
	{
		InspectorModel->SelectEntry(SelectedItem->GetSelectionType(), SelectedItem->EntryId);
	}
	InspectorHost->SetContent(
		SNew(SDiurnalScheduleInspector)
		.Model(InspectorModel)
		.ShowCreationActions(false));
}

void SDiurnalScheduleBrowser::SelectExact(
	UDiurnalSchedule* Source,
	const FGuid EntryId,
	const bool bRequestRename)
{
	PendingSelectionSource = Source;
	PendingSelectionId = EntryId;
	bPendingRename = bRequestRename;
}

void SDiurnalScheduleBrowser::AssetSelected(const FAssetData& AssetData)
{
	SelectedAsset = Cast<UDiurnalSchedule>(AssetData.GetAsset());
	SelectedItem.Reset();
	Rebuild();
}

void SDiurnalScheduleBrowser::AssetDoubleClicked(const FAssetData& AssetData)
{
	if (GEditor)
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(AssetData.GetSoftObjectPath());
	}
}

void SDiurnalScheduleBrowser::ExtendAssetPickerTopBar(const TSharedRef<SHorizontalBox> TopBar)
{
	TopBar->InsertSlot(0)
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 4, 0)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ToolTipText(LOCTEXT("CreateScheduleAssetTip", "Create a Day/Night Cycle schedule asset."))
			.OnClicked(this, &SDiurnalScheduleBrowser::CreateScheduleAssetFromPicker)
			[SNew(SImage).Image(FDiurnalCycleEditorStyle::Get().GetBrush("DiurnalCycle.ScheduleIcon"))]
		];
}

TSharedPtr<SWidget> SDiurnalScheduleBrowser::BuildAssetContextMenu(
	const TArray<FAssetData>& SelectedAssets)
{
	FMenuBuilder Menu(true, nullptr);
	Menu.BeginSection(TEXT("ScheduleCreate"), LOCTEXT("ScheduleCreateSection", "Schedule"));
	Menu.AddMenuEntry(
		LOCTEXT("CreateScheduleAsset", "Create Schedule…"),
		LOCTEXT("CreateScheduleAssetMenuTip", "Create a Day/Night Cycle schedule asset."),
		FSlateIcon(FDiurnalCycleEditorStyle::GetStyleSetName(), TEXT("DiurnalCycle.ScheduleIcon")),
		FUIAction(FExecuteAction::CreateLambda([this] { CreateScheduleAssetFromPicker(); })));
	Menu.EndSection();

	if (!SelectedAssets.IsEmpty())
	{
		Menu.BeginSection(TEXT("ScheduleAssetActions"), LOCTEXT("ScheduleAssetActionsSection", "Asset"));
		Menu.AddMenuEntry(
			LOCTEXT("OpenScheduleAsset", "Open"),
			LOCTEXT("OpenScheduleAssetTip", "Open the selected schedule in its editor."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.FolderOpen")),
			FUIAction(FExecuteAction::CreateSP(this, &SDiurnalScheduleBrowser::OpenScheduleAssets, SelectedAssets)));
		if (SelectedAssets.Num() == 1)
		{
			Menu.AddMenuEntry(
				LOCTEXT("RenameScheduleAsset", "Rename"),
				LOCTEXT("RenameScheduleAssetTip", "Rename the selected schedule asset in place."),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Edit")),
				FUIAction(FExecuteAction::CreateSP(this, &SDiurnalScheduleBrowser::RenameSelectedScheduleAsset)));
			Menu.AddMenuEntry(
				LOCTEXT("DuplicateScheduleAsset", "Duplicate…"),
				LOCTEXT("DuplicateScheduleAssetTip", "Duplicate the selected schedule asset."),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Duplicate")),
				FUIAction(FExecuteAction::CreateSP(this, &SDiurnalScheduleBrowser::DuplicateScheduleAsset, SelectedAssets[0])));
		}
		Menu.AddMenuEntry(
			LOCTEXT("DeleteScheduleAsset", "Delete"),
			LOCTEXT("DeleteScheduleAssetTip", "Delete the selected schedule asset after Unreal's reference check."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Delete")),
			FUIAction(FExecuteAction::CreateSP(this, &SDiurnalScheduleBrowser::DeleteScheduleAssets, SelectedAssets)));
		Menu.EndSection();
	}
	return Menu.MakeWidget();
}

FReply SDiurnalScheduleBrowser::CreateScheduleAssetFromPicker()
{
	if (UDiurnalSchedule* Schedule = CreateScheduleAsset())
	{
		SelectedAsset = Schedule;
		SelectedItem.Reset();
		RebuildBody();
		Rebuild();
	}
	return FReply::Handled();
}

UDiurnalSchedule* SDiurnalScheduleBrowser::CreateScheduleAsset()
{
	TStrongObjectPtr<UDiurnalScheduleFactory> Factory(
		NewObject<UDiurnalScheduleFactory>(GetTransientPackage()));
	return Cast<UDiurnalSchedule>(FAssetToolsModule::GetModule().Get().CreateAssetWithDialog(
		UDiurnalSchedule::StaticClass(), Factory.Get(), TEXT("DiurnalScheduleBrowser")));
}

void SDiurnalScheduleBrowser::RenameSelectedScheduleAsset()
{
	if (!AssetPickerWidget) return;
	FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"))
		.Get()
		.ExecuteRename(AssetPickerWidget);
}

void SDiurnalScheduleBrowser::DuplicateScheduleAsset(const FAssetData SourceAsset)
{
	UDiurnalSchedule* Source = Cast<UDiurnalSchedule>(SourceAsset.GetAsset());
	if (!Source) return;
	IAssetTools& AssetTools = FAssetToolsModule::GetModule().Get();
	FString PackageName;
	FString AssetName;
	AssetTools.CreateUniqueAssetName(Source->GetOutermost()->GetName(), TEXT("_Copy"), PackageName, AssetName);
	if (UDiurnalSchedule* Duplicate = Cast<UDiurnalSchedule>(AssetTools.DuplicateAssetWithDialog(
		AssetName, FPackageName::GetLongPackagePath(PackageName), Source)))
	{
		SelectedAsset = Duplicate;
		SelectedItem.Reset();
		RebuildBody();
		Rebuild();
	}
}

void SDiurnalScheduleBrowser::DeleteScheduleAssets(TArray<FAssetData> Assets)
{
	TArray<UObject*> Objects;
	for (const FAssetData& Asset : Assets)
	{
		if (UDiurnalSchedule* Schedule = Cast<UDiurnalSchedule>(Asset.GetAsset())) Objects.Add(Schedule);
	}
	if (Objects.IsEmpty()) return;
	const bool bSelectedAssetWillBeDeleted = SelectedAsset.IsValid() && Objects.Contains(SelectedAsset.Get());
	if (AssetViewUtils::DeleteAssets(Objects) > 0)
	{
		if (bSelectedAssetWillBeDeleted)
		{
			SelectedAsset.Reset();
			SelectedItem.Reset();
		}
		Rebuild();
	}
}

void SDiurnalScheduleBrowser::OpenScheduleAssets(TArray<FAssetData> Assets)
{
	if (!GEditor) return;
	TArray<UObject*> Objects;
	for (const FAssetData& Asset : Assets)
	{
		if (UObject* Object = Asset.GetAsset()) Objects.Add(Object);
	}
	if (!Objects.IsEmpty())
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(Objects);
	}
}

UDiurnalSchedule* SDiurnalScheduleBrowser::GetEditingSchedule() const
{
	if (SelectedItem && SelectedItem->Source.IsValid()) return SelectedItem->Source.Get();
	return Mode == EDiurnalScheduleBrowserMode::ScheduleAssets ? SelectedAsset.Get() : nullptr;
}

TSharedPtr<FDiurnalScheduleEditorModel> SDiurnalScheduleBrowser::MakeEditingModel() const
{
	UDiurnalSchedule* Schedule = GetEditingSchedule();
	if (!Schedule) return nullptr;
	TSharedPtr<FDiurnalScheduleEditorModel> Model = MakeShared<FDiurnalScheduleEditorModel>(Schedule);
	if (SelectedItem)
	{
		Model->SelectEntry(SelectedItem->GetSelectionType(), SelectedItem->EntryId);
	}
	return Model;
}

void SDiurnalScheduleBrowser::AddEntryToSchedule(
	UDiurnalSchedule* Schedule,
	const bool bRange)
{
	if (!Schedule) return;
	TSharedPtr<FDiurnalScheduleEditorModel> Model = MakeShared<FDiurnalScheduleEditorModel>(Schedule);
	const FGuid EntryId = bRange ? Model->AddRange() : Model->AddRepeatingEvent();
	if (!EntryId.IsValid()) return;
	SelectedAsset = Schedule;
	SelectExact(Schedule, EntryId, true);
	Rebuild();
}

void SDiurnalScheduleBrowser::CreateDefaultScheduleWithEntry(const bool bRange)
{
	UDiurnalSchedule* Schedule = CreateScheduleAsset();
	if (!Schedule) return;
	UDiurnalCycleSettings* Settings = GetMutableDefault<UDiurnalCycleSettings>();
	{
		const FScopedTransaction Transaction(LOCTEXT("AddDefaultSchedule", "Add Default Schedule"));
		Settings->Modify();
		Settings->DefaultSchedules.AddUnique(Schedule);
		Settings->SaveConfig();
		Settings->PostEditChange();
	}
	AddEntryToSchedule(Schedule, bRange);
}

FReply SDiurnalScheduleBrowser::DuplicateSelected()
{
	if (TSharedPtr<FDiurnalScheduleEditorModel> Model = MakeEditingModel(); Model && Model->DuplicateSelected())
	{
		SelectExact(Model->GetSchedule(), Model->GetSelectedId(), true);
		Rebuild();
	}
	return FReply::Handled();
}

FReply SDiurnalScheduleBrowser::DeleteSelected()
{
	if (TSharedPtr<FDiurnalScheduleEditorModel> Model = MakeEditingModel(); Model && Model->DeleteSelected())
	{
		SelectedItem.Reset();
		Rebuild();
	}
	return FReply::Handled();
}

FReply SDiurnalScheduleBrowser::OpenSelectedSource()
{
	if (!GEditor || !SelectedItem || !SelectedItem->Source.IsValid()) return FReply::Handled();
	UDiurnalSchedule* Source = SelectedItem->Source.Get();
	UAssetEditorSubsystem* AssetEditors = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	AssetEditors->OpenEditorForAsset(Source);
	if (IAssetEditorInstance* Instance = AssetEditors->FindEditorForAsset(Source, true);
		Instance && Instance->GetEditorName() == FName(TEXT("DiurnalScheduleEditor")))
	{
		static_cast<FDiurnalScheduleEditorToolkit*>(Instance)->FocusEntry(
			SelectedItem->GetSelectionType(), SelectedItem->EntryId);
	}
	return FReply::Handled();
}

FReply SDiurnalScheduleBrowser::RefreshBrowser()
{
	Rebuild();
	return FReply::Handled();
}

FReply SDiurnalScheduleBrowser::SaveSchedules()
{
	TArray<UPackage*> Packages;
	GatherDirtySchedulePackages(Packages);
	if (!Packages.IsEmpty())
	{
		FEditorFileUtils::PromptForCheckoutAndSave(Packages, true, false);
	}
	return FReply::Handled();
}

bool SDiurnalScheduleBrowser::CanSaveSchedules() const
{
	TArray<UPackage*> Packages;
	GatherDirtySchedulePackages(Packages);
	return !Packages.IsEmpty();
}

void SDiurnalScheduleBrowser::GatherDirtySchedulePackages(TArray<UPackage*>& OutPackages) const
{
	TSet<UPackage*> UniquePackages;
	const auto AddSchedule = [&UniquePackages](const UDiurnalSchedule* Schedule)
	{
		if (Schedule && Schedule->GetOutermost()->IsDirty())
		{
			UniquePackages.Add(Schedule->GetOutermost());
		}
	};
	for (const TWeakObjectPtr<UDiurnalSchedule>& Schedule : TouchedSchedules)
	{
		AddSchedule(Schedule.Get());
	}
	for (const TSharedPtr<FDiurnalScheduleBrowserItem>& Item : AllItems)
	{
		if (Item) AddSchedule(Item->Source.Get());
	}
	AddSchedule(SelectedAsset.Get());
	OutPackages = UniquePackages.Array();
}

FReply SDiurnalScheduleBrowser::OpenProjectSettings()
{
	FDiurnalCycleEditorModule::OpenProjectSettings();
	return FReply::Handled();
}

bool SDiurnalScheduleBrowser::CanEditSelected() const
{
	return SelectedItem && SelectedItem->Source.IsValid();
}

bool SDiurnalScheduleBrowser::CanOpenSelectedSource() const
{
	return SelectedItem && SelectedItem->Source.IsValid();
}

FText SDiurnalScheduleBrowser::GetEmptyText() const
{
	if (Mode == EDiurnalScheduleBrowserMode::ScheduleAssets && !SelectedAsset.IsValid())
	{
		return LOCTEXT("SelectAssetEmpty", "Select a schedule asset to browse and edit its entries.");
	}
	if (bInvalidConfiguration)
	{
		return LOCTEXT("InvalidEmpty", "Default Schedules contains the same schedule more than once. Remove the duplicate in Project Settings.");
	}
	if (AllItems.IsEmpty() && Mode == EDiurnalScheduleBrowserMode::CombinedSchedule)
	{
		return LOCTEXT("EmptyDefaults", "No default entries. Right-click here to add one to an existing or new default schedule.");
	}
	if (AllItems.IsEmpty())
	{
		return LOCTEXT("EmptySchedule", "This schedule has no entries. Right-click here to add one.");
	}
	return Mode == EDiurnalScheduleBrowserMode::CombinedSchedule
		? LOCTEXT("EmptyFilteredDefaults", "No default entries match the current search and filters.")
		: LOCTEXT("EmptyFilteredSchedule", "No entries match the current search.");
}

EVisibility SDiurnalScheduleBrowser::GetEmptyVisibility() const
{
	return FilteredItems.IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed;
}

void SDiurnalScheduleBrowser::HandleObjectPropertyChanged(
	UObject* Object,
	FPropertyChangedEvent& PropertyChangedEvent)
{
	if (Object != GetMutableDefault<UDiurnalCycleSettings>())
	{
		const UDiurnalSchedule* ChangedSchedule = Cast<UDiurnalSchedule>(Object);
		if (!ChangedSchedule || !IsRelevantSchedulePath(FSoftObjectPath(ChangedSchedule))) return;
		TouchedSchedules.Add(const_cast<UDiurnalSchedule*>(ChangedSchedule));
		Rebuild();
		return;
	}
	Rebuild();
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
			if (SelectedAsset.Get() == PreviousSchedule) SelectedAsset = const_cast<UDiurnalSchedule*>(ReplacementSchedule);
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

void SDiurnalScheduleBrowser::HandleScheduleAssetRenamed(
	const FAssetData& AssetData,
	const FString& OldObjectPath)
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
	if (ObjectPath.IsNull()) return false;
	if (SelectedAsset.IsValid() && FSoftObjectPath(SelectedAsset.Get()) == ObjectPath) return true;
	const UDiurnalCycleSettings* Settings = GetDefault<UDiurnalCycleSettings>();
	if (Settings->DefaultSchedules.ContainsByPredicate([&](const TSoftObjectPtr<UDiurnalSchedule>& Reference)
	{
		return Reference.ToSoftObjectPath() == ObjectPath;
	}))
	{
		return true;
	}
	return AllItems.ContainsByPredicate([&](const TSharedPtr<FDiurnalScheduleBrowserItem>& Item)
	{
		return Item && Item->Source.IsValid() && FSoftObjectPath(Item->Source.Get()) == ObjectPath;
	});
}

#undef LOCTEXT_NAMESPACE
