using UnrealBuildTool;

public class GlossaryPhysics : ModuleRules
{
	public GlossaryPhysics(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore",
			"GlossaryCore", "PhysicsCore", "Chaos", "ChaosVehicles", "GeometryCollectionEngine", "FieldSystemEngine",
		});

		// 0211: 에디터 빌더에서 Geometry Collection 에셋을 만들고 Voronoi 로 자른다 (Fracture 모드가 내부에서 쓰는 모듈)
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd", "PlanarCut", "Voronoi", "GeometryCore", "MeshDescription", "StaticMeshDescription",
			});
		}
	}
}
