#include "ScheduleEditor/Widgets/SDiurnalScheduleWeekView.h"

#include "DiurnalCycleSettings.h"
#include "DiurnalSchedule.h"
#include "ScheduleEditor/DiurnalScheduleEditorModel.h"
#include "ScheduleEditor/DiurnalTimelineRangeController.h"
#include "Subsystem/DiurnalCycleSubsystem.h"

#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SDiurnalScheduleWeekView"

namespace
{
	DECLARE_DELEGATE_TwoParams(FOnWeekTimeAction, int32, FDiurnalTimeOfDay);
	DECLARE_DELEGATE_ThreeParams(FOnWeekEntrySelected, EDiurnalScheduleSelectionType, FGuid, int32);
	DECLARE_DELEGATE_OneParam(FOnTimelineDayAction, int32);

	FLinearColor WithAlpha(FLinearColor Color, const float Alpha)
	{
		Color.A = Alpha;
		return Color;
	}

	FString TagsTooltip(const FGameplayTagContainer& Tags)
	{
		return Tags.IsEmpty() ? TEXT("No semantic tags") : Tags.ToStringSimple();
	}

	class SDiurnalWeekGrid final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SDiurnalWeekGrid) {}
			SLATE_ATTRIBUTE(FDiurnalWeekViewGeometry, ViewGeometry)
			SLATE_ATTRIBUTE(TOptional<FDiurnalScheduleRuntimeCursor>, RuntimeCursor)
			SLATE_ATTRIBUTE(int32, SelectedDay)
			SLATE_EVENT(FOnTimelineDayAction, OnSelectDay)
			SLATE_EVENT(FOnWeekTimeAction, OnAddEvent)
			SLATE_EVENT(FOnWeekTimeAction, OnAddRange)
		SLATE_END_ARGS()

		void Construct(const FArguments& Args)
		{
			Geometry = Args._ViewGeometry;
			Cursor = Args._RuntimeCursor;
			SelectedDay = Args._SelectedDay;
			OnSelectDay = Args._OnSelectDay;
			OnAddEvent = Args._OnAddEvent;
			OnAddRange = Args._OnAddRange;
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			const FDiurnalWeekViewGeometry Value = Geometry.Get();
			return FVector2D(Value.GetTotalWidth(), Value.GetTimelineHeight());
		}

		virtual bool SupportsKeyboardFocus() const override { return true; }

		virtual int32 OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			const int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			const bool bParentEnabled) const override
		{
			const FDiurnalWeekViewGeometry Value = Geometry.Get();
			const FSlateBrush* WhiteBrush = FAppStyle::GetBrush("WhiteBrush");
			const int32 Selected = SelectedDay.Get();
			const TOptional<FDiurnalScheduleRuntimeCursor> Marker = Cursor.Get();
			for (int32 DayOffset = 0; DayOffset < Value.VisibleDayCount; ++DayOffset)
			{
				const int32 Day = Value.FirstVisibleDay + DayOffset;
				const float X = Value.RulerWidth + DayOffset * Value.DayColumnWidth;
				FLinearColor Background = DayOffset % 2 == 0 ? FStyleColors::Background.GetSpecifiedColor() : FStyleColors::Panel.GetSpecifiedColor();
				Background.A = 0.42f;
				if (Marker.IsSet() && Day == Marker->Day) Background = WithAlpha(FStyleColors::AccentBlue.GetSpecifiedColor(), Marker->bIsLiveRuntime ? 0.16f : 0.08f);
				if (Day == Selected) Background = WithAlpha(FStyleColors::Select.GetSpecifiedColor(), 0.20f);
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(
					FVector2D(Value.DayColumnWidth, Value.GetTimelineHeight()), FSlateLayoutTransform(FVector2D(X, 0))), WhiteBrush, ESlateDrawEffect::None, Background);
			}

			const int32 MajorInterval = Value.PixelsPerHour >= 60.0f ? 1 : Value.PixelsPerHour >= 38.0f ? 2 : Value.PixelsPerHour >= 28.0f ? 3 : 6;
			for (int32 Hour = 0; Hour <= 24; ++Hour)
			{
				const float Y = Hour * Value.PixelsPerHour;
				const bool bEmphasized = Hour == 0 || Hour == 12 || Hour == 24;
				const bool bMajor = bEmphasized || Hour % MajorInterval == 0;
				const TArray<FVector2D> Points = { FVector2D(Value.RulerWidth, Y), FVector2D(Value.GetTotalWidth(), Y) };
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Points, ESlateDrawEffect::None,
					WithAlpha(FStyleColors::Foreground.GetSpecifiedColor(), bEmphasized ? 0.46f : bMajor ? 0.28f : 0.10f), false, bEmphasized ? 1.5f : bMajor ? 1.0f : 0.5f);
				if (Hour < 24 && Value.PixelsPerHour >= 58.0f)
				{
					const float HalfY = Y + Value.PixelsPerHour * 0.5f;
					const TArray<FVector2D> HalfPoints = { FVector2D(Value.RulerWidth, HalfY), FVector2D(Value.GetTotalWidth(), HalfY) };
					FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), HalfPoints, ESlateDrawEffect::None, WithAlpha(FStyleColors::Foreground.GetSpecifiedColor(), 0.08f), false, 0.5f);
				}
			}
			for (int32 DayOffset = 0; DayOffset <= Value.VisibleDayCount; ++DayOffset)
			{
				const float X = Value.RulerWidth + DayOffset * Value.DayColumnWidth;
				const TArray<FVector2D> Points = { FVector2D(X, 0), FVector2D(X, Value.GetTimelineHeight()) };
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, WithAlpha(FStyleColors::Foreground.GetSpecifiedColor(), 0.22f), false, 1.0f);
			}

			if (Marker.IsSet() && Marker->Day >= Value.FirstVisibleDay && Marker->Day < Value.FirstVisibleDay + Value.VisibleDayCount && Marker->TimeOfDay.IsValid())
			{
				const FVector2D Position = Value.DayAndTimeToPosition(Marker->Day, Marker->TimeOfDay);
				const float StartX = Value.RulerWidth + (Marker->Day - Value.FirstVisibleDay) * Value.DayColumnWidth;
				const TArray<FVector2D> Points = { FVector2D(StartX, Position.Y), FVector2D(StartX + Value.DayColumnWidth, Position.Y) };
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 4, AllottedGeometry.ToPaintGeometry(), Points, ESlateDrawEffect::None,
					Marker->bIsLiveRuntime ? FStyleColors::AccentRed.GetSpecifiedColor() : FStyleColors::AccentYellow.GetSpecifiedColor(), false, 2.0f);
			}
			return LayerId + 5;
		}

		virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
			FDiurnalWeekHit Hit;
			if (!Geometry.Get().PositionToDayAndTime(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()), Hit)) return FReply::Unhandled();
			OnAddEvent.ExecuteIfBound(Hit.Day, Hit.TimeOfDay);
			return FReply::Handled().SetUserFocus(AsShared(), EFocusCause::Mouse);
		}

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
			FDiurnalWeekHit Hit;
			if (!Geometry.Get().PositionToDayAndTime(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()), Hit)) return FReply::Unhandled();
			OnSelectDay.ExecuteIfBound(Hit.Day);
			return FReply::Handled();
		}

		virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() != EKeys::RightMouseButton) return FReply::Unhandled();
			FDiurnalWeekHit Hit;
			if (!Geometry.Get().PositionToDayAndTime(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()), Hit)) return FReply::Unhandled();
			FMenuBuilder Menu(true, nullptr);
			Menu.AddMenuEntry(LOCTEXT("AddEventHere", "Add Event Here"), FText::Format(LOCTEXT("AddEventHereTip", "Create a one-off Event on Day {0} at {1}."), Hit.Day, FText::FromString(Hit.TimeOfDay.ToString())), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Event"), FUIAction(FExecuteAction::CreateLambda([Action = OnAddEvent, Hit] { Action.ExecuteIfBound(Hit.Day, Hit.TimeOfDay); })));
			Menu.AddMenuEntry(LOCTEXT("AddRangeHere", "Add Time Range Here"), LOCTEXT("AddRangeHereTip", "Create a one-hour one-off Time Range beginning at this day and time."), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Details"), FUIAction(FExecuteAction::CreateLambda([Action = OnAddRange, Hit] { Action.ExecuteIfBound(Hit.Day, Hit.TimeOfDay); })));
			FSlateApplication::Get().PushMenu(AsShared(), FWidgetPath(), Menu.MakeWidget(), MouseEvent.GetScreenSpacePosition(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
			return FReply::Handled();
		}

	private:
		TAttribute<FDiurnalWeekViewGeometry> Geometry;
		TAttribute<TOptional<FDiurnalScheduleRuntimeCursor>> Cursor;
		TAttribute<int32> SelectedDay;
		FOnWeekTimeAction OnAddEvent;
		FOnWeekTimeAction OnAddRange;
		FOnTimelineDayAction OnSelectDay;
	};

	class SDiurnalWeekEntryBlock final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SDiurnalWeekEntryBlock) {}
			SLATE_ARGUMENT(EDiurnalScheduleSelectionType, Type)
			SLATE_ARGUMENT(FGuid, EntryId)
			SLATE_ARGUMENT(int32, Day)
			SLATE_ARGUMENT(FLinearColor, Color)
			SLATE_ARGUMENT(FText, PrimaryText)
			SLATE_ARGUMENT(FText, SecondaryText)
			SLATE_ARGUMENT(FText, Tooltip)
			SLATE_ARGUMENT(bool, bSelected)
			SLATE_EVENT(FOnWeekEntrySelected, OnSelected)
		SLATE_END_ARGS()

		void Construct(const FArguments& Args)
		{
			Type = Args._Type;
			EntryId = Args._EntryId;
			Day = Args._Day;
			OnSelected = Args._OnSelected;
			FLinearColor Background = Args._Color;
			Background.A = 0.58f;
			const FLinearColor Composite = FMath::Lerp(FStyleColors::Panel.GetSpecifiedColor(), FLinearColor(Background.R, Background.G, Background.B, 1.0f), Background.A);
			const FSlateColor Foreground(Composite.GetLuminance() > 0.48f ? FLinearColor(0.035f, 0.035f, 0.035f, 1.0f) : FLinearColor::White);
			const FLinearColor SelectionBorder = Args._bSelected ? FStyleColors::Select.GetSpecifiedColor() : FLinearColor::Transparent;
			ChildSlot
			[
				SNew(SBorder).Padding(Args._bSelected ? 2.0f : 0.0f).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(SelectionBorder).ToolTipText(Args._Tooltip)
				[
					SNew(SBorder).Padding(FMargin(5, 3)).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Background)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Args._PrimaryText).Font(FAppStyle::GetFontStyle("NormalFontBold")).ColorAndOpacity(Foreground).Clipping(EWidgetClipping::ClipToBounds)]
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Args._SecondaryText).Visibility(Args._SecondaryText.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible).Font(FAppStyle::GetFontStyle("SmallFont")).ColorAndOpacity(Foreground).Clipping(EWidgetClipping::ClipToBounds)]
					]
				]
			];
		}

		virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
			OnSelected.ExecuteIfBound(Type, EntryId, Day);
			return FReply::Handled().SetUserFocus(AsShared(), EFocusCause::Mouse);
		}

		virtual bool SupportsKeyboardFocus() const override { return true; }

	private:
		EDiurnalScheduleSelectionType Type = EDiurnalScheduleSelectionType::None;
		FGuid EntryId;
		int32 Day = 1;
		FOnWeekEntrySelected OnSelected;
	};

	class SDiurnalTimelineDayHeader final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SDiurnalTimelineDayHeader) {}
			SLATE_ARGUMENT(int32, Day)
			SLATE_ARGUMENT(float, Width)
			SLATE_ARGUMENT(FLinearColor, Tint)
			SLATE_EVENT(FOnTimelineDayAction, OnSelected)
			SLATE_EVENT(FOnTimelineDayAction, OnFocused)
		SLATE_END_ARGS()

		void Construct(const FArguments& Args)
		{
			Day = Args._Day;
			OnSelected = Args._OnSelected;
			OnFocused = Args._OnFocused;
			const FText HeaderText = Args._Width >= 72.0f
				? FText::Format(LOCTEXT("DayHeader", "Day {0}"), Day)
				: Args._Width >= 38.0f || Day % 2 == 1
					? FText::AsNumber(Day)
					: FText::GetEmpty();
			ChildSlot
			[
				SNew(SBox).WidthOverride(Args._Width).HeightOverride(38)
				[
					SNew(SBorder).Padding(6).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Args._Tint)
					.ToolTipText(FText::Format(LOCTEXT("DayHeaderTooltip", "Day {0}"), Day))
					[SNew(STextBlock).Text(HeaderText).Font(FAppStyle::GetFontStyle("NormalFontBold")).Justification(ETextJustify::Center)]
				]
			];
		}

		virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent& Event) override
		{
			if (Event.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
			OnSelected.ExecuteIfBound(Day);
			return FReply::Handled().SetUserFocus(AsShared(), EFocusCause::Mouse);
		}

		virtual bool SupportsKeyboardFocus() const override { return true; }

		virtual FReply OnMouseButtonDoubleClick(const FGeometry&, const FPointerEvent& Event) override
		{
			if (Event.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
			OnFocused.ExecuteIfBound(Day);
			return FReply::Handled();
		}

	private:
		int32 Day = 1;
		FOnTimelineDayAction OnSelected;
		FOnTimelineDayAction OnFocused;
	};
}

