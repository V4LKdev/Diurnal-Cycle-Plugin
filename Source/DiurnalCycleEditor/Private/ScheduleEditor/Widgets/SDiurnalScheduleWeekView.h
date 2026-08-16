#pragma once

#include "CoreMinimal.h"
#include "DiurnalCycleTypes.h"
#include "ScheduleEditor/Projection/DiurnalScheduleProjection.h"
#include "Widgets/SCompoundWidget.h"

class FDiurnalScheduleEditorModel;
class FDiurnalTimelineRangeController;
class ITimeSliderController;
class SConstraintCanvas;
class SHorizontalBox;
class SScrollBox;
class UDiurnalCycleSubsystem;

struct DIURNALCYCLEEDITOR_API FDiurnalWeekHit
{
	int32 Day = 1;
	FDiurnalTimeOfDay TimeOfDay;
};

/** Pixel/time conversion kept independent from Slate input handling for deterministic testing. */
struct DIURNALCYCLEEDITOR_API FDiurnalWeekViewGeometry
{
	int32 FirstVisibleDay = 1;
	int32 VisibleDayCount = 42;
	float RulerWidth = 64.0f;
	float DayColumnWidth = 150.0f;
	float PixelsPerHour = 48.0f;

	float GetTimelineHeight() const { return 24.0f * PixelsPerHour; }
	float GetTotalWidth() const { return RulerWidth + VisibleDayCount * DayColumnWidth; }
	FVector2D DayAndTimeToPosition(int32 Day, const FDiurnalTimeOfDay& TimeOfDay) const;
	bool PositionToDayAndTime(const FVector2D& LocalPosition, FDiurnalWeekHit& OutHit, int32 SnapMinutes = 15) const;
	static FDiurnalTimeOfDay SnapTime(const FDiurnalTimeOfDay& TimeOfDay, int32 SnapMinutes = 15);
};

/** Rolling, zoomable day timeline with selection and non-drag creation. */
class DIURNALCYCLEEDITOR_API SDiurnalScheduleWeekView final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDiurnalScheduleWeekView) {}
		SLATE_ARGUMENT(TSharedPtr<FDiurnalScheduleEditorModel>, Model)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);
	virtual ~SDiurnalScheduleWeekView() override;
	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	void GoToCurrent();
	void SetPixelsPerHour(float Value);
	void SetVisibleDaysPreset(int32 Days);
	void ResetView();
	float GetPixelsPerHour() const { return ViewGeometry.PixelsPerHour; }
	int32 GetFirstVisibleDay() const { return FirstVisibleDay; }
	int32 GetVisibleSpanDays() const { return VisibleDayCount; }
	int32 GetWorkingFirstDay() const { return WorkingFirstDay; }
	int32 GetWorkingDayCount() const { return WorkingDayCount; }
	TSharedRef<ITimeSliderController> GetTimelineRangeController() const;
	FText GetVisibleRangeText() const;
	FText GetRuntimeMarkerText() const;
	void SetVisibleDayRange(int32 NewFirstDay, int32 NewVisibleDayCount);
	void Activate();
	EVisibility GetItemCanvasVisibility() const;
	bool IsObservingLiveRuntime() const { return RuntimeSubsystem.IsValid() && RuntimeCursor.IsSet(); }
	bool IsRuntimeDiscoveryActive() const { return DiscoveryTimer.IsValid(); }
	TOptional<FDiurnalScheduleRuntimeCursor> GetObservedRuntimeCursor() const { return RuntimeCursor; }
	const FDiurnalScheduleProjectionResult& GetProjection() const { return Projection; }
	uint32 GetProjectionBuildSerial() const { return ProjectionBuildSerial; }

	static int32 ResolveCurrentDay(bool bIsPIE, const TOptional<FDiurnalScheduleRuntimeCursor>& RuntimeCursor, int32 ConfiguredStartDay);
	static float ClampVerticalScrollOffset(float RequestedOffset, float MaximumOffset);
	static float ApplyVerticalWheelDelta(float CurrentOffset, float WheelDelta, float ScrollStep, float MaximumOffset);
	/** Six weeks keeps a usable minimum column width while allowing monthly and multi-week overview. */
	static constexpr int32 MaximumVisibleDays = 42;
	static constexpr int32 MinimumWorkingDays = 28;

