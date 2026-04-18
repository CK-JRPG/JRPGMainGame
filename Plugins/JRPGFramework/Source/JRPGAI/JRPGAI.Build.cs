using UnrealBuildTool;

public class JRPGAI : ModuleRules
{
    public JRPGAI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "JRPGCore" 
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "JRPGCombat", 
            "AIModule",   
            "GameplayTasks"
        });
    }
}