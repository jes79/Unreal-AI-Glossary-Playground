using UnrealBuildTool;

public class GlossaryAnimation : ModuleRules
{
	public GlossaryAnimation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore",
			"GlossaryCore", "AnimGraphRuntime", "AnimationCore", "ControlRig", "IKRig", "PoseSearch",
			"LevelSequence", "MovieScene", "MovieSceneTracks", "CinematicCamera",
		});

		if (Target.bBuildEditor)
		{
			// 에셋 빌더(몽타주·블렌드 스페이스·레벨 시퀀스 저장)용
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
		}
	}
}
