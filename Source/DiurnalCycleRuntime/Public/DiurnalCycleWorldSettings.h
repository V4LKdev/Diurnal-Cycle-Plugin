#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"

#include "DiurnalCycleWorldSettings.generated.h"

/**
 * Determines whether a world permits automatic day-night-cycle advancement.
 */
UENUM(BlueprintType)
enum class EDiurnalCycleWorldTimePolicy : uint8
{
	UseProjectDefault UMETA(DisplayName = "Use Project Default"),
	Advance UMETA(DisplayName = "Advance"),
	Freeze UMETA(DisplayName = "Freeze")
};

/**
 * Per-world day-night-cycle configuration stored on AWorldSettings.
 *
 * This allows individual maps to override the project-wide automatic
 * advancement policy without requiring a custom AWorldSettings subclass.
 */
UCLASS(
	BlueprintType,
	EditInlineNew,
	DisplayName = "Day Night Cycle World Settings")
class DIURNALCYCLERUNTIME_API UDiurnalCycleWorldSettings final
	: public UAssetUserData
{
	GENERATED_BODY()

public:
	/** Automatic time-advancement policy for this world. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Day Night Cycle")
	EDiurnalCycleWorldTimePolicy TimeAdvancementPolicy =
		EDiurnalCycleWorldTimePolicy::UseProjectDefault;
};