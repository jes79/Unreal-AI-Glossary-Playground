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
	}
}
