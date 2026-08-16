#include "ScheduleEditor/Widgets/SDiurnalScheduleWorkspace.h"

#include "DiurnalCycleEditorStyle.h"
#include "DiurnalCycleSettings.h"
#include "ScheduleEditor/DiurnalScheduleEditorModel.h"
#include "ScheduleEditor/DiurnalTimelineRangeController.h"
#include "ScheduleEditor/Widgets/SDiurnalScheduleOverview.h"
#include "ScheduleEditor/Widgets/SDiurnalScheduleWeekView.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ISequencerWidgetsModule.h"
#include "Misc/ConfigCacheIni.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SDiurnalScheduleWorkspace"

namespace
{
	const TCHAR* ViewSettingsSection = TEXT("DiurnalScheduleEditor");
	const TCHAR* ViewSettingsKey = TEXT("LastView");
	const TCHAR* SortSettingsKey = TEXT("LastListSort");

	TSharedRef<SWidget> ViewToggleContent(const FName Brush, const FText& Label)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
			[
				SNew(SImage)
				.Image(FDiurnalCycleEditorStyle::Get().GetBrush(Brush))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(Label)
			];
	}
}

SDiurnalScheduleWorkspace::~SDiurnalScheduleWorkspace()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(SettingsChangedHandle);
}

void SDiurnalScheduleWorkspace::Construct(const FArguments& Args)
{
	Model = Args._Model;
	check(Model.IsValid());
	SettingsChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(
		this, &SDiurnalScheduleWorkspace::HandleSettingsChanged);
	FString SavedView;
	if (GConfig && GConfig->GetString(ViewSettingsSection, ViewSettingsKey, SavedView, GEditorPerProjectIni))
	{
		ViewMode = SavedView == TEXT("List") ? EDiurnalScheduleEditorViewMode::List : EDiurnalScheduleEditorViewMode::Timeline;
	}
	int32 SavedSort = static_cast<int32>(EDiurnalScheduleSortMode::ManualOrder);
	if (GConfig && GConfig->GetInt(ViewSettingsSection, SortSettingsKey, SavedSort, GEditorPerProjectIni)
		&& SavedSort >= static_cast<int32>(EDiurnalScheduleSortMode::ManualOrder)
		&& SavedSort <= static_cast<int32>(EDiurnalScheduleSortMode::TimeOfDay))
	{
		Model->SetSortMode(static_cast<EDiurnalScheduleSortMode>(SavedSort));
	}

	ListView = SNew(SDiurnalScheduleOverview).Model(Model).Commands(Args._Commands);
	WeekView = SNew(SDiurnalScheduleWeekView).Model(Model);

	ISequencerWidgetsModule& SequencerWidgets =
		FModuleManager::LoadModuleChecked<ISequencerWidgetsModule>(TEXT("SequencerWidgets"));
	const TSharedRef<ITimeSliderController> RangeController = WeekView->GetTimelineRangeController();
	const TSharedRef<SWidget> TimelineRangeSlider =
		SequencerWidgets.CreateTimeRangeSlider(RangeController);
	StaticCastSharedRef<FDiurnalTimelineRangeController>(RangeController)->SetRangeWidget(TimelineRangeSlider);
	const auto MakeDayBound = [](const FText& Label, const FText& Tooltip,
		const TAttribute<int32>& Value, TFunction<void(int32)> OnChanged)
	{
		return SNew(SBox)
			.MinDesiredWidth(64.0f)
			.HAlign(HAlign_Center)
			.ToolTipText(FText::Format(LOCTEXT("DayBoundTooltip", "{0}\n{1}"), Label, Tooltip))
			[
				SNew(SSpinBox<int32>)
				.Style(&FAppStyle::Get().GetWidgetStyle<FSpinBoxStyle>("Sequencer.HyperlinkSpinBox"))
				.Value(Value)
				.MinValue(TOptional<int32>())
				.MaxValue(TOptional<int32>())
				.Delta(1)
				.ClearKeyboardFocusOnCommit(true)
				.LinearDeltaSensitivity(25)
				.OnValueChanged_Lambda([Action = MoveTemp(OnChanged)](const int32 NewValue)
				{
					Action(NewValue);
				})
			];
	};
	const TSharedRef<SWidget> WorkingStartControl = MakeDayBound(
		LOCTEXT("WorkingStart", "Working Start"),
		LOCTEXT("WorkingStartTip", "First editor navigation day. This does not affect the schedule."),
		TAttribute<int32>::CreateLambda([this] { return WeekView->GetWorkingFirstDay(); }),
		[this, RangeController](const int32 Day)
		{
			RangeController->SetClampRange(
				Day, WeekView->GetWorkingFirstDay() + WeekView->GetWorkingDayCount());
		});
	const TSharedRef<SWidget> ViewStartControl = MakeDayBound(
		LOCTEXT("ViewStart", "View Start"),
		LOCTEXT("ViewStartTip", "First visible day."),
		TAttribute<int32>::CreateLambda([this] { return WeekView->GetFirstVisibleDay(); }),
		[this, RangeController](const int32 Day)
		{
			RangeController->SetViewRange(
				Day, WeekView->GetFirstVisibleDay() + WeekView->GetVisibleSpanDays(),
				EViewRangeInterpolation::Immediate);
		});
	const TSharedRef<SWidget> ViewEndControl = MakeDayBound(
		LOCTEXT("ViewEnd", "View End"),
		LOCTEXT("ViewEndTip", "Last visible day."),
		TAttribute<int32>::CreateLambda([this]
		{
			return WeekView->GetFirstVisibleDay() + WeekView->GetVisibleSpanDays() - 1;
		}),
		[this, RangeController](const int32 Day)
		{
			RangeController->SetViewRange(
				WeekView->GetFirstVisibleDay(), static_cast<double>(Day) + 1.0,
				EViewRangeInterpolation::Immediate);
		});
	const TSharedRef<SWidget> WorkingEndControl = MakeDayBound(
		LOCTEXT("WorkingEnd", "Working End"),
		LOCTEXT("WorkingEndTip", "Last editor navigation day. This does not affect the schedule."),
		TAttribute<int32>::CreateLambda([this]
		{
			return WeekView->GetWorkingFirstDay() + WeekView->GetWorkingDayCount() - 1;
		}),
		[this, RangeController](const int32 Day)
		{
			RangeController->SetClampRange(
				WeekView->GetWorkingFirstDay(), static_cast<double>(Day) + 1.0);
		});

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(FMargin(8, 5))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 2, 0)
				[
					SNew(SCheckBox).Style(FAppStyle::Get(), "ToggleButtonCheckbox")
					.IsChecked(this, &SDiurnalScheduleWorkspace::IsViewChecked, EDiurnalScheduleEditorViewMode::List)
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { if (State == ECheckBoxState::Checked) SetViewMode(EDiurnalScheduleEditorViewMode::List); })
					[ViewToggleContent(TEXT("DiurnalCycle.View.List"), LOCTEXT("ListMode", "List"))]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
				[
					SNew(SCheckBox).Style(FAppStyle::Get(), "ToggleButtonCheckbox")
					.IsChecked(this, &SDiurnalScheduleWorkspace::IsViewChecked, EDiurnalScheduleEditorViewMode::Timeline)
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { if (State == ECheckBoxState::Checked) SetViewMode(EDiurnalScheduleEditorViewMode::Timeline); })
					[ViewToggleContent(TEXT("DiurnalCycle.View.Timeline"), LOCTEXT("TimelineMode", "Timeline"))]
				]
				+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
				[
					SAssignNew(SearchBox, SSearchBox)
					.HintText(LOCTEXT("SearchHint", "Search name, tags, type, recurrence, behavior, or source…"))
					.OnTextChanged(this, &SDiurnalScheduleWorkspace::SetSearchText)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0, 0, 0)
				[
					SNew(SComboButton)
					.Visibility(this, &SDiurnalScheduleWorkspace::GetListControlsVisibility)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.OnGetMenuContent(this, &SDiurnalScheduleWorkspace::BuildSortMenu)
					.ToolTipText(LOCTEXT("SortTip", "Choose a display-only order. Authored arrays are never reordered by sorting."))
					.ButtonContent()
					[
						SNew(STextBlock).Text(this, &SDiurnalScheduleWorkspace::GetSortText)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 0, 0)
				[
					SNew(SComboButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.OnGetMenuContent(this, &SDiurnalScheduleWorkspace::BuildFilterMenu)
					.ButtonContent()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
						[
							SNew(SImage).Image(FAppStyle::GetBrush("Icons.Adjust"))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(this, &SDiurnalScheduleWorkspace::GetFilterText)
						]
					]
				]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1)
		[
			SAssignNew(ViewSwitcher, SWidgetSwitcher)
			.WidgetIndex(ViewMode == EDiurnalScheduleEditorViewMode::Timeline ? 1 : 0)
			+ SWidgetSwitcher::Slot()
			[
				ListView.ToSharedRef()
			]
			+ SWidgetSwitcher::Slot()
			[
				WeekView.ToSharedRef()
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder).BorderImage(FAppStyle::GetBrush("Brushes.Recessed")).Padding(FMargin(8, 3))
			.Visibility(this, &SDiurnalScheduleWorkspace::GetTimelineControlsVisibility)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 3)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)[SNew(SButton).Text(LOCTEXT("Current", "Current")).ToolTipText(LOCTEXT("CurrentTip", "Focus the live PIE day, or the configured starting day.")).OnClicked(this, &SDiurnalScheduleWorkspace::CurrentTimeline)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,10,0)[SNew(STextBlock).Text(this, &SDiurnalScheduleWorkspace::GetRuntimeMarkerText).Font(FAppStyle::GetFontStyle("SmallFont")).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 10, 0)[SNew(STextBlock).Text(this, &SDiurnalScheduleWorkspace::GetTimelineRangeText).Font(FAppStyle::GetFontStyle("NormalFontBold"))]
					+ SHorizontalBox::Slot().FillWidth(1)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10,0,4,0)[SNew(STextBlock).Text(LOCTEXT("HourHeight", "↕ Hour Height")).Font(FAppStyle::GetFontStyle("SmallFont"))]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(110)[SNew(SSlider).MinValue(24).MaxValue(120).Value(this, &SDiurnalScheduleWorkspace::GetHourHeight).OnValueChanged(this, &SDiurnalScheduleWorkspace::SetHourHeight)]]
					+ SHorizontalBox::Slot().AutoWidth().Padding(10,0,0,0)[SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").Text(LOCTEXT("OneDay", "1 Day")).OnClicked(this, &SDiurnalScheduleWorkspace::SetVisibleDayPreset, 1)]
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").Text(LOCTEXT("SevenDays", "7 Days")).OnClicked(this, &SDiurnalScheduleWorkspace::SetVisibleDayPreset, 7)]
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").Text(LOCTEXT("FourteenDays", "14 Days")).OnClicked(this, &SDiurnalScheduleWorkspace::SetVisibleDayPreset, 14)]
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").Text(LOCTEXT("Reset", "Reset")).ToolTipText(LOCTEXT("ResetTimelineTip", "Restore the default ranges, hour height, and vertical position.")).OnClicked(this, &SDiurnalScheduleWorkspace::ResetTimelineView)]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(0, 0, 6, 0)[WorkingStartControl]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(0, 0, 6, 0)[ViewStartControl]
						+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Bottom).Padding(2, 0, 2, 4)[TimelineRangeSlider]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(6, 0, 0, 0)[ViewEndControl]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(6, 0, 0, 0)[WorkingEndControl]
					]
				]
			]
		]
	];
	NormalizeSelectionForView();
}

