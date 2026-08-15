#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "DiurnalCycleWorldSettings.h"

#include "DiurnalCycleWorldSubsystem.generated.h"

/**
 * Resolves the effective day-night-cycle policy for one world.
 *
 * The subsystem does not own clock state. It only determines whether its world
 * currently permits automatic advancement of the game-instance clock.
 */
UCLASS()
class DIURNALCYCLERUNTIME_API UDiurnalCycleWorldSubsystem final
	: public UWorldSubsystem
{
	GENERATED_BODY()

public:
#pragma region PolicyQueries

	/** Returns the policy stored on this world's AWorldSettings. */
	EDiurnalCycleWorldTimePolicy GetConfiguredTimePolicy() const;

	/**
	 * Returns the currently effective policy.
	 *
	 * Resolution order:
	 * Runtime Override -> World Settings -> Project Default.
	 */
	EDiurnalCycleWorldTimePolicy GetEffectiveTimePolicy() const;

	/** Returns whether this world currently permits automatic advancement. */
	bool ShouldAdvanceTime() const
	{
		return GetEffectiveTimePolicy()
			== EDiurnalCycleWorldTimePolicy::Advance;
	}

	/** Returns whether this world currently has a runtime policy override. */
	bool HasRuntimeTimePolicyOverride() const
	{
		return RuntimeTimePolicyOverride.IsSet();
	}

#pragma endregion

#pragma region RuntimeOverride

	/**
	 * Overrides the advancement policy for this world at runtime.
	 *
	 * Passing UseProjectDefault clears the runtime override.
	 */
	void SetRuntimeTimePolicy(
		EDiurnalCycleWorldTimePolicy Policy);

	/** Clears the runtime override and restores configured policy resolution. */
	void ClearRuntimeTimePolicy();

#pragma endregion

private:
	/** Optional transient override affecting this world only. */
	TOptional<EDiurnalCycleWorldTimePolicy>
		RuntimeTimePolicyOverride;
};