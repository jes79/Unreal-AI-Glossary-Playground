using UnrealBuildTool;

public class GlossaryAI : ModuleRules
{
	public GlossaryAI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore",
			"GlossaryCore", "AIModule", "NavigationSystem", "GameplayTasks", "GameplayTags", "StateTreeModule", "GameplayStateTreeModule", "SmartObjectsModule", "MassEntity",
		});
	}
}
