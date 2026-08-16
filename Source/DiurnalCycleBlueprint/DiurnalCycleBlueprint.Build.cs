using UnrealBuildTool;

public class DiurnalCycleBlueprint : ModuleRules
{
	public DiurnalCycleBlueprint(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"DiurnalCycleRuntime"
			});
	}
}
