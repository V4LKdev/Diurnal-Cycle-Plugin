
#include "DiurnalCycleSettings.h"
#include "DiurnalCycleGameplayTags.h"

UDiurnalCycleSettings::UDiurnalCycleSettings()
{
	// Add Array Defaults
	TimeEvents.Emplace(
		DiurnalCycle::TimeEvent::DailyExample,
		FDiurnalTimeOfDay(6));

	TimeEvents.Emplace(
		DiurnalCycle::TimeEvent::DatedExample,
		FDiurnalTimeOfDay(18),
		true,
		2);

	TimeRanges.Emplace(
		DiurnalCycle::TimeRange::DayTime,
		FDiurnalTimeOfDay(6),
		FDiurnalTimeOfDay(18));

	TimeRanges.Emplace(
		DiurnalCycle::TimeRange::NightTime,
		FDiurnalTimeOfDay(18),
		FDiurnalTimeOfDay(6));
}

#if WITH_EDITOR

#define LOCTEXT_NAMESPACE "DiurnalCycleSettings"

EDataValidationResult UDiurnalCycleSettings::IsDataValid(
	FDataValidationContext& Context) const
{
	const EDataValidationResult SuperResult =
		Super::IsDataValid(Context);

	bool bIsValid =
		SuperResult != EDataValidationResult::Invalid;

	const auto AddError =
		[&Context, &bIsValid](
			const FText& Error)
		{
			Context.AddError(Error);
			bIsValid = false;
		};

#pragma region Clock

	if (!DiurnalCycle::IsValidRealSecondsPerGameHour(
			RealSecondsPerGameHour))
	{
		AddError(
			FText::Format(
				LOCTEXT(
					"InvalidRealSecondsPerGameHour",
					"Real Seconds Per Game Hour must be finite and at "
					"least {0}. Current value: {1}."),
				FText::AsNumber(
					DiurnalCycle::GMinimumRealSecondsPerGameHour),
				FText::AsNumber(
					RealSecondsPerGameHour)));
	}

	if (!StartingDateTime.IsValid())
	{
		AddError(
			FText::Format(
				LOCTEXT(
					"InvalidStartingDateTime",
					"Starting Date Time is invalid: "
					"Day {0}, {1}:{2}:{3}."),
				FText::AsNumber(
					StartingDateTime.Day),
				FText::AsNumber(
					StartingDateTime.Hour),
				FText::AsNumber(
					StartingDateTime.Minute),
				FText::AsNumber(
					StartingDateTime.Second)));
	}

	if (!DiurnalCycle::IsValidTimeScale(
			DefaultTimeScale))
	{
		AddError(
			FText::Format(
				LOCTEXT(
					"InvalidDefaultTimeScale",
					"Default Time Scale must be finite and in the "
					"range [{0}, {1}]. Current value: {2}."),
				FText::AsNumber(
					DiurnalCycle::GMinimumTimeScale),
				FText::AsNumber(
					DiurnalCycle::GMaximumTimeScale),
				FText::AsNumber(
					DefaultTimeScale)));
	}

#pragma endregion

#pragma region Events

	TSet<FGameplayTag> SeenEventTags;

	for (int32 Index = 0;
		 Index < TimeEvents.Num();
		 ++Index)
	{
		const FDiurnalTimeEvent& Event =
			TimeEvents[Index];

		if (!Event.EventTag.IsValid())
		{
			AddError(
				FText::Format(
					LOCTEXT(
						"InvalidTimeEventTag",
						"Time Events[{0}] has no valid event tag."),
					FText::AsNumber(
						Index)));
		}
		else if (SeenEventTags.Contains(
					Event.EventTag))
		{
			AddError(
				FText::Format(
					LOCTEXT(
						"DuplicateTimeEventTag",
						"Time Events[{0}] duplicates the tag '{1}'."),
					FText::AsNumber(
						Index),
					FText::FromString(
						Event.EventTag.ToString())));
		}
		else
		{
			SeenEventTags.Add(
				Event.EventTag);
		}

		if (!Event.TimeOfDay.IsValid())
		{
			AddError(
				FText::Format(
					LOCTEXT(
						"InvalidTimeEventTimeOfDay",
						"Time Events[{0}] ('{1}') has an invalid "
						"time of day."),
					FText::AsNumber(
						Index),
					FText::FromString(
						Event.EventTag.ToString())));
		}

		if (Event.bDatedEvent
			&& Event.EventDay < 1)
		{
			AddError(
				FText::Format(
					LOCTEXT(
						"InvalidDatedEventDay",
						"Time Events[{0}] ('{1}') is dated but has "
						"invalid day {2}."),
					FText::AsNumber(
						Index),
					FText::FromString(
						Event.EventTag.ToString()),
					FText::AsNumber(
						Event.EventDay)));
		}
	}

#pragma endregion

#pragma region TimeRanges

	TSet<FGameplayTag> SeenRangeTags;

	for (int32 Index = 0;
		 Index < TimeRanges.Num();
		 ++Index)
	{
		const FDiurnalTimeRange& Range =
			TimeRanges[Index];

		if (!Range.RangeTag.IsValid())
		{
			AddError(
				FText::Format(
					LOCTEXT(
						"InvalidTimeRangeTag",
						"Time Ranges[{0}] has no valid range tag."),
					FText::AsNumber(
						Index)));
		}
		else if (SeenRangeTags.Contains(
					Range.RangeTag))
		{
			AddError(
				FText::Format(
					LOCTEXT(
						"DuplicateTimeRangeTag",
						"Time Ranges[{0}] duplicates the tag '{1}'."),
					FText::AsNumber(
						Index),
					FText::FromString(
						Range.RangeTag.ToString())));
		}
		else
		{
			SeenRangeTags.Add(
				Range.RangeTag);
		}

		if (!Range.StartTime.IsValid())
		{
			AddError(
				FText::Format(
					LOCTEXT(
						"InvalidTimeRangeStart",
						"Time Ranges[{0}] ('{1}') has an invalid "
						"start time."),
					FText::AsNumber(
						Index),
					FText::FromString(
						Range.RangeTag.ToString())));
		}

		if (!Range.EndTime.IsValid())
		{
			AddError(
				FText::Format(
					LOCTEXT(
						"InvalidTimeRangeEnd",
						"Time Ranges[{0}] ('{1}') has an invalid "
						"end time."),
					FText::AsNumber(
						Index),
					FText::FromString(
						Range.RangeTag.ToString())));
		}

		if (Range.StartTime.IsValid()
			&& Range.EndTime.IsValid()
			&& Range.StartTime == Range.EndTime)
		{
			AddError(
				FText::Format(
					LOCTEXT(
						"ZeroLengthTimeRange",
						"Time Ranges[{0}] ('{1}') has identical start "
						"and end times; ranges must have a non-zero "
						"duration."),
					FText::AsNumber(
						Index),
					FText::FromString(
						Range.RangeTag.ToString())));
		}
	}

#pragma endregion

	return bIsValid
		? EDataValidationResult::Valid
		: EDataValidationResult::Invalid;
}

#undef LOCTEXT_NAMESPACE

#endif
