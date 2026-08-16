#include "ScheduleEditor/Widgets/SDiurnalScheduleOverview.h"

#include "DiurnalSchedule.h"
#include "DiurnalCycleEditorStyle.h"
#include "ScheduleEditor/DiurnalScheduleEditorCommands.h"
#include "ScheduleEditor/DiurnalScheduleEditorModel.h"
#include "ScheduleEditor/DiurnalScheduleEditorPresentation.h"
#include "ScheduleEditor/DiurnalScheduleEditorViewState.h"

#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"

struct FDiurnalScheduleOverviewItem : public FDiurnalScheduleListItem
{
	TWeakPtr<SInlineEditableTextBlock> InlineName;
};

#define LOCTEXT_NAMESPACE "SDiurnalScheduleOverview"

namespace
{
	FString TagsToString(const FGameplayTagContainer& Tags)
	{
		return Tags.ToStringSimple();
	}

	TSharedRef<SWidget> BuildTagChips(const FGameplayTagContainer& Tags)
	{
		const FDiurnalTagChipProjection Projection = DiurnalScheduleEditor::ProjectTagChips(Tags);
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		for (const FGameplayTag& Tag : Projection.VisibleTags)
		{
			Row->AddSlot().AutoWidth().Padding(0, 0, 4, 0)
			[
				SNew(SBorder)
				.Padding(FMargin(5, 1))
				.BorderImage(FAppStyle::GetBrush("Brushes.Header"))
				.BorderBackgroundColor(FStyleColors::Panel)
				.ToolTipText(FText::FromString(Tag.ToString()))
				[
					SNew(STextBlock)
					.Text(FText::FromName(Tag.GetTagLeafName()))
					.Font(FAppStyle::GetFontStyle("SmallFont"))
				]
			];
		}
		if (Projection.OverflowCount > 0)
		{
			Row->AddSlot().AutoWidth()
			[
				SNew(SBorder)
				.Padding(FMargin(5, 1))
				.BorderImage(FAppStyle::GetBrush("Brushes.Header"))
				.BorderBackgroundColor(FStyleColors::Panel)
				.ToolTipText(Projection.OverflowTooltip)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("+%d"), Projection.OverflowCount)))
					.Font(FAppStyle::GetFontStyle("SmallFont"))
				]
			];
		}
		return Row;
	}

	TSharedRef<SWidget> Icon(const FName Brush, const FText& Tooltip, const EVisibility Visibility = EVisibility::Visible)
	{
		return SNew(SBox)
		.WidthOverride(16)
		.HeightOverride(16)
		.ToolTipText(Tooltip)
		.Visibility(Visibility)
		[
			SNew(SImage).Image(FAppStyle::GetBrush(Brush)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
	}
}

void SDiurnalScheduleOverview::Construct(const FArguments& Args)
{
	Model = Args._Model;
	Commands = Args._Commands;
	check(Model.IsValid());
	check(Commands.IsValid());
	Model->OnChanged().AddSP(this, &SDiurnalScheduleOverview::Refresh);
	Model->OnVisualChanged().AddSP(this, &SDiurnalScheduleOverview::Refresh);

	ChildSlot
	[
		SNew(SBorder)
		.Padding(12)
		.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(2, 0, 0, 4)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("EntriesTitle", "Schedule Entries"))
				.Font(FAppStyle::GetFontStyle("HeadingSmall"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2, 0, 0, 10)
			[
				SNew(STextBlock)
				.Text(this, &SDiurnalScheduleOverview::GetSummary)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SVerticalBox::Slot().FillHeight(1)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SAssignNew(ListView, SListView<TSharedPtr<FDiurnalScheduleOverviewItem>>)
					.ListItemsSource(&Items)
					.SelectionMode(ESelectionMode::Single)
					.OnGenerateRow(this, &SDiurnalScheduleOverview::GenerateRow)
					.OnSelectionChanged(this, &SDiurnalScheduleOverview::SelectionChanged)
					.OnItemScrolledIntoView(this, &SDiurnalScheduleOverview::ItemScrolledIntoView)
					.OnContextMenuOpening(this, &SDiurnalScheduleOverview::BuildContextMenu)
					.OnKeyDownHandler(this, &SDiurnalScheduleOverview::HandleKeyDown)
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(24)
				[
					SNew(SBorder)
					.Padding(18)
					.BorderImage(FAppStyle::GetBrush("Brushes.Recessed"))
					.Visibility(this, &SDiurnalScheduleOverview::GetEmptyVisibility)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(STextBlock).Text(this, &SDiurnalScheduleOverview::GetEmptyTitle).Font(FAppStyle::GetFontStyle("NormalFontBold"))
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 5, 0, 10)
						[
							SNew(STextBlock).Text(this, &SDiurnalScheduleOverview::GetEmptyDescription).ColorAndOpacity(FSlateColor::UseSubduedForeground()).AutoWrapText(true).Justification(ETextJustify::Center)
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(SHorizontalBox)
							.Visibility(this, &SDiurnalScheduleOverview::GetNoEntriesActionsVisibility)
							+ SHorizontalBox::Slot().AutoWidth().Padding(2)[SNew(SButton).Text(LOCTEXT("EmptyAddEvent", "Add Event")).OnClicked(this, &SDiurnalScheduleOverview::AddEventFromEmpty)]
							+ SHorizontalBox::Slot().AutoWidth().Padding(2)[SNew(SButton).Text(LOCTEXT("EmptyAddRange", "Add Time Range")).OnClicked(this, &SDiurnalScheduleOverview::AddRangeFromEmpty)]
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(SButton)
							.Text(LOCTEXT("ClearFilters", "Clear Filters"))
							.Visibility(this, &SDiurnalScheduleOverview::GetFilteredActionsVisibility)
							.OnClicked(this, &SDiurnalScheduleOverview::ClearFilters)
						]
					]
				]
			]
		]
	];
	Refresh();
}

