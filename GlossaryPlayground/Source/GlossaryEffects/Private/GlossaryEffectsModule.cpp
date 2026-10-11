#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "GlossaryAssetKit.h"
#include "EffectsAssets.h"   // BuildGlossaryEffectsAssets: /Game/Glossary/05_Effects 의 머티리얼·텍스처·Niagara System
#endif

class FGlossaryEffectsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::RegisterBuilder(TEXT("05_Effects"), &BuildGlossaryEffectsAssets);
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::UnregisterBuilder(TEXT("05_Effects"));
#endif
	}
};

IMPLEMENT_MODULE(FGlossaryEffectsModule, GlossaryEffects);
