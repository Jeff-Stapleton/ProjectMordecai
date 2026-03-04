// Copyright Project Mordecai. All Rights Reserved.

using UnrealBuildTool;

public class MordecaiCoreRuntime : ModuleRules
{
	public MordecaiCoreRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[] { });
		PrivateIncludePaths.AddRange(new string[] { });

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"LyraGame",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"UMG",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"EnhancedInput",
			"NetCore",
		});

		DynamicallyLoadedModuleNames.AddRange(new string[] { });
	}
}