FVector2D FDiurnalWeekViewGeometry::DayAndTimeToPosition(const int32 Day, const FDiurnalTimeOfDay& TimeOfDay) const
{
	const int32 DayOffset = FMath::Clamp(Day - FirstVisibleDay, 0, FMath::Max(0, VisibleDayCount - 1));
	return FVector2D(RulerWidth + DayOffset * DayColumnWidth, static_cast<float>(TimeOfDay.ToHours() * PixelsPerHour));
}

bool FDiurnalWeekViewGeometry::PositionToDayAndTime(const FVector2D& LocalPosition, FDiurnalWeekHit& OutHit, const int32 SnapMinutes) const
{
	if (VisibleDayCount <= 0 || DayColumnWidth <= 0 || PixelsPerHour <= 0 || LocalPosition.X < RulerWidth || LocalPosition.X >= GetTotalWidth() || LocalPosition.Y < 0 || LocalPosition.Y > GetTimelineHeight()) return false;
	const int32 DayOffset = FMath::Clamp(FMath::FloorToInt((LocalPosition.X - RulerWidth) / DayColumnWidth), 0, VisibleDayCount - 1);
	const int32 RawSecond = FMath::Clamp(FMath::RoundToInt(LocalPosition.Y / PixelsPerHour * 3600.0f), 0, 86399);
	OutHit.Day = FirstVisibleDay + DayOffset;
	OutHit.TimeOfDay = SnapTime(FDiurnalTimeOfDay(RawSecond / 3600, (RawSecond / 60) % 60, RawSecond % 60), SnapMinutes);
	return true;
}

