using UnrealBuildTool;

public class GlossaryUI : ModuleRules
{
	public GlossaryUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore",
			"GlossaryCore", "CommonUI", "CommonInput", "ModelViewViewModel", "FieldNotification",
		});
	}
}
