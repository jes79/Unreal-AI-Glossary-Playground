using UnrealBuildTool;

public class GlossaryAudio : ModuleRules
{
	public GlossaryAudio(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore",
			"GlossaryCore", "AudioMixer", "MetasoundEngine", "MetasoundFrontend", "AudioExtensions",
		});
	}
}
