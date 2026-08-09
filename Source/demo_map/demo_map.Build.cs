// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class demo_map : ModuleRules
{
	// P2 Reward Source Projection remains part of this single gameplay module.
	public demo_map(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		// Automation sources use file-local helpers with intentionally repeated names.
		bUseUnity = false;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "Json" });

		PublicIncludePaths.AddRange(new string[] {
			"demo_map",
			"demo_map/Variant_Strategy",
			"demo_map/Variant_Strategy/UI",
			"demo_map/Variant_TwinStick",
			"demo_map/Variant_TwinStick/AI",
			"demo_map/Variant_TwinStick/Gameplay",
			"demo_map/Variant_TwinStick/UI"
		});
		// Native automation sources include schema, item-use, and Reward Generation contract suites.

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
