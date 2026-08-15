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
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",

                "Slate",
                "SlateCore",

                "ToolMenus",
                "LevelEditor",
                "UnrealEd",
                "Settings",
                "DeveloperSettings",

                "DiurnalCycleRuntime"
            }
        );
    }
}