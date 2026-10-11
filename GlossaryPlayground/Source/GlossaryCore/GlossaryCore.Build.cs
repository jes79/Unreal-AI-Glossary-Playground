using UnrealBuildTool;

public class GlossaryCore : ModuleRules
{
	public GlossaryCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"UMG", "Slate", "SlateCore", "AIModule", "NavigationSystem",
		});

		// 에디터 빌드에서만: 생성 에셋(머티리얼·사운드·레벨)을 코드로 만드는 GlossaryAssetKit
		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd", "MaterialEditor", "AssetTools", "AssetRegistry", "AudioEditor",
			});
		}
	}
}
