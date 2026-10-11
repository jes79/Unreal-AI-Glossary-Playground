#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "GlossaryAssetKit.h"

/** 07_AI 분야 데모가 쓰는 생성 에셋 (/Game/Glossary/07_AI). 없으면 비워 둔다. */
static void BuildGlossaryAIAssets()
{
}
#endif

class FGlossaryAIModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::RegisterBuilder(TEXT("07_AI"), &BuildGlossaryAIAssets);
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::UnregisterBuilder(TEXT("07_AI"));
#endif
	}
};

IMPLEMENT_MODULE(FGlossaryAIModule, GlossaryAI);
