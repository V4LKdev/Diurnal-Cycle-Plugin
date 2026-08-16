#include "Modules/ModuleManager.h"
#include "DiurnalSchedule.h"
#include "Engine/AssetManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"

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

				TArray<FString> ScheduleRoots;
				FString ProjectContentRoot;
				if (FPackageName::TryConvertFilenameToLongPackageName(
					FPaths::ProjectContentDir(), ProjectContentRoot))
				{
					ScheduleRoots.Add(ProjectContentRoot);
				}
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