private:
	friend class FDiurnalTimelineRangeController;

	void RebuildProjection();
	void RebuildHeader();
	void RebuildCanvas();
	void FocusDay(int32 Day, bool bCenter = true);
	void SelectDay(int32 Day);
	void FocusDayAsSingleDay(int32 Day);
	void ResetWorkingRange(int32 TargetDay, bool bConstrainView = true);
	void RepositionWorkingRangeAround(int32 TargetDay, bool bConstrainView = true);
	void EnsureWorkingRangeContains(int32 Day);
	void ApplyWorkingDayRange(int32 NewFirstDay, int32 NewWorkingDayCount, bool bConstrainView = true);
	void ApplyVisibleDayRange(int32 NewFirstDay, int32 NewVisibleDayCount);
	void SetControllerViewRange(double NewRangeMin, double NewRangeMax);
	void SetControllerWorkingRange(double NewRangeMin, double NewRangeMax);
	void UpdateRenderWindow(bool bForceLayout = false);
	void PanVisibleRange(int32 DeltaDays);
	void SelectEntry(EDiurnalScheduleSelectionType Type, FGuid EntryId, int32 Day);
	void AddEventAt(int32 Day, FDiurnalTimeOfDay TimeOfDay);
	void AddRangeAt(int32 Day, FDiurnalTimeOfDay TimeOfDay);
	void HandleBeginPIE(bool bIsSimulating);
	void HandlePostPIEStarted(bool bIsSimulating);
	void HandleEndPIE(bool bIsSimulating);
	void HandleTimeChanged(const FDiurnalTimeChange& Change);
	void TryBindOrBeginRuntimeDiscovery();
	void StopRuntimeDiscovery();
	bool IsBoundToCurrentPlayWorld() const;
	bool TryBindRuntime();
	void UnbindRuntime();
	EActiveTimerReturnType DiscoverRuntime(double CurrentTime, float DeltaTime);
	TOptional<FDiurnalScheduleRuntimeCursor> GetDisplayCursor() const;
	EVisibility GetFilteredSelectionVisibility() const;
	FText GetFilteredSelectionText() const;

	TSharedPtr<FDiurnalScheduleEditorModel> Model;
	FDiurnalWeekViewGeometry ViewGeometry;
	FDiurnalScheduleProjectionResult Projection;
	TSharedPtr<SHorizontalBox> HeaderRow;
	TSharedPtr<SConstraintCanvas> ItemCanvas;
	TSharedPtr<SWidget> GridWidget;
	TSharedPtr<SScrollBox> VerticalScroll;
	TSharedPtr<FDiurnalTimelineRangeController> TimelineRangeController;
	TWeakObjectPtr<UDiurnalCycleSubsystem> RuntimeSubsystem;
	TOptional<FDiurnalScheduleRuntimeCursor> RuntimeCursor;
	FDelegateHandle BeginPIEHandle;
	FDelegateHandle PostPIEStartedHandle;
	FDelegateHandle EndPIEHandle;
	FDelegateHandle TimeChangedHandle;
	TWeakPtr<FActiveTimerHandle> DiscoveryTimer;
	int32 FirstVisibleDay = 1;
	int32 VisibleDayCount = 7;
	int32 WorkingFirstDay = 1;
	int32 WorkingDayCount = MinimumWorkingDays;
	uint32 ProjectionBuildSerial = 0;
	float LastViewportWidth = 0.0f;
	bool bPIESessionExpected = false;
	bool bMiddlePanning = false;
	FVector2D MiddlePanOrigin;
	int32 MiddlePanFirstDay = 1;
};
