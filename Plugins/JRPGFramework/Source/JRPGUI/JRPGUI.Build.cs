using UnrealBuildTool;

public class JRPGUI : ModuleRules
{
    public JRPGUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "JRPGCore",
            "UMG"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore",
            "JRPGCombat",
            "JRPGEconomy",
            "JRPGProgression",
            "JRPGExploration",
            "JRPGGameFlow"
        });
    }
}