void SDiurnalScheduleOverview::Refresh()
{
	Items.Reset();
	TSharedPtr<FDiurnalScheduleOverviewItem> SelectedItem;
	if (const UDiurnalSchedule* Asset = Model->GetSchedule())
	{
		for (const FDiurnalScheduleListItem& Projected : DiurnalScheduleEditor::BuildListItems(*Asset, Model->GetFilter(), Model->GetSortMode()))
		{
			TSharedRef<FDiurnalScheduleOverviewItem> Item = MakeShared<FDiurnalScheduleOverviewItem>();
			static_cast<FDiurnalScheduleListItem&>(*Item) = Projected;
			Items.Add(Item);
			if (Model->GetSelectionType() == Item->Type && Model->GetSelectedId() == Item->EntryId) SelectedItem = Item;
		}
	}
	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
		if (SelectedItem)
		{
			ListView->SetSelection(SelectedItem, ESelectInfo::Direct);
			if (PendingRenameId == SelectedItem->EntryId && PendingRenameType == SelectedItem->Type) ListView->RequestScrollIntoView(SelectedItem);
		}
	}
}

TSharedRef<ITableRow> SDiurnalScheduleOverview::GenerateRow(
	TSharedPtr<FDiurnalScheduleOverviewItem> Item,
	const TSharedRef<STableViewBase>& Owner)
{
	FText Primary = LOCTEXT("Missing", "Missing Entry");
	FText Secondary;
	FText Tooltip;
	bool bValidEntry = false;
	FLinearColor EntryColor = FLinearColor::Gray;
	TSharedRef<SWidget> TagsWidget = SNullWidget::NullWidget;
	if (const UDiurnalSchedule* Asset = Model->GetSchedule())
	{
		if (Item->Type == EDiurnalScheduleSelectionType::Event)
		{
			if (const FDiurnalTimeEvent* Event = Asset->TimeEvents.FindByPredicate([&](const FDiurnalTimeEvent& Value) { return Value.EntryId == Item->EntryId; }))
			{
				Primary = FText::FromName(Event->GetDisplayName());
				const TCHAR* Behavior = Event->IsBlocking() ? TEXT("Blocking") : TEXT("Notify");
				const FDiurnalRecurrence Recurrence = Event->Recurrence;
				Secondary = Recurrence.Mode == EDiurnalRecurrenceMode::Once
					? FText::FromString(FString::Printf(TEXT("Once · Day %d · %s · %s"), Recurrence.AnchorDay, *Event->TimeOfDay.ToString(), Behavior))
					: FText::FromString(FString::Printf(TEXT("Every %d day%s from Day %d · %s · %s"), Recurrence.IntervalDays, Recurrence.IntervalDays == 1 ? TEXT("") : TEXT("s"), Recurrence.AnchorDay, *Event->TimeOfDay.ToString(), Behavior));
				bValidEntry = Event->IsValid();
				TagsWidget = BuildTagChips(Event->EventTags);
				EntryColor = DiurnalScheduleEditor::GetEditorColor(*Event);
				Tooltip = FText::FromString(FString::Printf(TEXT("%s\nTags: %s"), *Event->GetDisplayName().ToString(), *TagsToString(Event->EventTags)));
			}
		}
		else if (const FDiurnalTimeRange* Range = Asset->TimeRanges.FindByPredicate([&](const FDiurnalTimeRange& Value) { return Value.EntryId == Item->EntryId; }))
		{
			Primary = FText::FromName(Range->GetDisplayName());
			const FDiurnalRecurrence Recurrence = Range->Recurrence;
			Secondary = Recurrence.Mode == EDiurnalRecurrenceMode::Once
				? FText::FromString(FString::Printf(TEXT("Once · Day %d · %s–%s"), Recurrence.AnchorDay, *Range->StartTime.ToString(), *Range->EndTime.ToString()))
				: FText::FromString(FString::Printf(TEXT("Every %d day%s from Day %d · %s–%s"), Recurrence.IntervalDays, Recurrence.IntervalDays == 1 ? TEXT("") : TEXT("s"), Recurrence.AnchorDay, *Range->StartTime.ToString(), *Range->EndTime.ToString()));
			bValidEntry = Range->IsValid();
			TagsWidget = BuildTagChips(Range->RangeTags);
			EntryColor = DiurnalScheduleEditor::GetEditorColor(*Range);
			Tooltip = FText::FromString(FString::Printf(TEXT("%s\nTags: %s"), *Range->GetDisplayName().ToString(), *TagsToString(Range->RangeTags)));
		}
	}

	TSharedPtr<SInlineEditableTextBlock> InlineName;
	const TWeakPtr<FDiurnalScheduleOverviewItem> WeakItem = Item;
	TSharedRef<STableRow<TSharedPtr<FDiurnalScheduleOverviewItem>>> Row =
		SNew(STableRow<TSharedPtr<FDiurnalScheduleOverviewItem>>, Owner)
		.ToolTipText(Tooltip)
		.Padding(FMargin(0, 1))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(3)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(EntryColor)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1)
			[
				SNew(SBorder)
				.Padding(FMargin(8, 6))
				.BorderImage(FAppStyle::GetBrush("Brushes.Recessed"))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
					[
						SNew(SImage)
						.Image(FAppStyle::GetBrush("Icons.Warning.Solid"))
						.ColorAndOpacity(FStyleColors::Warning)
						.Visibility(bValidEntry ? EVisibility::Collapsed : EVisibility::Visible)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 3, 0)
							[SNew(SBox).WidthOverride(16).HeightOverride(16).ToolTipText(Item->Type == EDiurnalScheduleSelectionType::Event ? (Item->bOneOff ? LOCTEXT("OnceIcon", "One-off Event") : LOCTEXT("RepeatIcon", "Repeating Event")) : LOCTEXT("RangeIcon", "Time Range"))[SNew(SImage).Image(FDiurnalCycleEditorStyle::Get().GetBrush(Item->Type == EDiurnalScheduleSelectionType::Range ? "DiurnalCycle.Entry.Range" : Item->bOneOff ? "DiurnalCycle.Entry.Once" : "DiurnalCycle.Entry.Repeating")).ColorAndOpacity(FSlateColor::UseSubduedForeground())]]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 3, 0)
						[Icon(Item->bBlocking ? FName("Icons.Pinned") : FName("Icons.Event"), Item->bBlocking ? LOCTEXT("BlockIcon", "Blocking behavior") : LOCTEXT("NotifyIcon", "Notify behavior"), Item->Type == EDiurnalScheduleSelectionType::Event ? EVisibility::Visible : EVisibility::Collapsed)]
					]
					+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SAssignNew(InlineName, SInlineEditableTextBlock)
							.Text(Primary)
							.Font(FAppStyle::GetFontStyle("NormalFontBold"))
							.IsSelected_Lambda([this, WeakItem]
							{
								const TSharedPtr<FDiurnalScheduleOverviewItem> Pinned = WeakItem.Pin();
								return Pinned && ListView.IsValid() && ListView->IsItemSelected(Pinned);
							})
							.OnVerifyTextChanged(this, &SDiurnalScheduleOverview::VerifyName)
							.OnTextCommitted(this, &SDiurnalScheduleOverview::CommitName, Item)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
						[
							SNew(STextBlock).Text(Secondary).ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
						[
							TagsWidget
						]
					]
				]
			]
		];
	Item->InlineName = InlineName;
	return Row;
}

