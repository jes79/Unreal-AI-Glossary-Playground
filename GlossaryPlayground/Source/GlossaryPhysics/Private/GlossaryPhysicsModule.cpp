#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "GlossaryAssetKit.h"

/** 02_Physics 분야 데모가 쓰는 생성 에셋 (/Game/Glossary/02_Physics). 없으면 비워 둔다. */
static void BuildGlossaryPhysicsAssets()
{
}
#endif

class FGlossaryPhysicsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::RegisterBuilder(TEXT("02_Physics"), &BuildGlossaryPhysicsAssets);
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::UnregisterBuilder(TEXT("02_Physics"));
#endif
	}
};

IMPLEMENT_MODULE(FGlossaryPhysicsModule, GlossaryPhysics);
