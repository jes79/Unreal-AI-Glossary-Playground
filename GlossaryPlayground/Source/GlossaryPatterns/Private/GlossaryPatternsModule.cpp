#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "GlossaryAssetKit.h"

/** 11_Patterns 분야 데모가 쓰는 생성 에셋 (/Game/Glossary/11_Patterns). 없으면 비워 둔다. */
static void BuildGlossaryPatternsAssets()
{
}
#endif

class FGlossaryPatternsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::RegisterBuilder(TEXT("11_Patterns"), &BuildGlossaryPatternsAssets);
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::UnregisterBuilder(TEXT("11_Patterns"));
#endif
	}
};

IMPLEMENT_MODULE(FGlossaryPatternsModule, GlossaryPatterns);