FDiurnalTimeOfDay FDiurnalWeekViewGeometry::SnapTime(const FDiurnalTimeOfDay& TimeOfDay, const int32 SnapMinutes)
{
	if (!TimeOfDay.IsValid() || SnapMinutes <= 0) return TimeOfDay;
	const int32 Interval = SnapMinutes * 60;
	const int32 Snapped = FMath::Clamp(FMath::RoundToInt(static_cast<double>(TimeOfDay.ToSecondsIntoDay()) / Interval) * Interval, 0, 86400 - Interval);
	return FDiurnalTimeOfDay(Snapped / 3600, (Snapped / 60) % 60, Snapped % 60);
}

void SDiurnalScheduleWeekView::Construct(const FArguments& Args)
{
	Model = Args._Model;
	check(Model.IsValid());
	const int32 StartDay = FMath::Max(1, GetDefault<UDiurnalCycleSettings>()->StartingDateTime.Day);
	FirstVisibleDay = FMath::Max(1, StartDay - 3);
	VisibleDayCount = 7;
	ResetWorkingRange(StartDay);
	ViewGeometry.FirstVisibleDay = FirstVisibleDay;
	ViewGeometry.VisibleDayCount = VisibleDayCount;
	TimelineRangeController = MakeShared<FDiurnalTimelineRangeController>(SharedThis(this));
	Model->OnChanged().AddSP(this, &SDiurnalScheduleWeekView::RebuildProjection);
	Model->OnVisualChanged().AddSP(this, &SDiurnalScheduleWeekView::RebuildProjection);
	BeginPIEHandle = FEditorDelegates::BeginPIE.AddSP(this, &SDiurnalScheduleWeekView::HandleBeginPIE);
	PostPIEStartedHandle = FEditorDelegates::PostPIEStarted.AddSP(this, &SDiurnalScheduleWeekView::HandlePostPIEStarted);
	EndPIEHandle = FEditorDelegates::EndPIE.AddSP(this, &SDiurnalScheduleWeekView::HandleEndPIE);
	bPIESessionExpected = GEditor && IsValid(GEditor->PlayWorld);

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBox).WidthOverride_Lambda([this] { return ViewGeometry.GetTotalWidth(); })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(HeaderRow, SHorizontalBox)
				]
				+ SVerticalBox::Slot().FillHeight(1)
				[
					SAssignNew(VerticalScroll, SScrollBox).Orientation(Orient_Vertical)
					.ConsumeMouseWheel(EConsumeMouseWheel::Never)
					.AllowOverscroll(EAllowOverscroll::No)
					+ SScrollBox::Slot()
					[
						SNew(SBox).HeightOverride_Lambda([this] { return ViewGeometry.GetTimelineHeight(); })
						[
							SNew(SOverlay)
							+ SOverlay::Slot()
							[
								SAssignNew(GridWidget, SDiurnalWeekGrid)
								.ViewGeometry_Lambda([this] { return ViewGeometry; })
								.RuntimeCursor_Lambda([this] { return GetDisplayCursor(); })
								.SelectedDay_Lambda([this] { return Model->GetSelectedDay(); })
								.OnSelectDay(FOnTimelineDayAction::CreateSP(this, &SDiurnalScheduleWeekView::SelectDay))
								.OnAddEvent(FOnWeekTimeAction::CreateSP(this, &SDiurnalScheduleWeekView::AddEventAt))
								.OnAddRange(FOnWeekTimeAction::CreateSP(this, &SDiurnalScheduleWeekView::AddRangeAt))
							]
							+ SOverlay::Slot()
							[
								SAssignNew(ItemCanvas, SConstraintCanvas)
								.Visibility(EVisibility::SelfHitTestInvisible)
							]
						]
					]
				]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(80, 48, 16, 0)
		[
			SNew(SBorder).Padding(FMargin(8, 4)).BorderImage(FAppStyle::GetBrush("Brushes.Recessed"))
			.Visibility(this, &SDiurnalScheduleWeekView::GetFilteredSelectionVisibility)
			[SNew(STextBlock).Text(this, &SDiurnalScheduleWeekView::GetFilteredSelectionText).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
		]
	];
	TryBindOrBeginRuntimeDiscovery();
	RebuildProjection();
	FocusDay(ResolveCurrentDay(RuntimeSubsystem.IsValid(), RuntimeCursor, StartDay));
}

