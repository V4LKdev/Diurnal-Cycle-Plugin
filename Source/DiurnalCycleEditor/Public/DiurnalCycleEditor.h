#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Provides editor-only integration for the Day Night Cycle plugin.
 *
 * Registers the compact live clock and current-map policy controls in the
 * Level Editor toolbar.
 */
class DIURNALCYCLEEDITOR_API FDiurnalCycleEditorModule final
	: public IModuleInterface
{
public:
	static const FName ScheduleBrowserTabName;

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Opens or focuses the read-only merged Schedule Browser. */
	static void OpenScheduleBrowser();
	static void OpenProjectSettings();

private:
	/** Registers the Day Night Cycle Level Editor toolbar integration. */
	void RegisterMenus();

	TSharedRef<class SDockTab> SpawnScheduleBrowserTab(
		const class FSpawnTabArgs& Args);
};