void SDiurnalScheduleWorkspace::HandleSettingsChanged(
	UObject* Object,
	FPropertyChangedEvent&)
{
	if (Object == GetMutableDefault<UDiurnalCycleSettings>())
	{
		Model->NotifyInteractiveValueChanged();
	}
}

void SDiurnalScheduleWorkspace::RequestRenameSelected()
{
	if (ViewMode != EDiurnalScheduleEditorViewMode::List) SetViewMode(EDiurnalScheduleEditorViewMode::List);
	if (ListView) ListView->RequestRenameSelected();
}

void SDiurnalScheduleWorkspace::FocusEntry(
	const EDiurnalScheduleSelectionType Type,
	const FGuid EntryId)
{
	SetViewMode(EDiurnalScheduleEditorViewMode::List);
	Model->SelectEntry(Type, EntryId);
}

FReply SDiurnalScheduleWorkspace::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		ClearSelectionForCurrentView();
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

void SDiurnalScheduleWorkspace::SetViewMode(const EDiurnalScheduleEditorViewMode InViewMode)
{
	ViewMode = InViewMode;
	if (ViewSwitcher) ViewSwitcher->SetActiveWidgetIndex(ViewMode == EDiurnalScheduleEditorViewMode::Timeline ? 1 : 0);
	NormalizeSelectionForView();
	if (GConfig) GConfig->SetString(ViewSettingsSection, ViewSettingsKey, ViewMode == EDiurnalScheduleEditorViewMode::Timeline ? TEXT("Timeline") : TEXT("List"), GEditorPerProjectIni);
}

