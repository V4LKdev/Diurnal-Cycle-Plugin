#include "ScheduleEditor/Widgets/SDiurnalScheduleInspector.h"

#include "DiurnalCycleSettings.h"
#include "DiurnalSchedule.h"
#include "ScheduleEditor/DiurnalScheduleEditorPresentation.h"
#include "IDetailTreeNode.h"
#include "IPropertyRowGenerator.h"
#include "PropertyEditorModule.h"
#include "PropertyHandle.h"
#include "SResetToDefaultPropertyEditor.h"
#include "ScheduleEditor/DiurnalScheduleEditorModel.h"
#include "ScopedTransaction.h"
#include "Async/Async.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Misc/DataValidation.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "UObject/UObjectGlobals.h"

#define LOCTEXT_NAMESPACE "SDiurnalScheduleInspector"

namespace
{
	TSharedPtr<IPropertyHandle> FindProperty(const TArray<TSharedRef<IDetailTreeNode>>& Nodes, const FName Name)
	{
		for (const TSharedRef<IDetailTreeNode>& Node : Nodes)
		{
			if (TSharedPtr<IPropertyHandle> Handle = Node->CreatePropertyHandle(); Handle && Handle->GetProperty() && Handle->GetProperty()->GetFName() == Name) return Handle;
			TArray<TSharedRef<IDetailTreeNode>> Children; Node->GetChildren(Children, true);
			if (TSharedPtr<IPropertyHandle> Found = FindProperty(Children, Name)) return Found;
		}
		return nullptr;
	}
}

void SDiurnalScheduleInspector::Construct(const FArguments& Args)
{
	Model = Args._Model; check(Model);
	bShowCreationActions = Args._ShowCreationActions;
	FPropertyRowGeneratorArgs GeneratorArgs;
	RowGenerator = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor")).CreatePropertyRowGenerator(GeneratorArgs);
	Model->OnChanged().AddSP(this, &SDiurnalScheduleInspector::Refresh);
	ObjectPropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(this, &SDiurnalScheduleInspector::HandleObjectPropertyChanged);
	ChildSlot[SNew(SBorder).Padding(12).BorderImage(FAppStyle::GetBrush("Brushes.Panel"))[SAssignNew(Rows, SVerticalBox)]];
	Refresh();
}

SDiurnalScheduleInspector::~SDiurnalScheduleInspector()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(ObjectPropertyChangedHandle);
}

