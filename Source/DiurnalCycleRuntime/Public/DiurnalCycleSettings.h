#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DiurnalCycleTypes.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"

#endif

#include "DiurnalCycleSettings.generated.h"

/**
 * Project-wide defaults used to initialize each day-night-cycle runtime.
 *
 * Mutable schedules are copied into the runtime subsystem during
 * initialization and may then diverge from these configured defaults.
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

#pragma region Events

	/**
	 * Initial daily and dated events copied into the runtime schedule.
	 *
	 * Event tags must be valid and unique. Runtime systems may subsequently add
	 * or remove events independently of these configured defaults.
	 */
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Events")
	TArray<FDiurnalTimeEvent> TimeEvents;

#pragma endregion

#pragma region TimeRanges

	/**
	 * Initial recurring ranges copied into the runtime schedule.
	 *
	 * Range tags must be valid and unique. Ranges may overlap and may wrap
	 * across midnight. Runtime systems may subsequently add or remove ranges.
	 */
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Time Ranges")
	TArray<FDiurnalTimeRange> TimeRanges;

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

