using UnrealBuildTool;

public class JRPGProgression : ModuleRules
{
    public JRPGProgression(ReadOnlyTargetRules Target) : base(Target)
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
            "JRPGEconomy" //리워드 시스템 떄문에 추가
        });
    }
}