void SDiurnalScheduleInspector::Refresh()
{
	Rows->ClearChildren();
	UDiurnalSchedule* Asset = Model->GetSchedule();
	if (!Asset || Model->GetSelectionType() == EDiurnalScheduleSelectionType::None)
	{
		if (!Asset) return;
		const int32 SelectedDay = Model->GetSelectedDay();
		if (SelectedDay != INDEX_NONE)
		{
			int32 EventCount = 0;
			for (const FDiurnalTimeEvent& Event : Asset->TimeEvents) EventCount += Event.OccursOnDay(SelectedDay) ? 1 : 0;
			int32 RangeCount = 0;
			for (const FDiurnalTimeRange& Range : Asset->TimeRanges)
			{
				RangeCount += Range.OccursOnDay(SelectedDay)
					|| (SelectedDay > 1 && Range.StartTime > Range.EndTime && Range.OccursOnDay(SelectedDay - 1)) ? 1 : 0;
			}
			Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
			[
				SNew(STextBlock).Text(FText::Format(LOCTEXT("DaySummaryTitle", "Day {0}"), FText::AsNumber(SelectedDay))).Font(FAppStyle::GetFontStyle("HeadingSmall"))
			];
			Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 12)
			[
				SNew(STextBlock).Text(FText::Format(LOCTEXT("DaySummary", "{0} event occurrences\n{1} active range entries"), FText::AsNumber(EventCount), FText::AsNumber(RangeCount))).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			];
			if (bShowCreationActions)
			{
				Rows->AddSlot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 5, 0)[SNew(SButton).Text(LOCTEXT("AddDayEvent", "Add Event")).OnClicked_Lambda([Model = Model, SelectedDay] { Model->AddOnceEventAt(SelectedDay, FDiurnalTimeOfDay(12)); return FReply::Handled(); })]
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(LOCTEXT("AddDayRange", "Add Time Range")).OnClicked_Lambda([Model = Model, SelectedDay] { Model->AddRangeAt(FDiurnalTimeOfDay(12), 60, SelectedDay); return FReply::Handled(); })]
				];
			}
			return;
		}
		FDataValidationContext ValidationContext;
		const bool bValid = Asset->IsDataValid(ValidationContext) != EDataValidationResult::Invalid;
		Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
		[
			SNew(STextBlock).Text(FText::FromString(Asset->GetName())).Font(FAppStyle::GetFontStyle("HeadingSmall"))
		];
		Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 3)
		[
			SNew(STextBlock).Text(FText::Format(LOCTEXT("AssetSummary", "{0} events\n{1} ranges"), Asset->TimeEvents.Num(), Asset->TimeRanges.Num())).ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
		Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 12)
		[
			SNew(STextBlock).Text(bValid ? LOCTEXT("Valid", "Valid") : LOCTEXT("Invalid", "Needs attention"))
			.ColorAndOpacity(bValid ? FSlateColor(FStyleColors::AccentGreen) : FSlateColor(FStyleColors::Warning))
		];
		if (bShowCreationActions)
		{
			Rows->AddSlot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 5, 0)[SNew(SButton).Text(LOCTEXT("AddEvent", "Add Event")).OnClicked_Lambda([Model = Model] { Model->AddRepeatingEvent(); return FReply::Handled(); })]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(LOCTEXT("AddRange", "Add Time Range")).OnClicked_Lambda([Model = Model] { Model->AddRange(); return FReply::Handled(); })]
			];
		}
		return;
	}

	RowGenerator->SetObjects({ Asset });
	const FName ArrayName = Model->GetSelectionType() == EDiurnalScheduleSelectionType::Event ? GET_MEMBER_NAME_CHECKED(UDiurnalSchedule, TimeEvents) : GET_MEMBER_NAME_CHECKED(UDiurnalSchedule, TimeRanges);
	TSharedPtr<IPropertyHandle> ArrayHandle = FindProperty(RowGenerator->GetRootTreeNodes(), ArrayName);
	TSharedPtr<IPropertyHandleArray> Array = ArrayHandle ? ArrayHandle->AsArray() : nullptr;
	int32 SelectedIndex = INDEX_NONE;
	if (Model->GetSelectionType() == EDiurnalScheduleSelectionType::Event) SelectedIndex = Asset->TimeEvents.IndexOfByPredicate([&](const FDiurnalTimeEvent& Value){ return Value.EntryId == Model->GetSelectedId(); });
	else SelectedIndex = Asset->TimeRanges.IndexOfByPredicate([&](const FDiurnalTimeRange& Value){ return Value.EntryId == Model->GetSelectedId(); });
	TSharedPtr<IPropertyHandle> Element;
	if (Array && SelectedIndex != INDEX_NONE) Element = Array->GetElement(SelectedIndex);
	if (!Element)
	{
		Rows->AddSlot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("Unavailable", "The selected entry is no longer available."))];
		return;
	}

	Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[SNew(STextBlock).Text(Model->GetSelectionType() == EDiurnalScheduleSelectionType::Event ? LOCTEXT("Event", "Event Inspector") : LOCTEXT("Range", "Range Inspector")).Font(FAppStyle::GetFontStyle("HeadingSmall"))];
	TSharedPtr<IDetailTreeNode> ElementNode = RowGenerator->FindTreeNode(Element);
	if (!ElementNode)
	{
		Rows->AddSlot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("NoCustomizedNode", "Property customization is temporarily unavailable for this entry."))];
		return;
	}

	TArray<TSharedRef<IDetailTreeNode>> Children;
	ElementNode->GetChildren(Children, true);
	TMap<FName, TSharedPtr<IDetailTreeNode>> NodesByProperty;
	for (const TSharedRef<IDetailTreeNode>& ChildNode : Children)
	{
		const TSharedPtr<IPropertyHandle> Child = ChildNode->CreatePropertyHandle();
		if (!Child || !Child->GetProperty()) continue;
		NodesByProperty.Add(Child->GetProperty()->GetFName(), ChildNode);
	}

	const TArray<FName> DisplayOrder = Model->GetSelectionType() == EDiurnalScheduleSelectionType::Event
		? TArray<FName>{ TEXT("EventName"), TEXT("EventTags"), TEXT("Recurrence"), TEXT("TimeOfDay"), TEXT("Behavior"), TEXT("EditorColor") }
		: TArray<FName>{ TEXT("RangeName"), TEXT("RangeTags"), TEXT("Recurrence"), TEXT("StartTime"), TEXT("EndTime"), TEXT("EditorColor") };
	for (const FName PropertyName : DisplayOrder)
	{
		const TSharedPtr<IDetailTreeNode>* NodePtr = NodesByProperty.Find(PropertyName);
		if (!NodePtr || !NodePtr->IsValid()) continue;
		const TSharedRef<IDetailTreeNode> ChildNode = NodePtr->ToSharedRef();
		const TSharedPtr<IPropertyHandle> Child = ChildNode->CreatePropertyHandle();
		if (!Child) continue;
		if (PropertyName == TEXT("Recurrence"))
		{
			AddRecurrenceRows(Child);
			continue;
		}
		if (PropertyName == TEXT("TimeOfDay") || PropertyName == TEXT("StartTime") || PropertyName == TEXT("EndTime"))
		{
			AddTimePropertyNode(ChildNode);
			continue;
		}
		if (PropertyName == TEXT("EditorColor"))
		{
			AddColorRow();
			continue;
		}
		AddPropertyNode(ChildNode);
	}
}

