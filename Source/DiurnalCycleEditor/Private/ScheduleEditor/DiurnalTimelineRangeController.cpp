#include "ScheduleEditor/DiurnalTimelineRangeController.h"

#include "AnimatedRange.h"
#include "ScheduleEditor/Widgets/SDiurnalScheduleWeekView.h"

FDiurnalTimelineRangeController::FDiurnalTimelineRangeController(
	const TWeakPtr<SDiurnalScheduleWeekView> InTimeline)
	: Timeline(InTimeline)
{
}

void FDiurnalTimelineRangeController::SetRangeWidget(const TWeakPtr<SWidget> InRangeWidget)
{
	RangeWidget = InRangeWidget;
}

int32 FDiurnalTimelineRangeController::OnPaintTimeSlider(
	bool, const FGeometry&, const FSlateRect&, FSlateWindowElementList&,
	const int32 LayerId, const FWidgetStyle&, bool) const
{
	return LayerId;
}

int32 FDiurnalTimelineRangeController::OnPaintViewArea(
	const FGeometry&, const FSlateRect&, FSlateWindowElementList&,
	const int32 LayerId, bool, const FPaintViewAreaArgs&) const
{
	return LayerId;
}

FFrameRate FDiurnalTimelineRangeController::GetDisplayRate() const
{
	return FFrameRate(1, 1);
}

FFrameRate FDiurnalTimelineRangeController::GetTickResolution() const
{
	return FFrameRate(1, 1);
}

FAnimatedRange FDiurnalTimelineRangeController::GetViewRange() const
{
	if (const TSharedPtr<SDiurnalScheduleWeekView> Pinned = Timeline.Pin())
	{
		return FAnimatedRange(
			Pinned->GetFirstVisibleDay(),
			Pinned->GetFirstVisibleDay() + Pinned->GetVisibleSpanDays());
	}
	return FAnimatedRange(1.0, 8.0);
}

FAnimatedRange FDiurnalTimelineRangeController::GetClampRange() const
{
	if (const TSharedPtr<SDiurnalScheduleWeekView> Pinned = Timeline.Pin())
	{
		return FAnimatedRange(
			Pinned->GetWorkingFirstDay(),
			Pinned->GetWorkingFirstDay() + Pinned->GetWorkingDayCount());
	}
	return FAnimatedRange(1.0, 29.0);
}

void FDiurnalTimelineRangeController::SetViewRange(
	const double NewRangeMin,
	const double NewRangeMax,
	EViewRangeInterpolation)
{
	if (const TSharedPtr<SDiurnalScheduleWeekView> Pinned = Timeline.Pin())
	{
		Pinned->SetControllerViewRange(NewRangeMin, NewRangeMax);
	}
	if (const TSharedPtr<SWidget> PinnedWidget = RangeWidget.Pin())
	{
		PinnedWidget->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void FDiurnalTimelineRangeController::SetClampRange(
	const double NewRangeMin,
	const double NewRangeMax)
{
	if (const TSharedPtr<SDiurnalScheduleWeekView> Pinned = Timeline.Pin())
	{
		Pinned->SetControllerWorkingRange(NewRangeMin, NewRangeMax);
	}
	if (const TSharedPtr<SWidget> PinnedWidget = RangeWidget.Pin())
	{
		PinnedWidget->Invalidate(EInvalidateWidgetReason::Paint);
	}
}
