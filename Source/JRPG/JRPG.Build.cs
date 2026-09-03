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
			"GameplayTasks",
			"NavigationSystem",
			"AIModule", 
			"UMG", 
			"Slate", 
			"SlateCore",
			"EnhancedInput",
			"Niagara"
		});

		// JRPG Framework 플러그인 : Public
		PublicDependencyModuleNames.AddRange(new string[] {
			"JRPGCore",
			"JRPGCombat"
		});
	}
}