void SDiurnalScheduleInspector::AddColorRow()
{
	Rows->AddSlot().AutoHeight().Padding(0, 2)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(.42f).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(LOCTEXT("Color", "Color")).Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
			.ToolTipText(LOCTEXT("ColorTip", "Editor-only planning color. It has no cooked runtime effect."))
		]
		+ SHorizontalBox::Slot().FillWidth(.58f).VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1)
			[
				SNew(SColorBlock)
				.Color(this, &SDiurnalScheduleInspector::GetSelectedEditorColor)
				.ShowBackgroundForAlpha(true)
				.AlphaDisplayMode(EColorBlockAlphaDisplayMode::Ignore)
				.Size(FVector2D(120, 20))
				.CornerRadius(FVector4(4, 4, 4, 4))
				.OnMouseButtonDown(this, &SDiurnalScheduleInspector::HandleColorBlockMouseDown)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0, 0, 0)
			[
				SNew(SResetToDefaultPropertyEditor, TSharedPtr<IPropertyHandle>())
				.CustomResetToDefault(FResetToDefaultOverride::Create(
					TAttribute<bool>::CreateSP(this, &SDiurnalScheduleInspector::IsSelectedColorOverridden),
					FSimpleDelegate::CreateSP(this, &SDiurnalScheduleInspector::ResetColorToAutomatic)))
			]
		]
	];
}

FReply SDiurnalScheduleInspector::HandleColorBlockMouseDown(
	const FGeometry&,
	const FPointerEvent& PointerEvent)
{
	if (PointerEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	return OpenColorPicker();
}

void SDiurnalScheduleInspector::AddTimePropertyNode(const TSharedRef<IDetailTreeNode>& Node)
{
	const TSharedPtr<IPropertyHandle> TimeHandle = Node->CreatePropertyHandle();
	if (!TimeHandle) return;

	const TSharedPtr<IPropertyHandle> Hour = TimeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDiurnalTimeOfDay, Hour));
	const TSharedPtr<IPropertyHandle> Minute = TimeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDiurnalTimeOfDay, Minute));
	const TSharedPtr<IPropertyHandle> Second = TimeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDiurnalTimeOfDay, Second));
	if (!Hour || !Minute || !Second) return;

	const FNodeWidgets Widgets = Node->CreateNodeWidgets();
	const auto Component = [](const TSharedPtr<IPropertyHandle>& Handle, const FText& Tooltip)
	{
		return SNew(SBox)
			.WidthOverride(42.f)
			.ToolTipText(Tooltip)
			[Handle->CreatePropertyValueWidget(false)];
	};

	Rows->AddSlot().AutoHeight().Padding(0, 2)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(.42f).VAlign(VAlign_Center)
		[
			Widgets.NameWidget.IsValid()
				? Widgets.NameWidget.ToSharedRef()
				: StaticCastSharedRef<SWidget>(SNew(STextBlock).Text(TimeHandle->GetPropertyDisplayName()))
		]
		+ SHorizontalBox::Slot().FillWidth(.58f).VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[Component(Hour, LOCTEXT("HourTooltip", "Hour (00–23)"))]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0)[SNew(STextBlock).Text(FText::FromString(TEXT(":")))]
			+ SHorizontalBox::Slot().AutoWidth()[Component(Minute, LOCTEXT("MinuteTooltip", "Minute (00–59)"))]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0)[SNew(STextBlock).Text(FText::FromString(TEXT(":")))]
			+ SHorizontalBox::Slot().AutoWidth()[Component(Second, LOCTEXT("SecondTooltip", "Second (00–59)"))]
		]
	];
}