void SDiurnalScheduleOverview::SelectionChanged(TSharedPtr<FDiurnalScheduleOverviewItem> Item, const ESelectInfo::Type SelectInfo)
{
	if (SelectInfo == ESelectInfo::Direct) return;
	if (Item) Model->SelectEntry(Item->Type, Item->EntryId);
	else Model->ClearAllSelection();
}

void SDiurnalScheduleOverview::ItemScrolledIntoView(TSharedPtr<FDiurnalScheduleOverviewItem> Item, const TSharedPtr<ITableRow>&)
{
	if (!Item || Item->Type != PendingRenameType || Item->EntryId != PendingRenameId) return;
	PendingRenameType = EDiurnalScheduleSelectionType::None;
	PendingRenameId.Invalidate();
	if (const TSharedPtr<SInlineEditableTextBlock> Inline = Item->InlineName.Pin()) Inline->EnterEditingMode();
}

void SDiurnalScheduleOverview::RequestRenameSelected()
{
	for (const TSharedPtr<FDiurnalScheduleOverviewItem>& Item : Items)
	{
		if (Item->Type != Model->GetSelectionType() || Item->EntryId != Model->GetSelectedId()) continue;
		PendingRenameType = Item->Type;
		PendingRenameId = Item->EntryId;
		if (ListView.IsValid())
		{
			ListView->SetSelection(Item, ESelectInfo::Direct);
			ListView->RequestScrollIntoView(Item);
		}
		if (const TSharedPtr<SInlineEditableTextBlock> Inline = Item->InlineName.Pin())
		{
			PendingRenameType = EDiurnalScheduleSelectionType::None;
			PendingRenameId.Invalidate();
			Inline->EnterEditingMode();
		}
		return;
	}
}