SDiurnalScheduleWeekView::~SDiurnalScheduleWeekView()
{
	Model->OnChanged().RemoveAll(this);
	Model->OnVisualChanged().RemoveAll(this);
	FEditorDelegates::BeginPIE.Remove(BeginPIEHandle);
	FEditorDelegates::PostPIEStarted.Remove(PostPIEStartedHandle);
	FEditorDelegates::EndPIE.Remove(EndPIEHandle);
	StopRuntimeDiscovery();
	UnbindRuntime();
}

void SDiurnalScheduleWeekView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	const bool bHasPlayWorld = GEditor && IsValid(GEditor->PlayWorld);
	if ((RuntimeSubsystem.IsValid() && !IsBoundToCurrentPlayWorld())
		|| (!RuntimeSubsystem.IsValid() && bHasPlayWorld && !DiscoveryTimer.IsValid()))
	{
		UnbindRuntime();
		RuntimeCursor.Reset();
		bPIESessionExpected = bHasPlayWorld;
		TryBindOrBeginRuntimeDiscovery();
		RebuildProjection();
	}
	const float Width = AllottedGeometry.GetLocalSize().X;
	if (Width > 200.0f && !FMath::IsNearlyEqual(Width, LastViewportWidth, 1.0f))
	{
		LastViewportWidth = Width;
		UpdateRenderWindow(true);
	}
}

FReply SDiurnalScheduleWeekView::OnMouseWheel(const FGeometry&, const FPointerEvent& MouseEvent)
{
	const float Delta = MouseEvent.GetWheelDelta();
	if (MouseEvent.IsShiftDown())
	{
		PanVisibleRange(FMath::RoundToInt(-Delta * FMath::Max(1.0, VisibleDayCount * 0.12)));
	}
	else if (VerticalScroll)
	{
		VerticalScroll->EndInertialScrolling();
		VerticalScroll->SetScrollOffset(ApplyVerticalWheelDelta(
			VerticalScroll->GetScrollOffset(), Delta, 48.0f, VerticalScroll->GetScrollOffsetOfEnd()));
	}
	// Always consume wheel input so reaching the vertical boundary never leaks
	// into an unrelated parent or changes the horizontal day range.
	return FReply::Handled();
}

FReply SDiurnalScheduleWeekView::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		bMiddlePanning = true;
		MiddlePanOrigin = MouseEvent.GetScreenSpacePosition();
		MiddlePanFirstDay = FirstVisibleDay;
		return FReply::Handled().CaptureMouse(AsShared());
	}
	return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SDiurnalScheduleWeekView::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bMiddlePanning || !HasMouseCapture()) return SCompoundWidget::OnMouseMove(MyGeometry, MouseEvent);
	const float Width = FMath::Max(1.0f, MyGeometry.GetAbsoluteSize().X - ViewGeometry.RulerWidth);
	const double DeltaDays = -(MouseEvent.GetScreenSpacePosition().X - MiddlePanOrigin.X) / Width * VisibleDayCount;
	TimelineRangeController->SetViewRange(
		MiddlePanFirstDay + DeltaDays,
		MiddlePanFirstDay + VisibleDayCount + DeltaDays,
		EViewRangeInterpolation::Immediate);
	return FReply::Handled();
}

FReply SDiurnalScheduleWeekView::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton && bMiddlePanning)
	{
		bMiddlePanning = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return SCompoundWidget::OnMouseButtonUp(MyGeometry, MouseEvent);
}

void SDiurnalScheduleWeekView::GoToCurrent()
{
	const int32 StartDay = GetDefault<UDiurnalCycleSettings>()->StartingDateTime.Day;
	const int32 Day = ResolveCurrentDay(RuntimeSubsystem.IsValid(), RuntimeCursor, StartDay);
	if (Day < WorkingFirstDay || Day >= WorkingFirstDay + WorkingDayCount)
	{
		RepositionWorkingRangeAround(Day, false);
	}
	Model->SelectDay(Day);
	FocusDay(Day);
}

void SDiurnalScheduleWeekView::SetPixelsPerHour(const float Value)
{
	const float Clamped = FMath::Clamp(Value, 24.0f, 120.0f);
	if (FMath::IsNearlyEqual(ViewGeometry.PixelsPerHour, Clamped)) return;
	ViewGeometry.PixelsPerHour = Clamped;
	RebuildCanvas();
	if (GridWidget) GridWidget->Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
}

void SDiurnalScheduleWeekView::SetVisibleDaysPreset(const int32 Days)
{
	const double Center = FirstVisibleDay + VisibleDayCount * 0.5;
	const int32 Span = FMath::Clamp(Days, 1, MaximumVisibleDays);
	TimelineRangeController->SetViewRange(
		Center - Span * 0.5,
		Center + Span * 0.5,
		EViewRangeInterpolation::Immediate);
}

void SDiurnalScheduleWeekView::ResetView()
{
	const int32 StartDay = GetDefault<UDiurnalCycleSettings>()->StartingDateTime.Day;
	const int32 TargetDay = ResolveCurrentDay(RuntimeSubsystem.IsValid(), RuntimeCursor, StartDay);
	SetPixelsPerHour(48.0f);
	ResetWorkingRange(TargetDay, false);
	TimelineRangeController->SetViewRange(
		FMath::Max(1, TargetDay - 3),
		FMath::Max(1, TargetDay - 3) + 7,
		EViewRangeInterpolation::Immediate);
	Model->SelectDay(TargetDay);
	if (VerticalScroll)
	{
		VerticalScroll->EndInertialScrolling();
		VerticalScroll->SetScrollOffset(0.0f);
	}
}

FText SDiurnalScheduleWeekView::GetVisibleRangeText() const
{
	return FText::Format(LOCTEXT("DaysRange", "Days {0}–{1}"), FirstVisibleDay, FirstVisibleDay + VisibleDayCount - 1);
}

TSharedRef<ITimeSliderController> SDiurnalScheduleWeekView::GetTimelineRangeController() const
{
	return TimelineRangeController.ToSharedRef();
}

FText SDiurnalScheduleWeekView::GetRuntimeMarkerText() const
{
	const TOptional<FDiurnalScheduleRuntimeCursor> Cursor = GetDisplayCursor();
	if (!Cursor.IsSet()) return FText::GetEmpty();
	return FText::Format(Cursor->bIsLiveRuntime ? LOCTEXT("LiveMarker", "Live: Day {0} · {1}") : LOCTEXT("StartMarker", "Configured start: Day {0} · {1}"), Cursor->Day, FText::FromString(Cursor->TimeOfDay.ToString()));
}