void SDiurnalScheduleInspector::AddPropertyNode(const TSharedRef<IDetailTreeNode>& Node)
{
	const FNodeWidgets Widgets = Node->CreateNodeWidgets();
	if (Widgets.WholeRowWidget.IsValid())
	{
		Rows->AddSlot().AutoHeight().Padding(0, 2)[Widgets.WholeRowWidget.ToSharedRef()];
		return;
	}
	if (!Widgets.NameWidget.IsValid() && !Widgets.ValueWidget.IsValid()) return;
	Rows->AddSlot().AutoHeight().Padding(0, 2)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(.42f).VAlign(VAlign_Center)
		[Widgets.NameWidget.IsValid() ? Widgets.NameWidget.ToSharedRef() : SNullWidget::NullWidget]
		+ SHorizontalBox::Slot().FillWidth(.58f).VAlign(VAlign_Center)
		[Widgets.ValueWidget.IsValid() ? Widgets.ValueWidget.ToSharedRef() : SNullWidget::NullWidget]
	];
}

void SDiurnalScheduleInspector::AddPropertyHandleRow(const FText& Label, const TSharedPtr<IPropertyHandle>& Handle)
{
	if (!Handle) return;
	Rows->AddSlot().AutoHeight().Padding(0, 2)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(.42f).VAlign(VAlign_Center)[SNew(STextBlock).Text(Label).Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))]
		+ SHorizontalBox::Slot().FillWidth(.58f)
		[Handle->CreatePropertyValueWidget()]
	];
}

void SDiurnalScheduleInspector::AddRecurrenceRows(const TSharedPtr<IPropertyHandle>& Handle)
{
	if (!Handle) return;
	const TSharedPtr<IPropertyHandle> Mode = Handle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDiurnalRecurrence, Mode));
	const TSharedPtr<IPropertyHandle> Anchor = Handle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDiurnalRecurrence, AnchorDay));
	const TSharedPtr<IPropertyHandle> Interval = Handle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDiurnalRecurrence, IntervalDays));
	AddPropertyHandleRow(LOCTEXT("Occurs", "Occurs"), Mode);
	uint8 ModeValue = static_cast<uint8>(EDiurnalRecurrenceMode::Repeating);
	if (Mode) Mode->GetValue(ModeValue);
	const bool bRepeating = static_cast<EDiurnalRecurrenceMode>(ModeValue) == EDiurnalRecurrenceMode::Repeating;
	AddPropertyHandleRow(bRepeating ? LOCTEXT("StartingDay", "Starting Day") : LOCTEXT("OccurrenceDay", "Day"), Anchor);
	if (bRepeating) AddPropertyHandleRow(LOCTEXT("EveryDays", "Every (days)"), Interval);
}

FLinearColor SDiurnalScheduleInspector::GetSelectedEditorColor() const
{
	const UDiurnalSchedule* Asset = Model->GetSchedule();
	if (!Asset) return FLinearColor::White;
	if (Model->GetSelectionType() == EDiurnalScheduleSelectionType::Event)
	{
		if (const FDiurnalTimeEvent* Event = Asset->TimeEvents.FindByPredicate([this](const FDiurnalTimeEvent& Value) { return Value.EntryId == Model->GetSelectedId(); }))
		{
			return DiurnalScheduleEditor::GetEditorColor(*Event);
		}
	}
	else if (Model->GetSelectionType() == EDiurnalScheduleSelectionType::Range)
	{
		if (const FDiurnalTimeRange* Range = Asset->TimeRanges.FindByPredicate([this](const FDiurnalTimeRange& Value) { return Value.EntryId == Model->GetSelectedId(); }))
		{
			return DiurnalScheduleEditor::GetEditorColor(*Range);
		}
	}
	return FLinearColor::White;
}

