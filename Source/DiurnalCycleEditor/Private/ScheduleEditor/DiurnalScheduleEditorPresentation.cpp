#include "ScheduleEditor/DiurnalScheduleEditorPresentation.h"
#include "DiurnalCycleSettings.h"

namespace
{
	FLinearColor Opaque(FLinearColor Color)
	{
		Color.A = 1.0f;
		return Color;
	}
}

FLinearColor DiurnalScheduleEditor::GetAutomaticEventColor(const FDiurnalTimeEvent& Event)
{
	const UDiurnalCycleSettings* Settings = GetDefault<UDiurnalCycleSettings>();
	const bool bOnce = Event.Recurrence.Mode == EDiurnalRecurrenceMode::Once;
	if (Event.IsBlocking())
	{
		return Opaque(bOnce ? Settings->OnceGateColor : Settings->RepeatingGateColor);
	}
	return Opaque(bOnce ? Settings->OnceEventColor : Settings->RepeatingEventColor);
}

FLinearColor DiurnalScheduleEditor::GetAutomaticRangeColor(const FDiurnalTimeRange& Range)
{
	const UDiurnalCycleSettings* Settings = GetDefault<UDiurnalCycleSettings>();
	return Opaque(Range.Recurrence.Mode == EDiurnalRecurrenceMode::Once
		? Settings->OnceRangeColor
		: Settings->RepeatingRangeColor);
}

FLinearColor DiurnalScheduleEditor::GetEditorColor(const FDiurnalTimeEvent& Event)
{
#if WITH_EDITORONLY_DATA
	if (Event.bOverrideEditorColor)
	{
		return Event.EditorColor;
	}
#endif
	return GetAutomaticEventColor(Event);
}

FLinearColor DiurnalScheduleEditor::GetEditorColor(const FDiurnalTimeRange& Range)
{
#if WITH_EDITORONLY_DATA
	if (Range.bOverrideEditorColor)
	{
		return Range.EditorColor;
	}
#endif
	return GetAutomaticRangeColor(Range);
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
