using UnrealBuildTool;

public class DiurnalCycleEditor : ModuleRules
{
	public DiurnalCycleEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"InputCore",

				"Slate",
				"SlateCore",
				"AppFramework",
				"SequencerWidgets",

				"ToolMenus",
				"LevelEditor",
				"UnrealEd",
				"AssetDefinition",
				"AssetTools",
				"AssetRegistry",
				"Projects",
				"Settings",
				"PropertyEditor",
				"DeveloperSettings",
				"MessageLog",

				"DiurnalCycleRuntime",
			});
	}
}
