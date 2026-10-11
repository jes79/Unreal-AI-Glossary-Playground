using UnrealBuildTool;

public class GlossaryEditor : ModuleRules
{
	public GlossaryEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "GlossaryCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd", "BSPUtils", "ToolMenus", "Slate", "SlateCore", "NavigationSystem", "AssetRegistry", "LevelEditor",
		});
	}
}
