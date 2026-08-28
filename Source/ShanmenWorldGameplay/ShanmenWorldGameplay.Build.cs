using UnrealBuildTool;

public class ShanmenWorldGameplay : ModuleRules
{
	public ShanmenWorldGameplay(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"ShanmenCombatCore"
		});
	}
}