void SDiurnalScheduleWorkspace::ClearSelectionForCurrentView()
{
	if (ViewMode == EDiurnalScheduleEditorViewMode::Timeline)
	{
		Model->ClearEntrySelection();
		if (WeekView) WeekView->Activate();
	}
	else
	{
		Model->ClearAllSelection();
	}
}

void SDiurnalScheduleWorkspace::NormalizeSelectionForView()
{
	if (ViewMode == EDiurnalScheduleEditorViewMode::Timeline)
	{
		if (WeekView) WeekView->Activate();
		return;
	}
	if (Model->GetSelectionType() == EDiurnalScheduleSelectionType::None)
	{
		Model->ClearAllSelection();
	}
	else if (Model->GetSelectedDay() != INDEX_NONE)
	{
		Model->SelectEntry(Model->GetSelectionType(), Model->GetSelectedId());
	}
}

TSharedRef<SWidget> SDiurnalScheduleWorkspace::BuildSortMenu()
{
	FMenuBuilder Menu(true, nullptr);
	const auto AddMode = [this, &Menu](const EDiurnalScheduleSortMode Mode)
	{
		Menu.AddMenuEntry(
			DiurnalScheduleEditor::GetSortModeText(Mode),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SDiurnalScheduleWorkspace::SetSortMode, Mode),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, Mode] { return Model->GetSortMode() == Mode; })),
			NAME_None,
			EUserInterfaceActionType::RadioButton);
	};
	AddMode(EDiurnalScheduleSortMode::ManualOrder);
	AddMode(EDiurnalScheduleSortMode::Name);
	AddMode(EDiurnalScheduleSortMode::Type);
	AddMode(EDiurnalScheduleSortMode::DayAndTime);
	AddMode(EDiurnalScheduleSortMode::TimeOfDay);
	return Menu.MakeWidget();
}

