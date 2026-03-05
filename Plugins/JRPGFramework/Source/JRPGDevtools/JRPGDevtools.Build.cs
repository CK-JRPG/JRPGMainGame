using UnrealBuildTool;

public class JRPGDevtools : ModuleRules
{
    public JRPGDevtools(ReadOnlyTargetRules Target) : base(Target)
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
            // 필요 모듈 넣을 것.
        });

        // Shipping 일때는 해당 모듈 컴파일 제외.
        if (Target.Configuration == UnrealTargetConfiguration.Shipping)
        {
            bUsePrecompiled = true;
        }
    }
}