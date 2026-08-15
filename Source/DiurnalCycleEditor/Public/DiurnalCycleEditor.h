#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Provides editor-only integration for the Day Night Cycle plugin.
 *
 * Registers the compact live clock and current-map policy controls in the
 * Level Editor toolbar.
 */
class FDiurnalCycleEditorModule final
	: public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Registers the Day Night Cycle Level Editor toolbar integration. */
	void RegisterMenus();
};