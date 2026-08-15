#include "Modules/ModuleManager.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebugger.h"
#include "GameplayDebuggerCategory_DiurnalCycle.h"
#endif

namespace
{
	const FName DiurnalCycleCategoryName(
		TEXT("DiurnalCycle"));
}

class FDiurnalCycleDebugModule final
	: public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_GAMEPLAY_DEBUGGER
		IGameplayDebugger& GameplayDebugger =
			IGameplayDebugger::Get();

		GameplayDebugger.RegisterCategory(
			DiurnalCycleCategoryName,
			IGameplayDebugger::FOnGetCategory::CreateStatic(
				&FGameplayDebuggerCategory_DiurnalCycle::
					MakeInstance),
			EGameplayDebuggerCategoryState::
				EnabledInGameAndSimulate,
			INDEX_NONE);

		GameplayDebugger.NotifyCategoriesChanged();
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_GAMEPLAY_DEBUGGER
		if (IGameplayDebugger::IsAvailable())
		{
			IGameplayDebugger& GameplayDebugger =
				IGameplayDebugger::Get();

			GameplayDebugger.UnregisterCategory(
				DiurnalCycleCategoryName);

			GameplayDebugger.NotifyCategoriesChanged();
		}
#endif
	}
};

IMPLEMENT_MODULE(
	FDiurnalCycleDebugModule,
	DiurnalCycleDebug)