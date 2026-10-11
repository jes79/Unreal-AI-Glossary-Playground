#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "GlossaryAssetKit.h"

/** 08_Systems 분야 데모가 쓰는 생성 에셋 (/Game/Glossary/08_Systems). 없으면 비워 둔다. */
static void BuildGlossarySystemsAssets()
{
}
#endif

class FGlossarySystemsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::RegisterBuilder(TEXT("08_Systems"), &BuildGlossarySystemsAssets);
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::UnregisterBuilder(TEXT("08_Systems"));
#endif
	}
};

IMPLEMENT_MODULE(FGlossarySystemsModule, GlossarySystems);
