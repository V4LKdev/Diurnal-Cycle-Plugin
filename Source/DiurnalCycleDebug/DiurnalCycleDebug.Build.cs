using UnrealBuildTool;

public class DiurnalCycleDebug : ModuleRules
{
    public DiurnalCycleDebug(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        SetupGameplayDebuggerSupport(Target);

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
                "GameplayDebugger",
                "DiurnalCycleRuntime",
                "InputCore"
            }
        );
    }
}