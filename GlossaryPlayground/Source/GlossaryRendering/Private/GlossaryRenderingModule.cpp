#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "GlossaryAssetKit.h"
#include "RenderingAssets.h"   // BuildGlossaryRenderingAssets (RenderingAssets.cpp)
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
