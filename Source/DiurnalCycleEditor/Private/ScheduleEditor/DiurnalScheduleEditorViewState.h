#pragma once

#include "CoreMinimal.h"
#include "DiurnalCycleTypes.h"

class UDiurnalSchedule;

enum class EDiurnalScheduleSelectionType : uint8
{
	None,
	Event,
	Range
};

enum class EDiurnalScheduleSortMode : uint8
{
	ManualOrder,
	Name,
	Type,
	DayAndTime,
	TimeOfDay
};

/** Shared, orthogonal editor filtering used by List, Week, and the Browser. */
struct FDiurnalScheduleEditorFilter
{
	FString SearchText;
	bool bShowRepeatingEvents = true;
	bool bShowOnceEvents = true;
	bool bShowTimeRanges = true;
	bool bShowNotify = true;
	bool bShowBlocking = true;

	bool IsActive() const;
	void Reset();
	bool MatchesEvent(const FDiurnalTimeEvent& Event, const FString& SourceName = FString()) const;
	bool MatchesRange(const FDiurnalTimeRange& Range, const FString& SourceName = FString()) const;
};

/** One display-only List row. It never owns or mutates authored data. */
struct FDiurnalScheduleListItem
{
	EDiurnalScheduleSelectionType Type = EDiurnalScheduleSelectionType::None;
	FGuid EntryId;
	int32 ManualIndex = INDEX_NONE;
	int32 ManualOrdinal = INDEX_NONE;
	FName DisplayName;
	FGameplayTagContainer Tags;
	FDiurnalTimeOfDay StartTime;
	int32 AnchorDay = 0;
	bool bOneOff = false;
	bool bBlocking = false;
};

namespace DiurnalScheduleEditor
{
	DIURNALCYCLEEDITOR_API FText GetSortModeText(EDiurnalScheduleSortMode SortMode);

	/**
	 * Builds a stable display projection without changing either authored array.
	 * Manual preserves event-array order followed by range-array order. Name is
	 * case-insensitive. Type groups events before ranges with chronological
	 * secondary ordering. DayAndTime groups repeating events, one-off events, then
	 * ranges. TimeOfDay compares only the occurrence/range start first. Every
	 * mode resolves ties by authored ordinal and EntryId.
	 */
	DIURNALCYCLEEDITOR_API TArray<FDiurnalScheduleListItem> BuildListItems(
		const UDiurnalSchedule& Schedule,
		const FDiurnalScheduleEditorFilter& Filter,
		EDiurnalScheduleSortMode SortMode);
}
