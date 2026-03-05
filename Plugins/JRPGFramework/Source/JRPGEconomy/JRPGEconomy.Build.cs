using UnrealBuildTool;

public class JRPGEconomy : ModuleRules
{
    public JRPGEconomy(ReadOnlyTargetRules Target) : base(Target)
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
            // 다른 모듈 의존 금지임.
        });
    }
}