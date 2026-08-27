using UnrealBuildTool;

public class ShanmenItems : ModuleRules
{
	public ShanmenItems(ReadOnlyTargetRules Target) : base(Target)
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
