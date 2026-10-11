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
	}
}
