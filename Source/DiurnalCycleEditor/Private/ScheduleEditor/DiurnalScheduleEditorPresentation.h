#pragma once

#include "CoreMinimal.h"
#include "DiurnalCycleTypes.h"

struct FDiurnalTagChipProjection
{
	TArray<FGameplayTag> VisibleTags;
	int32 OverflowCount = 0;
	FText OverflowTooltip;
};

namespace DiurnalScheduleEditor
{
	constexpr int32 DefaultVisibleTagChips = 2;

	DIURNALCYCLEEDITOR_API FLinearColor GetAutomaticEventColor(const FDiurnalTimeEvent& Event);
	DIURNALCYCLEEDITOR_API FLinearColor GetAutomaticRangeColor(const FDiurnalTimeRange& Range);
	DIURNALCYCLEEDITOR_API FLinearColor GetEditorColor(const FDiurnalTimeEvent& Event);
	DIURNALCYCLEEDITOR_API FLinearColor GetEditorColor(const FDiurnalTimeRange& Range);

	/** Sorts tags by full path, then returns a deterministic visible/overflow split. */
	DIURNALCYCLEEDITOR_API FDiurnalTagChipProjection ProjectTagChips(
		const FGameplayTagContainer& Tags,
		int32 MaximumVisible = DefaultVisibleTagChips);
}