FReply SDiurnalScheduleOverview::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		Model->ClearAllSelection();
		if (ListView) ListView->ClearSelection();
		return FReply::Handled().SetUserFocus(AsShared(), EFocusCause::Mouse);
	}
	return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
}

TSharedPtr<SWidget> SDiurnalScheduleOverview::BuildContextMenu() const
{
	if (Model->GetSelectionType() == EDiurnalScheduleSelectionType::None) return nullptr;
	FMenuBuilder Menu(true, Commands);
	const FDiurnalScheduleEditorCommands& EditorCommands = FDiurnalScheduleEditorCommands::Get();
	Menu.AddMenuEntry(EditorCommands.Rename);
	Menu.AddMenuEntry(EditorCommands.Duplicate);
	Menu.AddMenuEntry(EditorCommands.Delete);
	Menu.AddMenuSeparator();
	Menu.AddMenuEntry(EditorCommands.MoveUp);
	Menu.AddMenuEntry(EditorCommands.MoveDown);
	return Menu.MakeWidget();
}

FReply SDiurnalScheduleOverview::HandleKeyDown(const FGeometry&, const FKeyEvent& KeyEvent) const
{
	return Commands->ProcessCommandBindings(KeyEvent) ? FReply::Handled() : FReply::Unhandled();
}

