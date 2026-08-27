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

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Json",
			"JsonUtilities"
		});

		// Core's FPlatformMisc SHA-256 hook is not implemented on every runtime
		// platform. Use the engine-bundled OpenSSL implementation explicitly.
		AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL");
	}
}
