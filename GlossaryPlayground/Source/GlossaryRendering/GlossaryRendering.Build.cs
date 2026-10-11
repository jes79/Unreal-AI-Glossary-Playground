using UnrealBuildTool;

public class GlossaryRendering : ModuleRules
{
	public GlossaryRendering(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore",
			"GlossaryCore", "RenderCore", "RHI", "PCG",
		});

		if (Target.bBuildEditor)
		{
			// 에셋 빌더(메시·머티리얼·PCG 그래프 생성)와 BuildLevel(볼륨 브러시)용
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd", "BSPUtils", "MeshDescription", "StaticMeshDescription",
			});
		}
	}
}
