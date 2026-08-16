#include "ScheduleEditor/DiurnalScheduleEditorPresentation.h"

namespace
{
	const TArray<FLinearColor>& EventPalette()
	{
		static const TArray<FLinearColor> Palette =
		{
			FLinearColor(0.30f, 0.45f, 0.74f),
			FLinearColor(0.39f, 0.39f, 0.70f),
			FLinearColor(0.43f, 0.34f, 0.66f),
			FLinearColor(0.28f, 0.53f, 0.69f),
			FLinearColor(0.48f, 0.39f, 0.63f)
		};
		return Palette;
	}

	const TArray<FLinearColor>& RangePalette()
	{
		static const TArray<FLinearColor> Palette =
		{
			FLinearColor(0.22f, 0.55f, 0.48f),
			FLinearColor(0.27f, 0.50f, 0.42f),
			FLinearColor(0.20f, 0.49f, 0.53f),
			FLinearColor(0.31f, 0.57f, 0.39f),
			FLinearColor(0.24f, 0.46f, 0.45f)
		};
		return Palette;
	}

	FLinearColor PaletteColor(const FGuid& EntryId, const TArray<FLinearColor>& Palette)
	{
		const uint32 Hash = HashCombine(HashCombine(EntryId.A, EntryId.B), HashCombine(EntryId.C, EntryId.D));
		FLinearColor Result = Palette[Hash % Palette.Num()];
		Result.A = 1.0f;
		return Result;
	}
}

FLinearColor DiurnalScheduleEditor::GetAutomaticEventColor(const FGuid& EntryId)
{
	return PaletteColor(EntryId, EventPalette());
}

FLinearColor DiurnalScheduleEditor::GetAutomaticRangeColor(const FGuid& EntryId)
{
	return PaletteColor(EntryId, RangePalette());
}

FLinearColor DiurnalScheduleEditor::GetEditorColor(const FDiurnalTimeEvent& Event)
{
#if WITH_EDITORONLY_DATA
	if (Event.bOverrideEditorColor)
	{
		return Event.EditorColor;
	}
#endif
	return GetAutomaticEventColor(Event.EntryId);
}

FLinearColor DiurnalScheduleEditor::GetEditorColor(const FDiurnalTimeRange& Range)
{
#if WITH_EDITORONLY_DATA
	if (Range.bOverrideEditorColor)
	{
		return Range.EditorColor;
	}
#endif
	return GetAutomaticRangeColor(Range.EntryId);
}

FDiurnalTagChipProjection DiurnalScheduleEditor::ProjectTagChips(
	const FGameplayTagContainer& Tags,
	const int32 MaximumVisible)
{
	FDiurnalTagChipProjection Result;
	TArray<FGameplayTag> Sorted = Tags.GetGameplayTagArray();
	Sorted.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
	{
		return Left.ToString() < Right.ToString();
	});
	const int32 VisibleCount = FMath::Clamp(MaximumVisible, 0, Sorted.Num());
	if (VisibleCount > 0)
	{
		Result.VisibleTags.Append(Sorted.GetData(), VisibleCount);
	}
	Result.OverflowCount = Sorted.Num() - VisibleCount;
	if (Result.OverflowCount > 0)
	{
		FString Tooltip;
		for (int32 Index = VisibleCount; Index < Sorted.Num(); ++Index)
		{
			if (!Tooltip.IsEmpty()) Tooltip += TEXT("\n");
			Tooltip += Sorted[Index].ToString();
		}
		Result.OverflowTooltip = FText::FromString(Tooltip);
	}
	return Result;
}