bool SDiurnalScheduleInspector::IsSelectedColorOverridden() const
{
#if WITH_EDITORONLY_DATA
	const UDiurnalSchedule* Asset = Model->GetSchedule();
	if (!Asset) return false;
	if (Model->GetSelectionType() == EDiurnalScheduleSelectionType::Event)
	{
		if (const FDiurnalTimeEvent* Event = Asset->TimeEvents.FindByPredicate([this](const FDiurnalTimeEvent& Value) { return Value.EntryId == Model->GetSelectedId(); })) return Event->bOverrideEditorColor;
	}
	else if (Model->GetSelectionType() == EDiurnalScheduleSelectionType::Range)
	{
		if (const FDiurnalTimeRange* Range = Asset->TimeRanges.FindByPredicate([this](const FDiurnalTimeRange& Value) { return Value.EntryId == Model->GetSelectedId(); })) return Range->bOverrideEditorColor;
	}
#endif
	return false;
}

FReply SDiurnalScheduleInspector::OpenColorPicker()
{
#if WITH_EDITORONLY_DATA
	UDiurnalSchedule* Asset = Model->GetSchedule();
	if (!Asset || ColorTransaction) return FReply::Handled();
	bOriginalColorOverride = IsSelectedColorOverridden();
	bPackageWasDirty = Asset->GetOutermost()->IsDirty();
	bColorPickerCancelled = false;
	if (Model->GetSelectionType() == EDiurnalScheduleSelectionType::Event)
	{
		if (const FDiurnalTimeEvent* Event = Asset->TimeEvents.FindByPredicate([this](const FDiurnalTimeEvent& Value) { return Value.EntryId == Model->GetSelectedId(); })) OriginalStoredColor = Event->EditorColor;
	}
	else if (Model->GetSelectionType() == EDiurnalScheduleSelectionType::Range)
	{
		if (const FDiurnalTimeRange* Range = Asset->TimeRanges.FindByPredicate([this](const FDiurnalTimeRange& Value) { return Value.EntryId == Model->GetSelectedId(); })) OriginalStoredColor = Range->EditorColor;
	}
	ColorTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("SetEntryColor", "Set Schedule Entry Color"));
	Asset->Modify();

	FColorPickerArgs Args;
	Args.InitialColor = GetSelectedEditorColor();
	Args.bUseAlpha = false;
	Args.bOnlyRefreshOnMouseUp = false;
	Args.bOnlyRefreshOnOk = false;
	Args.ParentWidget = AsShared();
	Args.OnColorCommitted = FOnLinearColorValueChanged::CreateSP(this, &SDiurnalScheduleInspector::SetSelectedEditorColor);
	Args.OnColorPickerCancelled = FOnColorPickerCancelled::CreateSP(this, &SDiurnalScheduleInspector::HandleColorPickerCancelled);
	Args.OnInteractivePickBegin = FSimpleDelegate::CreateSP(this, &SDiurnalScheduleInspector::HandleColorPickerInteractiveBegin);
	Args.OnInteractivePickEnd = FSimpleDelegate::CreateSP(this, &SDiurnalScheduleInspector::HandleColorPickerInteractiveEnd);
	Args.OnColorPickerWindowClosed = FOnWindowClosed::CreateSP(this, &SDiurnalScheduleInspector::HandleColorPickerClosed);
	if (!::OpenColorPicker(Args)) ColorTransaction.Reset();
#endif
	return FReply::Handled();
}

void SDiurnalScheduleInspector::SetSelectedEditorColor(const FLinearColor NewColor)
{
#if WITH_EDITORONLY_DATA
	UDiurnalSchedule* Asset = Model->GetSchedule();
	if (!Asset) return;
	if (Model->GetSelectionType() == EDiurnalScheduleSelectionType::Event)
	{
		if (FDiurnalTimeEvent* Event = Asset->TimeEvents.FindByPredicate([this](const FDiurnalTimeEvent& Value) { return Value.EntryId == Model->GetSelectedId(); }))
		{
			Event->bOverrideEditorColor = true;
			Event->EditorColor = NewColor;
		}
	}
	else if (Model->GetSelectionType() == EDiurnalScheduleSelectionType::Range)
	{
		if (FDiurnalTimeRange* Range = Asset->TimeRanges.FindByPredicate([this](const FDiurnalTimeRange& Value) { return Value.EntryId == Model->GetSelectedId(); }))
		{
			Range->bOverrideEditorColor = true;
			Range->EditorColor = NewColor;
		}
	}
	Asset->MarkPackageDirty();
	Model->NotifyInteractiveValueChanged();
#endif
}