int32 SDiurnalScheduleWeekView::ResolveCurrentDay(const bool bIsPIE, const TOptional<FDiurnalScheduleRuntimeCursor>& Cursor, const int32 ConfiguredStartDay)
{
	return bIsPIE && Cursor.IsSet() ? FMath::Max(1, Cursor->Day) : FMath::Max(1, ConfiguredStartDay);
}

float SDiurnalScheduleWeekView::ClampVerticalScrollOffset(const float RequestedOffset, const float MaximumOffset)
{
	return FMath::Clamp(RequestedOffset, 0.0f, FMath::Max(0.0f, MaximumOffset));
}

float SDiurnalScheduleWeekView::ApplyVerticalWheelDelta(
	const float CurrentOffset,
	const float WheelDelta,
	const float ScrollStep,
	const float MaximumOffset)
{
	return ClampVerticalScrollOffset(CurrentOffset - WheelDelta * ScrollStep, MaximumOffset);
}

void SDiurnalScheduleWeekView::FocusDay(const int32 InDay, const bool bCenter)
{
	const int32 Day = FMath::Max(1, InDay);
	EnsureWorkingRangeContains(Day);
	const int32 NewFirstDay = bCenter ? Day - (VisibleDayCount - 1) / 2 : Day;
	TimelineRangeController->SetViewRange(
		NewFirstDay,
		NewFirstDay + VisibleDayCount,
		EViewRangeInterpolation::Immediate);
}

void SDiurnalScheduleWeekView::SelectDay(const int32 Day)
{
	Model->SelectDay(Day);
	// The model change callback refreshes the header, grid and inspector.
}

void SDiurnalScheduleWeekView::FocusDayAsSingleDay(const int32 Day)
{
	Model->SelectDay(Day);
	SetVisibleDaysPreset(1);
	FocusDay(Day);
}

void SDiurnalScheduleWeekView::Activate()
{
	const int32 ConfiguredStartDay = GetDefault<UDiurnalCycleSettings>()->StartingDateTime.Day;
	const int32 Day = ResolveCurrentDay(RuntimeSubsystem.IsValid(), RuntimeCursor, ConfiguredStartDay);
	if (Model->GetSelectedDay() != INDEX_NONE) return;
	if (Model->GetSelectionType() == EDiurnalScheduleSelectionType::None)
	{
		Model->SelectDay(Day);
	}
	else
	{
		Model->SelectEntryOccurrence(Model->GetSelectionType(), Model->GetSelectedId(), Day);
	}
}

EVisibility SDiurnalScheduleWeekView::GetItemCanvasVisibility() const
{
	return ItemCanvas ? ItemCanvas->GetVisibility() : EVisibility::Collapsed;
}

void SDiurnalScheduleWeekView::ResetWorkingRange(const int32 TargetDay, const bool bConstrainView)
{
	int32 EarliestDay = FMath::Max(1, TargetDay);
	int32 LatestDay = EarliestDay;
	if (const UDiurnalSchedule* Schedule = Model->GetSchedule())
	{
		for (const FDiurnalTimeEvent& Event : Schedule->TimeEvents)
		{
			EarliestDay = FMath::Min(EarliestDay, FMath::Max(1, Event.Recurrence.AnchorDay));
			LatestDay = FMath::Max(LatestDay, FMath::Max(1, Event.Recurrence.AnchorDay));
		}
		for (const FDiurnalTimeRange& Range : Schedule->TimeRanges)
		{
			EarliestDay = FMath::Min(EarliestDay, FMath::Max(1, Range.Recurrence.AnchorDay));
			LatestDay = FMath::Max(LatestDay, FMath::Max(1, Range.Recurrence.AnchorDay));
		}
	}

	const int32 FirstDay = FMath::Max(1, EarliestDay - 7);
	const int64 RequiredEnd = FMath::Max<int64>(
		static_cast<int64>(FirstDay) + MinimumWorkingDays,
		static_cast<int64>(LatestDay) + 8);
	ApplyWorkingDayRange(FirstDay, static_cast<int32>(FMath::Min<int64>(
		RequiredEnd - FirstDay, MAX_int32 - FirstDay)), bConstrainView);
}

void SDiurnalScheduleWeekView::RepositionWorkingRangeAround(
	const int32 TargetDay,
	const bool bConstrainView)
{
	const int32 SafeTarget = FMath::Max(1, TargetDay);
	const int32 FirstDay = FMath::Max(1, SafeTarget - 7);
	ApplyWorkingDayRange(FirstDay, MinimumWorkingDays, bConstrainView);
}

void SDiurnalScheduleWeekView::EnsureWorkingRangeContains(const int32 Day)
{
	if (Day < WorkingFirstDay || Day >= WorkingFirstDay + WorkingDayCount)
	{
		RepositionWorkingRangeAround(Day, false);
	}
}

void SDiurnalScheduleWeekView::ApplyWorkingDayRange(
	const int32 NewFirstDay,
	const int32 NewWorkingDayCount,
	const bool bConstrainView)
{
	WorkingFirstDay = FMath::Clamp(NewFirstDay, 1, MAX_int32 - 1);
	WorkingDayCount = FMath::Max(1, FMath::Min(NewWorkingDayCount, MAX_int32 - WorkingFirstDay));
	if (!bConstrainView) return;

	const int32 ConstrainedCount = FMath::Min(VisibleDayCount, FMath::Min(MaximumVisibleDays, WorkingDayCount));
	const int32 MaximumFirstDay = WorkingFirstDay + WorkingDayCount - ConstrainedCount;
	ApplyVisibleDayRange(FMath::Clamp(FirstVisibleDay, WorkingFirstDay, MaximumFirstDay), ConstrainedCount);
}

void SDiurnalScheduleWeekView::SetVisibleDayRange(
	const int32 NewFirstDay,
	const int32 NewVisibleDayCount)
{
	const int32 NewCount = FMath::Clamp(NewVisibleDayCount, 1, MaximumVisibleDays);
	const int32 SafeFirstDay = FMath::Max(1, NewFirstDay);
	if (SafeFirstDay < WorkingFirstDay
		|| static_cast<int64>(SafeFirstDay) + NewCount > static_cast<int64>(WorkingFirstDay) + WorkingDayCount)
	{
		RepositionWorkingRangeAround(SafeFirstDay + NewCount / 2, false);
	}
	SetControllerViewRange(SafeFirstDay, static_cast<double>(SafeFirstDay) + NewCount);
}

