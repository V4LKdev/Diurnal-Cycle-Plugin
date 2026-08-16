
#include "DiurnalCycleSettings.h"
#include "DiurnalSchedule.h"

bool DiurnalCycle::FindDuplicateScheduleReference(
	const TArray<TSoftObjectPtr<UDiurnalSchedule>>& Schedules,
	int32& OutFirstIndex,
	int32& OutDuplicateIndex,
	FSoftObjectPath& OutPath)
{
	OutFirstIndex = INDEX_NONE;
	OutDuplicateIndex = INDEX_NONE;
	OutPath.Reset();

	TMap<FSoftObjectPath, int32> FirstIndices;
	for (int32 Index = 0; Index < Schedules.Num(); ++Index)
	{
		const FSoftObjectPath Path = Schedules[Index].ToSoftObjectPath();
		if (Path.IsNull()) continue;
		if (const int32* FirstIndex = FirstIndices.Find(Path))
		{
			OutFirstIndex = *FirstIndex;
			OutDuplicateIndex = Index;
			OutPath = Path;
			return true;
		}
		FirstIndices.Add(Path, Index);
	}
	return false;
}

UDiurnalCycleSettings::UDiurnalCycleSettings() = default;

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
	const auto AddWarning = [&Context](const FText& Warning)
	{
		Context.AddWarning(Warning);
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

#pragma region ScheduleValidation

	if (!DefaultSchedules.IsEmpty())
	{
		int32 FirstDuplicateIndex = INDEX_NONE;
		int32 DuplicateIndex = INDEX_NONE;
		FSoftObjectPath DuplicatePath;
		if (DiurnalCycle::FindDuplicateScheduleReference(DefaultSchedules, FirstDuplicateIndex, DuplicateIndex, DuplicatePath))
		{
			AddError(FText::Format(
				LOCTEXT("DuplicateDefaultSchedule", "Default Schedules[{0}] duplicates Default Schedules[{1}] ('{2}'). Remove one reference; schedule composition is atomic and will not apply duplicate layers."),
				FText::AsNumber(DuplicateIndex), FText::AsNumber(FirstDuplicateIndex), FText::FromString(DuplicatePath.ToString())));
		}
		for (int32 Index = 0; Index < DefaultSchedules.Num(); ++Index)
		{
			const TSoftObjectPtr<UDiurnalSchedule>& Reference = DefaultSchedules[Index];
			const FSoftObjectPath Path = Reference.ToSoftObjectPath();
			if (Path.IsNull())
			{
				AddError(FText::Format(LOCTEXT("EmptyDefaultSchedule", "Default Schedules[{0}] is empty."), FText::AsNumber(Index)));
				continue;
			}

			UDiurnalSchedule* Schedule = Reference.LoadSynchronous();
			if (!IsValid(Schedule))
			{
				AddError(FText::Format(LOCTEXT("UnloadedDefaultSchedule", "Default Schedules[{0}] ('{1}') could not be loaded."), FText::AsNumber(Index), FText::FromString(Path.ToString())));
				continue;
			}

			TArray<FDiurnalScheduleValidationIssue> Issues;
			Schedule->GetValidationIssues(Issues);
			for (const FDiurnalScheduleValidationIssue& Issue : Issues)
			{
				const FText ContextualMessage = FText::Format(
					LOCTEXT("DefaultScheduleIssue", "Default schedule '{0}': {1}"),
					FText::FromString(Schedule->GetName()),
					Issue.Message);
				if (Issue.Severity == EDiurnalScheduleIssueSeverity::Error)
				{
					AddError(ContextualMessage);
				}
				else
				{
					AddWarning(ContextualMessage);
				}
			}
		}
	}

#pragma endregion

	return bIsValid
		? EDataValidationResult::Valid
		: EDataValidationResult::Invalid;
}

#undef LOCTEXT_NAMESPACE

#endif
