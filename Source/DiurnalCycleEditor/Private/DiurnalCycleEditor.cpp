#include "DiurnalCycleEditor.h"

#include "SDiurnalCycleToolbarWidget.h"
#include "DiurnalCycleEditorStyle.h"
#include "DiurnalCycleSettingsCustomization.h"
#include "DiurnalCycleSettings.h"
#include "ScheduleBrowser/SDiurnalScheduleBrowser.h"
#include "ScheduleEditor/DiurnalScheduleEditorCommands.h"

#include "Framework/Docking/TabManager.h"
#include "ISettingsModule.h"
#include "MessageLogModule.h"
#include "PropertyEditorModule.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

const FName FDiurnalCycleEditorModule::ScheduleBrowserTabName(
	TEXT("DiurnalCycleScheduleBrowser"));

void FDiurnalCycleEditorModule::StartupModule()
{
	FDiurnalCycleEditorStyle::Initialize();
	FDiurnalScheduleEditorCommands::Register();
	{
		FMessageLogInitializationOptions Options;
		Options.bShowFilters = true;
		Options.bShowPages = true;
		Options.bAllowClear = true;
		FModuleManager::LoadModuleChecked<FMessageLogModule>(TEXT("MessageLog")).RegisterLogListing(
			TEXT("DiurnalScheduleValidation"),
			NSLOCTEXT("DiurnalCycleEditor", "ValidationLog", "Day/Night Cycle Schedule Validation"),
			Options);
	}
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		ScheduleBrowserTabName,
		FOnSpawnTab::CreateRaw(
			this,
			&FDiurnalCycleEditorModule::SpawnScheduleBrowserTab))
		.SetDisplayName(
			NSLOCTEXT(
				"DiurnalCycleEditor",
				"CalendarTabTitle",
				"Day/Night Cycle Schedule Browser"))
		.SetTooltipText(
			NSLOCTEXT(
				"DiurnalCycleEditor",
				"CalendarTabTooltip",
				"Browse and edit project-default entries or any authored schedule asset."))
		.SetIcon(
			FSlateIcon(
				FDiurnalCycleEditorStyle::GetStyleSetName(),
				TEXT("DiurnalCycle.Toolbar.Schedule")))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(
			this,
			&FDiurnalCycleEditorModule::RegisterMenus));

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	PropertyEditor.RegisterCustomClassLayout(UDiurnalCycleSettings::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FDiurnalCycleSettingsCustomization::MakeInstance));
}

void FDiurnalCycleEditorModule::ShutdownModule()
{
	if (FDiurnalScheduleEditorCommands::IsRegistered()) FDiurnalScheduleEditorCommands::Unregister();
	if (FMessageLogModule* MessageLog = FModuleManager::GetModulePtr<FMessageLogModule>(TEXT("MessageLog")))
	{
		MessageLog->UnregisterLogListing(TEXT("DiurnalScheduleValidation"));
	}
	UToolMenus::UnRegisterStartupCallback(this);

	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::UnregisterOwner(this);
	}

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(
		ScheduleBrowserTabName);

	if (FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
	{
		FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor")).UnregisterCustomClassLayout(UDiurnalCycleSettings::StaticClass()->GetFName());
	}

	FDiurnalCycleEditorStyle::Shutdown();
}

void FDiurnalCycleEditorModule::OpenScheduleBrowser()
{
	FGlobalTabmanager::Get()->TryInvokeTab(ScheduleBrowserTabName);
}

void FDiurnalCycleEditorModule::OpenProjectSettings()
{
	const UDiurnalCycleSettings* Settings = GetDefault<UDiurnalCycleSettings>();
	ISettingsModule& Module = FModuleManager::LoadModuleChecked<ISettingsModule>(TEXT("Settings"));
	Module.ShowViewer(Settings->GetContainerName(), Settings->GetCategoryName(), Settings->GetSectionName());
}

void FDiurnalCycleEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	if (UToolMenu* Toolbar = UToolMenus::Get()->ExtendMenu(
		TEXT("LevelEditor.LevelEditorToolBar.PlayToolBar")))
	{
		FToolMenuSection& Section =
			Toolbar->FindOrAddSection(TEXT("DiurnalCycle"));

		Section.AddEntry(
			FToolMenuEntry::InitWidget(
				TEXT("DiurnalCycleTime"),
				SNew(SDiurnalCycleToolbarWidget),
				FText::GetEmpty(),
				true,
				false));
	}

	if (UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools")))
	{
		FToolMenuSection& Section = ToolsMenu->FindOrAddSection(TEXT("Tools"));
		Section.AddSubMenu(TEXT("DiurnalCycle"), NSLOCTEXT("DiurnalCycleEditor", "ToolsName", "Day/Night Cycle"), NSLOCTEXT("DiurnalCycleEditor", "ToolsTip", "Day/Night Cycle authoring tools."),
			FNewToolMenuDelegate::CreateLambda([](UToolMenu* Menu)
			{
				FToolMenuSection& Actions = Menu->AddSection(TEXT("DiurnalCycleActions"));
				const FSlateIcon Icon(FDiurnalCycleEditorStyle::GetStyleSetName(), TEXT("DiurnalCycle.Toolbar.Schedule"));
				Actions.AddMenuEntry(TEXT("OpenBrowser"), NSLOCTEXT("DiurnalCycleEditor", "Browser", "Open Schedule Browser"), FText(), Icon, FUIAction(FExecuteAction::CreateStatic(&FDiurnalCycleEditorModule::OpenScheduleBrowser)));
				Actions.AddMenuEntry(TEXT("Settings"), NSLOCTEXT("DiurnalCycleEditor", "Settings", "Project Settings"), FText(), FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Settings")), FUIAction(FExecuteAction::CreateStatic(&FDiurnalCycleEditorModule::OpenProjectSettings)));
			}));
	}
}

TSharedRef<SDockTab>
FDiurnalCycleEditorModule::SpawnScheduleBrowserTab(
	const FSpawnTabArgs& /* Args */)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SDiurnalScheduleBrowser)
		];
}

IMPLEMENT_MODULE(
	FDiurnalCycleEditorModule,
	DiurnalCycleEditor)
