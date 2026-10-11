using UnrealBuildTool;

public class GlossaryAI : ModuleRules
{
	public GlossaryAI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore",
			"GlossaryCore", "AIModule", "NavigationSystem", "GameplayTasks", "GameplayTags", "StateTreeModule", "GameplayStateTreeModule", "SmartObjectsModule", "MassEntity", "MassCore", "WorldConditions",
		});

		// 에디터 빌더(에셋 생성·BuildLevel 의 볼륨 브러시·StateTree 컴파일)에서만 쓰는 모듈
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "StateTreeEditorModule", "BSPUtils", "PropertyBindingUtils" });
		}
	}
}