TSharedRef<SWidget> SDiurnalScheduleWorkspace::BuildFilterMenu()
{
	FMenuBuilder Menu(true, nullptr);
	const auto AddFilter = [this, &Menu](const FText& Label, const EFilterOption Option, const FName IconName)
	{
		Menu.AddMenuEntry(
			Label,
			FText::GetEmpty(),
			FSlateIcon(FDiurnalCycleEditorStyle::GetStyleSetName(), IconName),
			FUIAction(
				FExecuteAction::CreateSP(this, &SDiurnalScheduleWorkspace::ToggleFilter, Option),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SDiurnalScheduleWorkspace::IsFilterEnabled, Option)),
			NAME_None,
			EUserInterfaceActionType::ToggleButton);
	};
	Menu.BeginSection("Types", LOCTEXT("Types", "Type"));
	AddFilter(LOCTEXT("RepeatingEvent", "Repeating Event"), EFilterOption::RepeatingEvent, FDiurnalCycleEditorStyle::GetOccurrenceIconName(false));
	AddFilter(LOCTEXT("OnceEvent", "One-off Event"), EFilterOption::OnceEvent, FDiurnalCycleEditorStyle::GetOccurrenceIconName(true));
	AddFilter(LOCTEXT("RepeatingRange", "Repeating Time Range"), EFilterOption::RepeatingRange, FDiurnalCycleEditorStyle::GetOccurrenceIconName(false));
	AddFilter(LOCTEXT("OnceRange", "One-off Time Range"), EFilterOption::OnceRange, FDiurnalCycleEditorStyle::GetOccurrenceIconName(true));
	Menu.EndSection();
	Menu.BeginSection("Behavior", LOCTEXT("Behavior", "Behavior"));
	AddFilter(LOCTEXT("Notify", "Notify"), EFilterOption::Notify, FName(TEXT("DiurnalCycle.Entry.Notify")));
	AddFilter(LOCTEXT("Blocking", "Blocking"), EFilterOption::Blocking, FName(TEXT("DiurnalCycle.Entry.Blocking")));
	Menu.EndSection();
	Menu.AddMenuSeparator();
	Menu.AddMenuEntry(
		LOCTEXT("Clear", "Clear Search and Filters"),
		LOCTEXT("ClearTip", "Restore all entry types and clear the search text."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.RecentAssets"),
		FUIAction(FExecuteAction::CreateLambda([this] { ClearFilters(); })));
	return Menu.MakeWidget();
}

void SDiurnalScheduleWorkspace::SetSearchText(const FText& Text)
{
	FDiurnalScheduleEditorFilter Filter = Model->GetFilter();
	Filter.SearchText = Text.ToString();
	Model->SetFilter(Filter);
}

