// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modules/ModuleManager.h"
#include "DiurnalSchedule.h"
#include "Engine/AssetManager.h"
#include "Interfaces/IPluginManager.h"
#include "NativeGameplayTags.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace DiurnalCycle::TimeEvent
{
	DIURNALCYCLERUNTIME_API FNativeGameplayTag RepeatingExample(
		UE_PLUGIN_NAME, UE_MODULE_NAME, TEXT("DiurnalCycle.Test.TimeEvent.RepeatingExample"), TEXT("Developer test fixture"), ENativeGameplayTagToken::PRIVATE_USE_MACRO_INSTEAD);
	DIURNALCYCLERUNTIME_API FNativeGameplayTag OnceExample(
		UE_PLUGIN_NAME, UE_MODULE_NAME, TEXT("DiurnalCycle.Test.TimeEvent.OnceExample"), TEXT("Developer test fixture"), ENativeGameplayTagToken::PRIVATE_USE_MACRO_INSTEAD);
}

namespace DiurnalCycle::TimeRange
{
	DIURNALCYCLERUNTIME_API FNativeGameplayTag DayTime(
		UE_PLUGIN_NAME, UE_MODULE_NAME, TEXT("DiurnalCycle.Test.TimeRange.DayTime"), TEXT("Developer test fixture"), ENativeGameplayTagToken::PRIVATE_USE_MACRO_INSTEAD);
	DIURNALCYCLERUNTIME_API FNativeGameplayTag NightTime(
		UE_PLUGIN_NAME, UE_MODULE_NAME, TEXT("DiurnalCycle.Test.TimeRange.NightTime"), TEXT("Developer test fixture"), ENativeGameplayTagToken::PRIVATE_USE_MACRO_INSTEAD);
}
#endif

class FDiurnalCycleRuntimeModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UAssetManager::CallOrRegister_OnAssetManagerCreated(
			FSimpleMulticastDelegate::FDelegate::CreateLambda([]
			{
				static bool bRegistered = false;
				if (bRegistered) return;
				bRegistered = true;

				UAssetManager& AssetManager = UAssetManager::Get();
				const FPrimaryAssetType ScheduleType(TEXT("DiurnalSchedule"));
				FPrimaryAssetTypeInfo ExistingTypeInfo;
				if (AssetManager.GetPrimaryAssetTypeInfo(ScheduleType, ExistingTypeInfo))
				{
					return;
				}

				TArray<FString> ScheduleRoots{TEXT("/Game/")};
				for (const TSharedRef<IPlugin>& Plugin : IPluginManager::Get().GetEnabledPluginsWithContent())
				{
					if (Plugin->GetLoadedFrom() == EPluginLoadedFrom::Project && Plugin->IsMounted())
					{
						ScheduleRoots.AddUnique(Plugin->GetMountedAssetPath());
					}
				}
				AssetManager.ScanPathsForPrimaryAssets(
					ScheduleType,
					ScheduleRoots,
					UDiurnalSchedule::StaticClass(),
					false,
					false,
					false);
				FPrimaryAssetRules Rules;
				Rules.CookRule = EPrimaryAssetCookRule::AlwaysCook;
				AssetManager.SetPrimaryAssetTypeRules(ScheduleType, Rules);
			}));
	}
};

IMPLEMENT_MODULE(FDiurnalCycleRuntimeModule, DiurnalCycleRuntime)
