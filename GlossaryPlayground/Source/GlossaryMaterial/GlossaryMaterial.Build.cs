using UnrealBuildTool;

public class GlossaryMaterial : ModuleRules
{
	public GlossaryMaterial(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore",
			"GlossaryCore", "RenderCore",
			// 0412 깃발·0417 Vertex Color 바닥: 실행 중 메시(정점 색 포함)를 만든다
			"ProceduralMeshComponent",
		});

		if (Target.bBuildEditor)
		{
			// 에셋 빌더: 머티리얼 함수 그래프 편집(UMaterialEditingLibrary)
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "MaterialEditor" });
		}
	}
}
