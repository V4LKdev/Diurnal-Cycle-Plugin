#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DiurnalCycleTypes.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"

#endif

#include "DiurnalCycleSettings.generated.h"

class UDiurnalSchedule;

namespace DiurnalCycle
{
	/** Returns the first duplicate non-null schedule reference and both indices. */
	DIURNALCYCLERUNTIME_API bool FindDuplicateScheduleReference(
		const TArray<TSoftObjectPtr<UDiurnalSchedule>>& Schedules,
		int32& OutFirstIndex,
		int32& OutDuplicateIndex,
		FSoftObjectPath& OutPath);
}

/**
 * Project-wide defaults used to initialize each day-night-cycle runtime.
 *
 * Clock defaults remain here. Authored schedules normally live in reusable
 * UDiurnalSchedule assets.
 */
UCLASS(
	Config = Game,
	DefaultConfig,
	meta = (
		DisplayName = "Day Night Cycle"
	))
class DIURNALCYCLERUNTIME_API UDiurnalCycleSettings final
	: public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDiurnalCycleSettings();

#pragma region Clock

	/** Real seconds required for one game hour at a time scale of one. */
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Clock",
		meta = (
			ClampMin = "0.001",
			UIMin = "0.001"
		))
	double RealSecondsPerGameHour =
		DiurnalCycle::GDefaultRealSecondsPerGameHour;

	/** Absolute date and time assigned when the runtime clock initializes. */
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Clock")
	FDiurnalDateTime StartingDateTime = FDiurnalDateTime(1, FDiurnalTimeOfDay(9));

	/**
	 * Initial clock-speed multiplier.
	 *
	 * Zero keeps the clock unpaused but prevents automatic advancement.
	 */
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Clock",
		meta = (
			ClampMin = "0.0",
			ClampMax = "4096.0",
			UIMin = "0.0",
			UIMax = "4096.0"
		))
	double DefaultTimeScale =
		DiurnalCycle::GDefaultTimeScale;

	/** Whether automatic clock advancement begins explicitly paused. */
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Clock")
	bool bStartPaused = false;

#pragma endregion

#pragma region World

	/**
	 * Whether worlds advance time automatically when they do not provide a
	 * per-world override.
	 */
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "World")
	bool bAdvanceTimeByDefault = true;

#pragma endregion

#pragma region Schedule

	/** Ordered authored schedule layers used by new runtime instances. */
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Schedule",
		meta = (AllowedClasses = "/Script/DiurnalCycleRuntime.DiurnalSchedule"))
	TArray<TSoftObjectPtr<UDiurnalSchedule>> DefaultSchedules;

#if WITH_EDITORONLY_DATA
	/** Default planning color for a Once notification event. */
	UPROPERTY(Config, EditAnywhere, Category = "Schedule|Default Colors", meta = (DisplayName = "Once Event", HideAlphaChannel))
	FLinearColor OnceEventColor = FLinearColor(0.30f, 0.45f, 0.74f, 1.0f);

	/** Default planning color for a Repeating notification event. */
	UPROPERTY(Config, EditAnywhere, Category = "Schedule|Default Colors", meta = (DisplayName = "Repeating Event", HideAlphaChannel))
	FLinearColor RepeatingEventColor = FLinearColor(0.43f, 0.34f, 0.66f, 1.0f);

	/** Default planning color for a Once blocking event. */
	UPROPERTY(Config, EditAnywhere, Category = "Schedule|Default Colors", meta = (DisplayName = "Once Gate", HideAlphaChannel))
	FLinearColor OnceGateColor = FLinearColor(0.76f, 0.38f, 0.15f, 1.0f);

	/** Default planning color for a Repeating blocking event. */
	UPROPERTY(Config, EditAnywhere, Category = "Schedule|Default Colors", meta = (DisplayName = "Repeating Gate", HideAlphaChannel))
	FLinearColor RepeatingGateColor = FLinearColor(0.68f, 0.25f, 0.25f, 1.0f);

	/** Default planning color for a Once time range. */
	UPROPERTY(Config, EditAnywhere, Category = "Schedule|Default Colors", meta = (DisplayName = "Once Range", HideAlphaChannel))
	FLinearColor OnceRangeColor = FLinearColor(0.20f, 0.49f, 0.53f, 1.0f);

	/** Default planning color for a Repeating time range. */
	UPROPERTY(Config, EditAnywhere, Category = "Schedule|Default Colors", meta = (DisplayName = "Repeating Range", HideAlphaChannel))
	FLinearColor RepeatingRangeColor = FLinearColor(0.31f, 0.57f, 0.39f, 1.0f);
#endif

#pragma endregion

#pragma region UDeveloperSettings

	virtual FName GetCategoryName() const override
	{
		return FName(TEXT("Plugins"));
	}

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(
		FDataValidationContext& Context) const override;
#endif

#pragma endregion
};