void SDiurnalScheduleWorkspace::SetSortMode(const EDiurnalScheduleSortMode SortMode)
{
	Model->SetSortMode(SortMode);
	if (GConfig)
	{
		GConfig->SetInt(ViewSettingsSection, SortSettingsKey, static_cast<int32>(SortMode), GEditorPerProjectIni);
	}
}

void SDiurnalScheduleWorkspace::ToggleFilter(const EFilterOption Option)
{
	FDiurnalScheduleEditorFilter Filter = Model->GetFilter();
	switch (Option)
	{
	case EFilterOption::RepeatingEvent: Filter.bShowRepeatingEvents = !Filter.bShowRepeatingEvents; break;
	case EFilterOption::OnceEvent: Filter.bShowOnceEvents = !Filter.bShowOnceEvents; break;
	case EFilterOption::RepeatingRange: Filter.bShowRepeatingRanges = !Filter.bShowRepeatingRanges; break;
	case EFilterOption::OnceRange: Filter.bShowOnceRanges = !Filter.bShowOnceRanges; break;
	case EFilterOption::Notify: Filter.bShowNotify = !Filter.bShowNotify; break;
	case EFilterOption::Blocking: Filter.bShowBlocking = !Filter.bShowBlocking; break;
	}
	Model->SetFilter(Filter);
}

bool SDiurnalScheduleWorkspace::IsFilterEnabled(const EFilterOption Option) const
{
	const FDiurnalScheduleEditorFilter& Filter = Model->GetFilter();
	switch (Option)
	{
	case EFilterOption::RepeatingEvent: return Filter.bShowRepeatingEvents;
	case EFilterOption::OnceEvent: return Filter.bShowOnceEvents;
	case EFilterOption::RepeatingRange: return Filter.bShowRepeatingRanges;
	case EFilterOption::OnceRange: return Filter.bShowOnceRanges;
	case EFilterOption::Notify: return Filter.bShowNotify;
	case EFilterOption::Blocking: return Filter.bShowBlocking;
	default: return true;
	}
}

FReply SDiurnalScheduleWorkspace::ClearFilters()
{
	Model->ClearFilter();
	if (SearchBox) SearchBox->SetText(FText::GetEmpty());
	return FReply::Handled();
}

FText SDiurnalScheduleWorkspace::GetSortText() const
{
	return FText::Format(LOCTEXT("SortValue", "Sort: {0}"), DiurnalScheduleEditor::GetSortModeText(Model->GetSortMode()));
}

FText SDiurnalScheduleWorkspace::GetFilterText() const
{
	return Model->GetFilter().IsActive() ? LOCTEXT("Filtered", "Filters Active") : LOCTEXT("Filters", "Filters");
}

ECheckBoxState SDiurnalScheduleWorkspace::IsViewChecked(const EDiurnalScheduleEditorViewMode Mode) const
{
	return ViewMode == Mode ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

EVisibility SDiurnalScheduleWorkspace::GetListControlsVisibility() const
{
	return ViewMode == EDiurnalScheduleEditorViewMode::List ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SDiurnalScheduleWorkspace::GetTimelineControlsVisibility() const
{
	return ViewMode == EDiurnalScheduleEditorViewMode::Timeline ? EVisibility::Visible : EVisibility::Collapsed;
}

FReply SDiurnalScheduleWorkspace::CurrentTimeline()
{
	if (WeekView) WeekView->GoToCurrent();
	return FReply::Handled();
}

void SDiurnalScheduleWorkspace::SetHourHeight(const float Value)
{
	if (WeekView) WeekView->SetPixelsPerHour(Value);
}

float SDiurnalScheduleWorkspace::GetHourHeight() const
{
	return WeekView ? WeekView->GetPixelsPerHour() : 48.0f;
}

FReply SDiurnalScheduleWorkspace::SetVisibleDayPreset(const int32 Days)
{
	if (WeekView) WeekView->SetVisibleDaysPreset(Days);
	return FReply::Handled();
}

FReply SDiurnalScheduleWorkspace::ResetTimelineView()
{
	if (WeekView) WeekView->ResetView();
	return FReply::Handled();
}

FText SDiurnalScheduleWorkspace::GetTimelineRangeText() const
{
	return WeekView ? WeekView->GetVisibleRangeText() : FText::GetEmpty();
}

FText SDiurnalScheduleWorkspace::GetRuntimeMarkerText() const
{
	return WeekView ? WeekView->GetRuntimeMarkerText() : FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
