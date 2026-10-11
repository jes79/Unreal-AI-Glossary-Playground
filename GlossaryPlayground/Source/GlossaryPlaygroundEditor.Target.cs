using UnrealBuildTool;
using System.Collections.Generic;

public class GlossaryPlaygroundEditorTarget : TargetRules
{
	public GlossaryPlaygroundEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.AddRange(GlossaryModules.Runtime);
		ExtraModuleNames.Add("GlossaryEditor");
	}
}
