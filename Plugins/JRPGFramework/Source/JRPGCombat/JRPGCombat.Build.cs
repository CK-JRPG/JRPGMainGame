using UnrealBuildTool;

public class JRPGCombat : ModuleRules
{
    public JRPGCombat(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "JRPGCore" 
        });

        // 다른 JRPG 모듈은 절대 여기에 넣지말것.(노션 모듈 의존성 참고)
        PrivateDependencyModuleNames.AddRange(new string[] { });
    }
}