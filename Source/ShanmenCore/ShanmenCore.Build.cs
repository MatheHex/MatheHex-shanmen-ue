using UnrealBuildTool;

public class ShanmenCore : ModuleRules
{
	public ShanmenCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject"
		});
	}
}
