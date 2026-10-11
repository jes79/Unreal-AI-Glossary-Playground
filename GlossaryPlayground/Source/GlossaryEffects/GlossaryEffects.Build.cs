using UnrealBuildTool;

public class GlossaryEffects : ModuleRules
{
	public GlossaryEffects(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore",
			"GlossaryCore", "Niagara", "NiagaraCore", "RenderCore", "PhysicsCore",
		});

		// Niagara 시스템을 코드로 만드는 에셋 빌더(05_Effects)는 에디터에서만 돈다
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd", "BSPUtils", "NiagaraEditor", "NiagaraShader", "AssetRegistry", "AssetTools",
				"DeveloperSettings", "MovieScene", "ContentBrowser",
			});
		}
	}
}
