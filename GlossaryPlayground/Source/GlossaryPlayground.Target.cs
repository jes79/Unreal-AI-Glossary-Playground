using UnrealBuildTool;
using System.Collections.Generic;

public class GlossaryPlaygroundTarget : TargetRules
{
	public GlossaryPlaygroundTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.AddRange(GlossaryModules.Runtime);
	}
}

// 분야별 런타임 모듈 목록. 새 분야를 추가하면 여기와 .uproject 둘 다 고친다.
public static class GlossaryModules
{
	public static readonly string[] Runtime = new string[]
	{
		"GlossaryCore",
		"GlossaryMovement", "GlossaryPhysics", "GlossaryAnimation", "GlossaryMaterial", "GlossaryEffects",
		"GlossaryUI", "GlossaryAI", "GlossarySystems", "GlossaryAudio", "GlossaryRendering", "GlossaryPatterns",
	};
}
