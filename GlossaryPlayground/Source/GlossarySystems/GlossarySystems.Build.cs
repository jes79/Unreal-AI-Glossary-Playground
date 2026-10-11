using UnrealBuildTool;

public class GlossarySystems : ModuleRules
{
	public GlossarySystems(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore",
			"GlossaryCore", "GameplayAbilities", "GameplayTags", "GameplayTasks", "AIModule",
		});

		// 에셋 빌더(DataTable CSV 가져오기, 0815 서브레벨 맵 생성)는 에디터에서만
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
		}
	}
}
