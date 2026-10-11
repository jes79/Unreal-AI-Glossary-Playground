using UnrealBuildTool;

public class GlossaryMovement : ModuleRules
{
	public GlossaryMovement(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore",
			"GlossaryCore", "AIModule", "NavigationSystem",
			// 0112 Camera Shake: UPerlinNoiseCameraShakePattern · UWaveOscillatorCameraShakePattern (EngineCameras 플러그인, 기본 켜짐)
			"EngineCameras",
		});

		// 0116: 레벨 빌더(에디터)에서 Physics Volume(Water) 브러시를 만들 때만 필요
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "BSPUtils" });
		}
	}
}
