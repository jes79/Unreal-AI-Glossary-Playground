#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "GlossaryAssetKit.h"

/** 05_Effects 분야 데모가 쓰는 생성 에셋 (/Game/Glossary/05_Effects). 없으면 비워 둔다. */
static void BuildGlossaryEffectsAssets()
{
}
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