void SDiurnalScheduleWeekView::SetControllerViewRange(
	const double NewRangeMin,
	const double NewRangeMax)
{
	if (!FMath::IsFinite(NewRangeMin) || !FMath::IsFinite(NewRangeMax)) return;
	const int32 NewCount = FMath::Clamp(
		FMath::RoundToInt(NewRangeMax - NewRangeMin), 1,
		FMath::Min(MaximumVisibleDays, WorkingDayCount));
	const int32 MaximumFirstDay = WorkingFirstDay + WorkingDayCount - NewCount;
	const int32 NewFirstDay = FMath::Clamp(
		FMath::RoundToInt(NewRangeMin), WorkingFirstDay, MaximumFirstDay);
	ApplyVisibleDayRange(NewFirstDay, NewCount);
}

void SDiurnalScheduleWeekView::SetControllerWorkingRange(
	const double NewRangeMin,
	const double NewRangeMax)
{
	if (!FMath::IsFinite(NewRangeMin) || !FMath::IsFinite(NewRangeMax)) return;
	const int32 NewFirstDay = FMath::Clamp(FMath::RoundToInt(NewRangeMin), 1, MAX_int32 - 1);
	const int32 NewEndDay = FMath::Max(NewFirstDay + 1, FMath::RoundToInt(NewRangeMax));
	ApplyWorkingDayRange(NewFirstDay, NewEndDay - NewFirstDay);
}

void SDiurnalScheduleWeekView::ApplyVisibleDayRange(
	const int32 NewFirstDay,
	const int32 NewVisibleDayCount)
{
	if (FirstVisibleDay == NewFirstDay && VisibleDayCount == NewVisibleDayCount) return;
	FirstVisibleDay = NewFirstDay;
	VisibleDayCount = NewVisibleDayCount;
	UpdateRenderWindow();
}

void SDiurnalScheduleWeekView::PanVisibleRange(const int32 DeltaDays)
{
	if (DeltaDays == 0) return;
	TimelineRangeController->SetViewRange(
		FirstVisibleDay + DeltaDays,
		FirstVisibleDay + VisibleDayCount + DeltaDays,
		EViewRangeInterpolation::Immediate);
}

void SDiurnalScheduleWeekView::UpdateRenderWindow(const bool bForceLayout)
{
	const float Viewport = GetCachedGeometry().GetLocalSize().X;
	const float EffectiveWidth = Viewport > 200.0f ? Viewport : 1050.0f;
	const float DayWidth = FMath::Max(18.0f, (EffectiveWidth - ViewGeometry.RulerWidth) / VisibleDayCount);
	const bool bProjectionChanged = ViewGeometry.FirstVisibleDay != FirstVisibleDay || ViewGeometry.VisibleDayCount != VisibleDayCount;
	const bool bLayoutChanged = bForceLayout || !FMath::IsNearlyEqual(ViewGeometry.DayColumnWidth, DayWidth, 0.5f);
	ViewGeometry.FirstVisibleDay = FirstVisibleDay;
	ViewGeometry.VisibleDayCount = VisibleDayCount;
	ViewGeometry.DayColumnWidth = DayWidth;
	if (!bProjectionChanged && !bLayoutChanged) return;
	if (bProjectionChanged) RebuildProjection();
	else
	{
		RebuildHeader();
		RebuildCanvas();
		if (GridWidget) GridWidget->Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
	}
}

void SDiurnalScheduleWeekView::RebuildProjection()
{
	++ProjectionBuildSerial;
	FDiurnalScheduleProjectionRequest Request;
	if (UDiurnalSchedule* Schedule = Model->GetSchedule()) Request.Layers.Add(FDiurnalScheduleProjectionLayer::FromSchedule(*Schedule));
	Request.FirstVisibleDay = ViewGeometry.FirstVisibleDay;
	Request.VisibleDayCount = ViewGeometry.VisibleDayCount;
	Request.Filter = Model->GetFilter();
	Request.RuntimeCursor = GetDisplayCursor();
	Projection = FDiurnalScheduleProjection::Build(Request);
	RebuildHeader();
	RebuildCanvas();
	if (GridWidget) GridWidget->Invalidate(EInvalidateWidgetReason::Paint);
}

void SDiurnalScheduleWeekView::RebuildHeader()
{
	if (!HeaderRow) return;
	HeaderRow->ClearChildren();
	HeaderRow->AddSlot().AutoWidth()
	[
		SNew(SBox).WidthOverride(ViewGeometry.RulerWidth).HeightOverride(38)
		[
		SNew(SBorder).Padding(6).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FStyleColors::Header)
		[
			SNew(STextBlock).Text(LOCTEXT("Time", "Time")).Font(FAppStyle::GetFontStyle("NormalFontBold"))
		]
		]
	];
	const TOptional<FDiurnalScheduleRuntimeCursor> Cursor = GetDisplayCursor();
	for (int32 Offset = 0; Offset < ViewGeometry.VisibleDayCount; ++Offset)
	{
		const int32 Day = ViewGeometry.FirstVisibleDay + Offset;
		const int32 SelectedDay = Model->GetSelectedDay();
		FLinearColor Tint = Day == SelectedDay ? WithAlpha(FStyleColors::Select.GetSpecifiedColor(), 0.35f) : FStyleColors::Header.GetSpecifiedColor();
		if (Cursor.IsSet() && Cursor->Day == Day && Day != SelectedDay) Tint = WithAlpha(FStyleColors::AccentBlue.GetSpecifiedColor(), Cursor->bIsLiveRuntime ? 0.42f : 0.22f);
		HeaderRow->AddSlot().AutoWidth()
		[
			SNew(SDiurnalTimelineDayHeader).Day(Day).Width(ViewGeometry.DayColumnWidth).Tint(Tint)
			.OnSelected(FOnTimelineDayAction::CreateSP(this, &SDiurnalScheduleWeekView::SelectDay))
			.OnFocused(FOnTimelineDayAction::CreateSP(this, &SDiurnalScheduleWeekView::FocusDayAsSingleDay))
		];
	}
}

