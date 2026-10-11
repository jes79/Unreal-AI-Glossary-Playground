using UnrealBuildTool;

public class GlossaryPhysics : ModuleRules
{
	public GlossaryPhysics(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		// 데모마다 파일 안 익명 namespace 상수(WallX 등)를 쓰므로 Unity 빌드로 합쳐지면 이름이 겹친다
		bUseUnity = false;

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
