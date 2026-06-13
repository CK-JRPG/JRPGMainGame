using UnrealBuildTool;

public class JRPGGameFlow : ModuleRules
{
    public JRPGGameFlow(ReadOnlyTargetRules Target) : base(Target)
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
            "JRPGEconomy",
            "JRPGExploration",
            "JRPGProgression",
        });
    }
}