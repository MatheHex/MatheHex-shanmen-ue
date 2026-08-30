using UnrealBuildTool;

public class ShanmenCombatRuntime : ModuleRules
{
	public ShanmenCombatRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		// This module deliberately repeats file-local helper names. A clean unity
		// rebuild would merge those anonymous namespaces into one translation unit.
		bUseUnity = false;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"ShanmenCombatCore",
			"ShanmenWorldGameplay"
		});

		PrivateDependencyModuleNames.Add("ShanmenCore");
	}
}
