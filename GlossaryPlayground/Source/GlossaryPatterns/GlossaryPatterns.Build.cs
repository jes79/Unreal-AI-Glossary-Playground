using UnrealBuildTool;

public class GlossaryPatterns : ModuleRules
{
	public GlossaryPatterns(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore",
			"GlossaryCore", "AIModule", "NetCore", "DeveloperSettings", "NavigationSystem",
			// 1115: Gameplay Debugger 카테고리 확장. Shipping 에서는 WITH_GAMEPLAY_DEBUGGER=0 으로 정의돼 코드가 빠진다.
			"GameplayDebugger",
		});

		// 1112: Live Coding 상태 표시. 인터페이스(순수 가상)만 쓰므로 헤더 경로만 있으면 된다(링크 의존 없음).
		if (Target.bWithLiveCoding)
		{
			PrivateIncludePathModuleNames.Add("LiveCoding");
		}

		// 1110: 에디터 빌드에서만 들어가는 에디터 도구(ToolMenus 메뉴 + Slate 탭 + UEditorActorSubsystem).
		// 런타임 모듈이지만 코드 전체가 #if WITH_EDITOR 안에 있어 패키지 게임에는 포함되지 않는다.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd", "ToolMenus", "EditorSubsystem",
			});
		}
	}
}
