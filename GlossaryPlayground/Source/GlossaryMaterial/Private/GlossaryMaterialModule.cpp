#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "GlossaryAssetKit.h"

/** 04_Material 분야 데모가 쓰는 생성 에셋 (/Game/Glossary/04_Material). 구현은 GlossaryMaterialAssets.cpp */
void BuildGlossaryMaterialAssets();
#endif

class FGlossaryMaterialModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::RegisterBuilder(TEXT("04_Material"), &BuildGlossaryMaterialAssets);
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::UnregisterBuilder(TEXT("04_Material"));
#endif
	}
};

IMPLEMENT_MODULE(FGlossaryMaterialModule, GlossaryMaterial);
