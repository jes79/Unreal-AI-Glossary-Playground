using UnrealBuildTool;

public class GlossaryEffects : ModuleRules
{
	public GlossaryEffects(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore",
			"GlossaryCore", "Niagara", "NiagaraCore", "RenderCore",
		});
	}
}
