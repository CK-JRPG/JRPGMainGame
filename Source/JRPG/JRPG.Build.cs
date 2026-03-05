// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class JRPG : ModuleRules
{
	public JRPG(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		// 엔진 모듈
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"GameplayTags", 
			"AIModule", 
			"UMG", 
			"Slate", 
			"SlateCore",
			"EnhancedInput" 
		});

		// JRPG Framework 플러그인 : Public
		PublicDependencyModuleNames.AddRange(new string[] {
			"JRPGCore",
			"JRPGGameFlow"
		});

		// JRPG Framework 플러그인 : Private
		PrivateDependencyModuleNames.AddRange(new string[] {
			"JRPGCombat",
			"JRPGAI",
			"JRPGEconomy",
			"JRPGExploration",
			"JRPGProgression",
			"JRPGUI", "EnhancedInput"
		});

		// Devtools 전용모듈 -> Shipping 빌드에서는 제외
		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			PrivateDependencyModuleNames.Add("JRPGDevtools");
		}
	}
}