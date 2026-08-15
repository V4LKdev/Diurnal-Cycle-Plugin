#include "DiurnalCycleEditor.h"

#include "SDiurnalCycleToolbarWidget.h"

#include "ToolMenus.h"

void FDiurnalCycleEditorModule::StartupModule()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(
			this,
			&FDiurnalCycleEditorModule::
				RegisterMenus));
}

void FDiurnalCycleEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(
		this);

	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::UnregisterOwner(
			this);
	}
}

void FDiurnalCycleEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(
		this);

	UToolMenu* Toolbar =
		UToolMenus::Get()->ExtendMenu(
			TEXT(
				"LevelEditor.LevelEditorToolBar."
				"PlayToolBar"));

	if (!Toolbar)
	{
		return;
	}

	FToolMenuSection& Section =
		Toolbar->FindOrAddSection(
			TEXT("DiurnalCycle"));

	Section.AddEntry(
		FToolMenuEntry::InitWidget(
			TEXT("DiurnalCycleTime"),
			SNew(SDiurnalCycleToolbarWidget),
			FText::GetEmpty(),
			true,
			false));
}

IMPLEMENT_MODULE(
	FDiurnalCycleEditorModule,
	DiurnalCycleEditor)
