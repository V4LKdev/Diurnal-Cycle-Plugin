#pragma once

#include "CoreMinimal.h"
#include "DiurnalCycleTypes.h"
#include "ScheduleEditor/DiurnalScheduleEditorViewState.h"

class UDiurnalSchedule;

/** Origin of a layer shown by an editor projection. */
enum class EDiurnalScheduleProjectionProvenance : uint8
{
	AuthoredAsset,
	RuntimeOverlay
};

/** Immutable input snapshot for one ordered schedule layer. */
struct DIURNALCYCLEEDITOR_API FDiurnalScheduleProjectionLayer
{
	TSoftObjectPtr<UDiurnalSchedule> SourceSchedule;
	FString SourceDisplayName;
	int32 LayerOrder = 0;
	EDiurnalScheduleProjectionProvenance Provenance = EDiurnalScheduleProjectionProvenance::AuthoredAsset;
	bool bReadOnly = false;
	TArray<FDiurnalTimeEvent> Events;
	TArray<FDiurnalTimeRange> Ranges;

	static FDiurnalScheduleProjectionLayer FromSchedule(UDiurnalSchedule& Schedule, int32 LayerOrder = 0, bool bReadOnly = false);
};

/** Optional observational clock marker supplied by the caller. */
struct DIURNALCYCLEEDITOR_API FDiurnalScheduleRuntimeCursor
{
	int32 Day = 1;
	FDiurnalTimeOfDay TimeOfDay;
	bool bIsLiveRuntime = false;
};

struct DIURNALCYCLEEDITOR_API FProjectedDiurnalEvent
{
	FDiurnalScheduleEntryReference EntryReference;
	TSoftObjectPtr<UDiurnalSchedule> SourceSchedule;
	FString SourceDisplayName;
	int32 SourceLayer = 0;
	EDiurnalScheduleProjectionProvenance Provenance = EDiurnalScheduleProjectionProvenance::AuthoredAsset;
	bool bReadOnly = false;
	FName DisplayName;
	FGameplayTagContainer Tags;
	int32 VisibleDay = 1;
	FDiurnalTimeOfDay TimeOfDay;
	EDiurnalTimeEventBehavior Behavior = EDiurnalTimeEventBehavior::Notify;
	bool bIsRepeatingOccurrence = false;
	FLinearColor EditorColor = FLinearColor::White;
	int32 CollisionLane = 0;
	int32 CollisionLaneCount = 1;
};

/** A visible piece of a recurring range. Seconds are [0, 86400], allowing a 24:00 edge. */
struct DIURNALCYCLEEDITOR_API FProjectedDiurnalRangeSegment
{
	FDiurnalScheduleEntryReference EntryReference;
	TSoftObjectPtr<UDiurnalSchedule> SourceSchedule;
	FString SourceDisplayName;
	int32 SourceLayer = 0;
	EDiurnalScheduleProjectionProvenance Provenance = EDiurnalScheduleProjectionProvenance::AuthoredAsset;
	bool bReadOnly = false;
	FName DisplayName;
	FGameplayTagContainer Tags;
	int32 VisibleDay = 1;
	int32 StartSecond = 0;
	int32 EndSecond = static_cast<int32>(DiurnalCycle::GSecondsPerDay);
	bool bContinuesFromPreviousDay = false;
	bool bContinuesIntoNextDay = false;
	bool bIsRepeatingOccurrence = false;
	FLinearColor EditorColor = FLinearColor::White;
	int32 OverlapLane = 0;
	int32 OverlapLaneCount = 1;
};

struct DIURNALCYCLEEDITOR_API FDiurnalProjectedDay
{
	int32 Day = 1;
	TArray<FProjectedDiurnalEvent> Events;
	TArray<FProjectedDiurnalRangeSegment> RangeSegments;
	bool bIsRuntimeDay = false;
};

struct DIURNALCYCLEEDITOR_API FDiurnalScheduleProjectionRequest
{
	TArray<FDiurnalScheduleProjectionLayer> Layers;
	int32 FirstVisibleDay = 1;
	int32 VisibleDayCount = 7;
	FDiurnalScheduleEditorFilter Filter;
	EDiurnalScheduleSortMode SortMode = EDiurnalScheduleSortMode::DayAndTime;
	TOptional<FDiurnalScheduleRuntimeCursor> RuntimeCursor;
};

struct DIURNALCYCLEEDITOR_API FDiurnalScheduleProjectionResult
{
	int32 FirstVisibleDay = 1;
	int32 VisibleDayCount = 0;
	TArray<FDiurnalProjectedDay> Days;
	TOptional<FDiurnalScheduleRuntimeCursor> RuntimeCursor;

	const FDiurnalProjectedDay* FindDay(int32 Day) const;
};

/** Pure, deterministic schedule-to-visible-occurrence projection. */
class DIURNALCYCLEEDITOR_API FDiurnalScheduleProjection
{
public:
	static FDiurnalScheduleProjectionResult Build(const FDiurnalScheduleProjectionRequest& Request);
};
