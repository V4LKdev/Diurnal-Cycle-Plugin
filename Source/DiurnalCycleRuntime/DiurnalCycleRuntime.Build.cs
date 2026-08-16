// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DiurnalCycleRuntime : ModuleRules
{
	public DiurnalCycleRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"DeveloperSettings"
			}
			);

		PrivateDependencyModuleNames.Add("Projects");

	}
}
