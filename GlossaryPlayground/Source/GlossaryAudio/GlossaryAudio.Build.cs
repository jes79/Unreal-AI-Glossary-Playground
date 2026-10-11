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
			// 0905 Submix 로우패스: USubmixEffectFilterPreset (Synthesis 플러그인, 기본 켜짐)
			"Synthesis",
		});

		// 에디터 전용: 에셋 빌더(Sound Cue 그래프, MetaSound 에셋)와 0906 Audio Volume 브러시
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd", "BSPUtils", "AudioEditor", "MetasoundEditor", "EditorScriptingUtilities", "AssetTools",
			});
		}
	}
}
