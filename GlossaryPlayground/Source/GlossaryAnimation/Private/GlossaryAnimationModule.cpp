#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "GlossaryAssetKit.h"

/** 03_Animation 분야 데모가 쓰는 생성 에셋 (/Game/Glossary/03_Animation). 없으면 비워 둔다. */
static void BuildGlossaryAnimationAssets()
{
}
#endif

class FGlossaryAnimationModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::RegisterBuilder(TEXT("03_Animation"), &BuildGlossaryAnimationAssets);
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::UnregisterBuilder(TEXT("03_Animation"));
#endif
	}
};

IMPLEMENT_MODULE(FGlossaryAnimationModule, GlossaryAnimation);
