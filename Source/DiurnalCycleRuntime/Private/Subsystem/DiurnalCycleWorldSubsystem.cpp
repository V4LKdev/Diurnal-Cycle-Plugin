#include "Subsystem/DiurnalCycleWorldSubsystem.h"

#include "DiurnalCycleSettings.h"

#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"

EDiurnalCycleWorldTimePolicy
UDiurnalCycleWorldSubsystem::GetConfiguredTimePolicy() const
{
	UWorld* World =
		GetWorld();

	if (!IsValid(World))
	{
		return EDiurnalCycleWorldTimePolicy::UseProjectDefault;
	}

	AWorldSettings* WorldSettings =
		World->GetWorldSettings();

	if (!IsValid(WorldSettings))
	{
		return EDiurnalCycleWorldTimePolicy::UseProjectDefault;
	}

	const UDiurnalCycleWorldSettings* DiurnalSettings =
		Cast<UDiurnalCycleWorldSettings>(
			WorldSettings->GetAssetUserDataOfClass(
				UDiurnalCycleWorldSettings::StaticClass()));

	return DiurnalSettings
		? DiurnalSettings->TimeAdvancementPolicy
		: EDiurnalCycleWorldTimePolicy::UseProjectDefault;
}

EDiurnalCycleWorldTimePolicy
UDiurnalCycleWorldSubsystem::GetEffectiveTimePolicy() const
{
	if (RuntimeTimePolicyOverride.IsSet())
	{
		return RuntimeTimePolicyOverride.GetValue();
	}

	const EDiurnalCycleWorldTimePolicy ConfiguredPolicy =
		GetConfiguredTimePolicy();

	if (ConfiguredPolicy
		!= EDiurnalCycleWorldTimePolicy::UseProjectDefault)
	{
		return ConfiguredPolicy;
	}

	const UDiurnalCycleSettings* Settings =
		GetDefault<UDiurnalCycleSettings>();

	check(Settings);

	return Settings->bAdvanceTimeByDefault
		? EDiurnalCycleWorldTimePolicy::Advance
		: EDiurnalCycleWorldTimePolicy::Freeze;
}

void UDiurnalCycleWorldSubsystem::SetRuntimeTimePolicy(
	const EDiurnalCycleWorldTimePolicy Policy)
{
	if (Policy
		== EDiurnalCycleWorldTimePolicy::UseProjectDefault)
	{
		ClearRuntimeTimePolicy();
		return;
	}

	RuntimeTimePolicyOverride =
		Policy;
}

void UDiurnalCycleWorldSubsystem::ClearRuntimeTimePolicy()
{
	RuntimeTimePolicyOverride.Reset();
}