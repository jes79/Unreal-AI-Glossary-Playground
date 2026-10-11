#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "GlossaryAssetKit.h"

/** 01_Movement 분야 데모가 쓰는 생성 에셋 (/Game/Glossary/01_Movement). 없으면 비워 둔다. */
static void BuildGlossaryMovementAssets()
{
}
#endif

class FGlossaryMovementModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::RegisterBuilder(TEXT("01_Movement"), &BuildGlossaryMovementAssets);
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::UnregisterBuilder(TEXT("01_Movement"));
#endif
	}
};

IMPLEMENT_MODULE(FGlossaryMovementModule, GlossaryMovement);
