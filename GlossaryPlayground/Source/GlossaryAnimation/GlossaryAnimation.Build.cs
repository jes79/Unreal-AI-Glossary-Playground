using UnrealBuildTool;

public class GlossaryAnimation : ModuleRules
{
	public GlossaryAnimation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore",
			"GlossaryCore", "AnimGraphRuntime", "ControlRig", "IKRig", "PoseSearch", "LevelSequence", "MovieScene", "MovieSceneTracks",
		});
	}
}
