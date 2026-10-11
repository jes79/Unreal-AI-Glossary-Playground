using UnrealBuildTool;

public class GlossaryPatterns : ModuleRules
{
	public GlossaryPatterns(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore",
			"GlossaryCore", "AIModule", "NetCore", "DeveloperSettings",
		});
	}
}
