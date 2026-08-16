#pragma once

#include "ITimeSlider.h"

class SDiurnalScheduleWeekView;

/** Minimal SequencerWidgets adaptor for the Timeline's editor-only integer day ranges. */
class FDiurnalTimelineRangeController final : public ITimeSliderController
{
public:
	explicit FDiurnalTimelineRangeController(TWeakPtr<SDiurnalScheduleWeekView> InTimeline);
	void SetRangeWidget(TWeakPtr<SWidget> InRangeWidget);

	virtual int32 OnPaintTimeSlider(bool bMirrorLabels, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual int32 OnPaintViewArea(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bEnabled,
		const FPaintViewAreaArgs& Args) const override;
	virtual FFrameRate GetDisplayRate() const override;
	virtual FFrameRate GetTickResolution() const override;
	virtual FAnimatedRange GetViewRange() const override;
	virtual FAnimatedRange GetClampRange() const override;
	virtual void SetViewRange(double NewRangeMin, double NewRangeMax,
		EViewRangeInterpolation Interpolation) override;
	virtual void SetClampRange(double NewRangeMin, double NewRangeMax) override;

private:
	TWeakPtr<SDiurnalScheduleWeekView> Timeline;
	TWeakPtr<SWidget> RangeWidget;
};