void SDiurnalScheduleWeekView::RebuildCanvas()
{
	if (!ItemCanvas) return;
	ItemCanvas->ClearChildren();
	const int32 LabelInterval = ViewGeometry.PixelsPerHour >= 60.0f ? 1 : ViewGeometry.PixelsPerHour >= 38.0f ? 2 : ViewGeometry.PixelsPerHour >= 28.0f ? 3 : 6;
	for (int32 Hour = 0; Hour <= 24; Hour += LabelInterval)
	{
		const float Y = FMath::Clamp(Hour * ViewGeometry.PixelsPerHour - 9.0f, 0.0f, ViewGeometry.GetTimelineHeight() - 18.0f);
		ItemCanvas->AddSlot().Offset(FMargin(3, Y, ViewGeometry.RulerWidth - 7, 18)).Anchors(FAnchors(0)).Alignment(FVector2D::ZeroVector).ZOrder(5)
		[
			SNew(SBorder).Padding(FMargin(2, 0)).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(WithAlpha(FStyleColors::Panel.GetSpecifiedColor(), 0.92f))
			[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%02d:00"), Hour))).Font(FAppStyle::GetFontStyle("SmallFont")).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
		];
	}

	for (const FDiurnalProjectedDay& Day : Projection.Days)
	{
		const float DayX = ViewGeometry.RulerWidth + (Day.Day - ViewGeometry.FirstVisibleDay) * ViewGeometry.DayColumnWidth;
		for (const FProjectedDiurnalRangeSegment& Range : Day.RangeSegments)
		{
			const float LaneWidth = ViewGeometry.DayColumnWidth / FMath::Max(1, Range.OverlapLaneCount);
			const float X = DayX + Range.OverlapLane * LaneWidth + 3;
			const float Y = Range.StartSecond / 3600.0f * ViewGeometry.PixelsPerHour + 2;
			const float Height = FMath::Max(18.0f, (Range.EndSecond - Range.StartSecond) / 3600.0f * ViewGeometry.PixelsPerHour - 4);
			const bool bSelected = Model->GetSelectionType() == EDiurnalScheduleSelectionType::Range && Model->GetSelectedId() == Range.EntryReference.EntryId;
			FString Prefix;
			if (Range.bContinuesFromPreviousDay) Prefix += TEXT("↑ ");
			if (Range.bContinuesIntoNextDay) Prefix += TEXT("↓ ");
			const FText Tooltip = FText::FromString(FString::Printf(TEXT("%s\nDay %d · %02d:%02d–%02d:%02d\n%s\nSource: %s"), *Range.DisplayName.ToString(), Day.Day, Range.StartSecond / 3600, (Range.StartSecond / 60) % 60, Range.EndSecond / 3600, (Range.EndSecond / 60) % 60, *TagsTooltip(Range.Tags), *Range.SourceDisplayName));
			const FText RangePrimary = LaneWidth < 54.0f ? FText::FromString(Prefix.IsEmpty() ? TEXT("■") : Prefix) : FText::FromString(Prefix + Range.DisplayName.ToString());
			const FText RangeSecondary = LaneWidth >= 86.0f ? LOCTEXT("RangeBlock", "Time Range") : FText::GetEmpty();
			ItemCanvas->AddSlot().Offset(FMargin(X, Y, LaneWidth - 6, Height)).Anchors(FAnchors(0)).Alignment(FVector2D::ZeroVector).ZOrder(1)
			[
				SNew(SDiurnalWeekEntryBlock).Type(EDiurnalScheduleSelectionType::Range).EntryId(Range.EntryReference.EntryId).Day(Day.Day).Color(Range.EditorColor).bSelected(bSelected)
				.PrimaryText(RangePrimary).SecondaryText(RangeSecondary).Tooltip(Tooltip)
				.OnSelected(FOnWeekEntrySelected::CreateSP(this, &SDiurnalScheduleWeekView::SelectEntry))
			];
		}
		for (const FProjectedDiurnalEvent& Event : Day.Events)
		{
			const float LaneWidth = ViewGeometry.DayColumnWidth / FMath::Max(1, Event.CollisionLaneCount);
			const float X = DayX + Event.CollisionLane * LaneWidth + 3;
			const float Y = Event.TimeOfDay.ToHours() * ViewGeometry.PixelsPerHour - 18;
			const bool bSelected = Model->GetSelectionType() == EDiurnalScheduleSelectionType::Event && Model->GetSelectedId() == Event.EntryReference.EntryId;
			const FText Secondary = LaneWidth >= 92.0f ? FText::Format(LOCTEXT("EventBlockMeta", "{0} · {1}"), FText::FromString(Event.TimeOfDay.ToString().Left(5)), Event.Behavior == EDiurnalTimeEventBehavior::BlockTime ? LOCTEXT("Blocking", "Blocking") : LOCTEXT("Notify", "Notify")) : LaneWidth >= 62.0f ? FText::FromString(Event.TimeOfDay.ToString().Left(5)) : FText::GetEmpty();
			const FText Primary = LaneWidth < 48.0f ? FText::FromString(TEXT("●")) : FText::FromName(Event.DisplayName);
			const FText Tooltip = FText::FromString(FString::Printf(TEXT("%s\nDay %d · %s\n%s · %s\n%s\nSource: %s"), *Event.DisplayName.ToString(), Day.Day, *Event.TimeOfDay.ToString(), Event.bIsRepeatingOccurrence ? TEXT("Repeating") : TEXT("Once"), Event.Behavior == EDiurnalTimeEventBehavior::BlockTime ? TEXT("Blocking") : TEXT("Notify"), *TagsTooltip(Event.Tags), *Event.SourceDisplayName));
			ItemCanvas->AddSlot().Offset(FMargin(X, FMath::Max(0.0f, Y), LaneWidth - 6, 38)).Anchors(FAnchors(0)).Alignment(FVector2D::ZeroVector).ZOrder(2)
			[
				SNew(SDiurnalWeekEntryBlock).Type(EDiurnalScheduleSelectionType::Event).EntryId(Event.EntryReference.EntryId).Day(Day.Day).Color(Event.EditorColor).bSelected(bSelected)
				.PrimaryText(Primary).SecondaryText(Secondary).Tooltip(Tooltip)
				.OnSelected(FOnWeekEntrySelected::CreateSP(this, &SDiurnalScheduleWeekView::SelectEntry))
			];
		}
	}
}

void SDiurnalScheduleWeekView::SelectEntry(const EDiurnalScheduleSelectionType Type, const FGuid EntryId, const int32 Day)
{
	Model->SelectEntryOccurrence(Type, EntryId, Day);
}

void SDiurnalScheduleWeekView::AddEventAt(const int32 Day, const FDiurnalTimeOfDay TimeOfDay)
{
	Model->AddOnceEventAt(Day, FDiurnalWeekViewGeometry::SnapTime(TimeOfDay));
}

void SDiurnalScheduleWeekView::AddRangeAt(const int32 Day, const FDiurnalTimeOfDay TimeOfDay)
{
	Model->AddRangeAt(FDiurnalWeekViewGeometry::SnapTime(TimeOfDay), 60, Day);
}

void SDiurnalScheduleWeekView::HandleBeginPIE(bool)
{
	bPIESessionExpected = true;
	UnbindRuntime();
	RuntimeCursor.Reset();
	TryBindOrBeginRuntimeDiscovery();
	RebuildProjection();
}

void SDiurnalScheduleWeekView::HandlePostPIEStarted(bool)
{
	bPIESessionExpected = true;
	TryBindOrBeginRuntimeDiscovery();
	RebuildProjection();
}

void SDiurnalScheduleWeekView::HandleEndPIE(bool)
{
	bPIESessionExpected = false;
	StopRuntimeDiscovery();
	UnbindRuntime();
	RuntimeCursor.Reset();
	RebuildProjection();
}

void SDiurnalScheduleWeekView::HandleTimeChanged(const FDiurnalTimeChange& Change)
{
	const int32 PreviousDay = RuntimeCursor.IsSet() ? RuntimeCursor->Day : INDEX_NONE;
	FDiurnalScheduleRuntimeCursor Cursor;
	Cursor.Day = Change.CurrentDateTime.Day;
	Cursor.TimeOfDay = Change.CurrentDateTime.GetTimeOfDay();
	Cursor.bIsLiveRuntime = true;
	RuntimeCursor = Cursor;
	if (PreviousDay != Cursor.Day)
	{
		RebuildProjection();
	}
	else if (GridWidget)
	{
		GridWidget->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SDiurnalScheduleWeekView::TryBindOrBeginRuntimeDiscovery()
{
	if (RuntimeSubsystem.IsValid() && !IsBoundToCurrentPlayWorld())
	{
		UnbindRuntime();
		RuntimeCursor.Reset();
	}
	if (TryBindRuntime())
	{
		StopRuntimeDiscovery();
		return;
	}
	if (bPIESessionExpected && !DiscoveryTimer.IsValid())
	{
		DiscoveryTimer = RegisterActiveTimer(
			0.25f,
			FWidgetActiveTimerDelegate::CreateSP(this, &SDiurnalScheduleWeekView::DiscoverRuntime));
	}
}

void SDiurnalScheduleWeekView::StopRuntimeDiscovery()
{
	if (const TSharedPtr<FActiveTimerHandle> Handle = DiscoveryTimer.Pin())
	{
		UnRegisterActiveTimer(Handle.ToSharedRef());
	}
	DiscoveryTimer.Reset();
}

bool SDiurnalScheduleWeekView::IsBoundToCurrentPlayWorld() const
{
	if (!RuntimeSubsystem.IsValid() || !GEditor || !IsValid(GEditor->PlayWorld)) return false;
	const UGameInstance* BoundGameInstance = Cast<UGameInstance>(RuntimeSubsystem->GetOuter());
	return BoundGameInstance && BoundGameInstance == GEditor->PlayWorld->GetGameInstance();
}

bool SDiurnalScheduleWeekView::TryBindRuntime()
{
	if (IsBoundToCurrentPlayWorld()) return true;
	if (!GEditor || !IsValid(GEditor->PlayWorld) || !GEditor->PlayWorld->GetGameInstance()) return false;
	UDiurnalCycleSubsystem* Subsystem = GEditor->PlayWorld->GetGameInstance()->GetSubsystem<UDiurnalCycleSubsystem>();
	if (!Subsystem) return false;
	RuntimeSubsystem = Subsystem;
	TimeChangedHandle = Subsystem->OnTimeChanged().AddSP(this, &SDiurnalScheduleWeekView::HandleTimeChanged);
	FDiurnalScheduleRuntimeCursor Cursor;
	Cursor.Day = Subsystem->GetCurrentDay();
	Cursor.TimeOfDay = Subsystem->GetTimeOfDay();
	Cursor.bIsLiveRuntime = true;
	RuntimeCursor = Cursor;
	return true;
}

void SDiurnalScheduleWeekView::UnbindRuntime()
{
	if (UDiurnalCycleSubsystem* Subsystem = RuntimeSubsystem.Get(); Subsystem && TimeChangedHandle.IsValid()) Subsystem->OnTimeChanged().Remove(TimeChangedHandle);
	TimeChangedHandle.Reset();
	RuntimeSubsystem.Reset();
}

EActiveTimerReturnType SDiurnalScheduleWeekView::DiscoverRuntime(double, float)
{
	if (!bPIESessionExpected)
	{
		DiscoveryTimer.Reset();
		return EActiveTimerReturnType::Stop;
	}
	if (!TryBindRuntime()) return EActiveTimerReturnType::Continue;
	DiscoveryTimer.Reset();
	RebuildProjection();
	return EActiveTimerReturnType::Stop;
}

TOptional<FDiurnalScheduleRuntimeCursor> SDiurnalScheduleWeekView::GetDisplayCursor() const
{
	if (RuntimeSubsystem.IsValid() && RuntimeCursor.IsSet()) return RuntimeCursor;
	const FDiurnalDateTime Start = GetDefault<UDiurnalCycleSettings>()->StartingDateTime;
	FDiurnalScheduleRuntimeCursor Cursor;
	Cursor.Day = Start.Day;
	Cursor.TimeOfDay = Start.GetTimeOfDay();
	Cursor.bIsLiveRuntime = false;
	return Cursor;
}

EVisibility SDiurnalScheduleWeekView::GetFilteredSelectionVisibility() const
{
	if (Model->GetSelectionType() == EDiurnalScheduleSelectionType::None || !Model->GetFilter().IsActive()) return EVisibility::Collapsed;
	for (const FDiurnalProjectedDay& Day : Projection.Days)
	{
		if (Model->GetSelectionType() == EDiurnalScheduleSelectionType::Event && Day.Events.ContainsByPredicate([this](const FProjectedDiurnalEvent& Event) { return Event.EntryReference.EntryId == Model->GetSelectedId(); })) return EVisibility::Collapsed;
		if (Model->GetSelectionType() == EDiurnalScheduleSelectionType::Range && Day.RangeSegments.ContainsByPredicate([this](const FProjectedDiurnalRangeSegment& Range) { return Range.EntryReference.EntryId == Model->GetSelectedId(); })) return EVisibility::Collapsed;
	}
	return EVisibility::HitTestInvisible;
}

FText SDiurnalScheduleWeekView::GetFilteredSelectionText() const
{
	return LOCTEXT("FilteredSelection", "The selected entry is hidden by the current Timeline search or filters.");
}

#undef LOCTEXT_NAMESPACE
