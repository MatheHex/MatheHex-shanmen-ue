using UnrealBuildTool;

public class ShanmenCombatCore : ModuleRules
{
	public ShanmenCombatCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"GameplayTags",
			"ShanmenCore"
		});
	}
}
