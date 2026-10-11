#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "GlossaryAssetKit.h"

/** 06_UI 분야 데모가 쓰는 생성 에셋 (/Game/Glossary/06_UI). 없으면 비워 둔다. */
static void BuildGlossaryUIAssets()
{
}
#endif

class FGlossaryUIModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::RegisterBuilder(TEXT("06_UI"), &BuildGlossaryUIAssets);
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::UnregisterBuilder(TEXT("06_UI"));
#endif
	}
};

IMPLEMENT_MODULE(FGlossaryUIModule, GlossaryUI);
