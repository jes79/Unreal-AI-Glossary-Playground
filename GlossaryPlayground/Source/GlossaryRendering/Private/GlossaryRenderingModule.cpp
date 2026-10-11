#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "GlossaryAssetKit.h"

/** 10_Rendering 분야 데모가 쓰는 생성 에셋 (/Game/Glossary/10_Rendering). 없으면 비워 둔다. */
static void BuildGlossaryRenderingAssets()
{
}
#endif

class FGlossaryRenderingModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::RegisterBuilder(TEXT("10_Rendering"), &BuildGlossaryRenderingAssets);
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::UnregisterBuilder(TEXT("10_Rendering"));
#endif
	}
};

IMPLEMENT_MODULE(FGlossaryRenderingModule, GlossaryRendering);
