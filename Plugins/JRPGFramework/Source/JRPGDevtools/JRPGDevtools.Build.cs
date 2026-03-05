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
            // 검사할거면 필요한 다른 모듈들 추가
        });

        // Shipping 빌드일시 최적화 관련
        if (Target.Configuration == UnrealTargetConfiguration.Shipping)
        {
            bUsePrecompiled = true;
        }
    }
}