void SDiurnalScheduleInspector::HandleColorPickerCancelled(FLinearColor)
{
#if WITH_EDITORONLY_DATA
	bColorPickerCancelled = true;
	UDiurnalSchedule* Asset = Model->GetSchedule();
	if (!Asset) return;
	if (Model->GetSelectionType() == EDiurnalScheduleSelectionType::Event)
	{
		if (FDiurnalTimeEvent* Event = Asset->TimeEvents.FindByPredicate([this](const FDiurnalTimeEvent& Value) { return Value.EntryId == Model->GetSelectedId(); }))
		{
			Event->bOverrideEditorColor = bOriginalColorOverride;
			Event->EditorColor = OriginalStoredColor;
		}
	}
	else if (Model->GetSelectionType() == EDiurnalScheduleSelectionType::Range)
	{
		if (FDiurnalTimeRange* Range = Asset->TimeRanges.FindByPredicate([this](const FDiurnalTimeRange& Value) { return Value.EntryId == Model->GetSelectedId(); }))
		{
			Range->bOverrideEditorColor = bOriginalColorOverride;
			Range->EditorColor = OriginalStoredColor;
		}
	}
	if (ColorTransaction) ColorTransaction->Cancel();
	if (!bPackageWasDirty) Asset->GetOutermost()->SetDirtyFlag(false);
	Model->NotifyInteractiveValueChanged();
#endif
}

void SDiurnalScheduleInspector::HandleColorPickerInteractiveBegin() {}
void SDiurnalScheduleInspector::HandleColorPickerInteractiveEnd() {}

void SDiurnalScheduleInspector::HandleColorPickerClosed(const TSharedRef<SWindow>&)
{
	UDiurnalSchedule* Asset = Model->GetSchedule();
	if (Asset && !bColorPickerCancelled) Asset->PostEditChange();
	ColorTransaction.Reset();
	bColorPickerCancelled = false;
	Refresh();
}

void SDiurnalScheduleInspector::ResetColorToAutomatic()
{
#if WITH_EDITORONLY_DATA
	UDiurnalSchedule* Asset = Model->GetSchedule();
	if (!Asset || !IsSelectedColorOverridden()) return;
	const FScopedTransaction Transaction(LOCTEXT("ResetEntryColor", "Reset Schedule Entry Color"));
	Asset->Modify();
	if (Model->GetSelectionType() == EDiurnalScheduleSelectionType::Event)
	{
		if (FDiurnalTimeEvent* Event = Asset->TimeEvents.FindByPredicate([this](const FDiurnalTimeEvent& Value) { return Value.EntryId == Model->GetSelectedId(); })) Event->bOverrideEditorColor = false;
	}
	else if (Model->GetSelectionType() == EDiurnalScheduleSelectionType::Range)
	{
		if (FDiurnalTimeRange* Range = Asset->TimeRanges.FindByPredicate([this](const FDiurnalTimeRange& Value) { return Value.EntryId == Model->GetSelectedId(); })) Range->bOverrideEditorColor = false;
	}
	Asset->MarkPackageDirty();
	Asset->PostEditChange();
	Model->NotifyInteractiveValueChanged();
#endif
}

void SDiurnalScheduleInspector::HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event)
{
	if (Object == GetMutableDefault<UDiurnalCycleSettings>())
	{
		Refresh();
		return;
	}
	if (Object != Model->GetSchedule() || bRefreshQueued) return;
	if ((Event.ChangeType & EPropertyChangeType::Interactive) != 0)
	{
		Model->NotifyInteractiveValueChanged();
		return;
	}
	bRefreshQueued = true;
	const TWeakPtr<SDiurnalScheduleInspector> WeakThis = SharedThis(this);
	AsyncTask(ENamedThreads::GameThread, [WeakThis]
	{
		if (const TSharedPtr<SDiurnalScheduleInspector> Inspector = WeakThis.Pin())
		{
			Inspector->bRefreshQueued = false;
			Inspector->Model->HandleUndoRedo();
		}
	});
}

#undef LOCTEXT_NAMESPACE