bool SDiurnalScheduleOverview::VerifyName(const FText& Text, FText& OutError) const
{
	const FString Candidate = Text.ToString().TrimStartAndEnd();
	if (Candidate.IsEmpty() || FName(*Candidate).IsNone())
	{
		OutError = LOCTEXT("NameRequired", "Enter a name other than None.");
		return false;
	}
	return true;
}

void SDiurnalScheduleOverview::CommitName(const FText& Text, const ETextCommit::Type CommitType, TSharedPtr<FDiurnalScheduleOverviewItem> Item)
{
	if (CommitType == ETextCommit::OnCleared || !Item) return;
	const FString Candidate = Text.ToString().TrimStartAndEnd();
	if (Candidate.IsEmpty() || FName(*Candidate).IsNone()) return;
	Model->SelectEntry(Item->Type, Item->EntryId);
	Model->RenameSelected(FName(*Candidate));
}

FText SDiurnalScheduleOverview::GetSummary() const
{
	const UDiurnalSchedule* Asset = Model->GetSchedule();
	return Asset
		? FText::Format(LOCTEXT("Summary", "{0} visible  ·  {1} events  ·  {2} time ranges  ·  List view"), Items.Num(), Asset->TimeEvents.Num(), Asset->TimeRanges.Num())
		: FText::GetEmpty();
}

FText SDiurnalScheduleOverview::GetEmptyTitle() const
{
	const UDiurnalSchedule* Asset = Model->GetSchedule();
	return Asset && Asset->TimeEvents.IsEmpty() && Asset->TimeRanges.IsEmpty()
		? LOCTEXT("NoEntriesTitle", "This schedule has no entries.")
		: LOCTEXT("NoMatchesTitle", "No schedule entries match the current search and filters.");
}

FText SDiurnalScheduleOverview::GetEmptyDescription() const
{
	const UDiurnalSchedule* Asset = Model->GetSchedule();
	return Asset && Asset->TimeEvents.IsEmpty() && Asset->TimeRanges.IsEmpty()
		? LOCTEXT("NoEntriesDescription", "Add an instantaneous Event or a duration-based Time Range to begin authoring.")
		: LOCTEXT("NoMatchesDescription", "Clear the search and filters to show the complete authored schedule.");
}

EVisibility SDiurnalScheduleOverview::GetEmptyVisibility() const
{
	return Items.IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SDiurnalScheduleOverview::GetNoEntriesActionsVisibility() const
{
	const UDiurnalSchedule* Asset = Model->GetSchedule();
	return Asset && Asset->TimeEvents.IsEmpty() && Asset->TimeRanges.IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SDiurnalScheduleOverview::GetFilteredActionsVisibility() const
{
	const UDiurnalSchedule* Asset = Model->GetSchedule();
	return Asset && (!Asset->TimeEvents.IsEmpty() || !Asset->TimeRanges.IsEmpty()) && Items.IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed;
}

FReply SDiurnalScheduleOverview::AddEventFromEmpty()
{
	if (Model->AddRepeatingEvent().IsValid()) RequestRenameSelected();
	return FReply::Handled();
}

FReply SDiurnalScheduleOverview::AddRangeFromEmpty()
{
	if (Model->AddRange().IsValid()) RequestRenameSelected();
	return FReply::Handled();
}

FReply SDiurnalScheduleOverview::ClearFilters()
{
	Model->ClearFilter();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
