#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "GlossaryAssetKit.h"

/** 09_Audio 분야 데모가 쓰는 생성 에셋 (/Game/Glossary/09_Audio). 구현은 GlossaryAudioAssets.cpp */
void BuildGlossaryAudioAssets();
#endif

class FGlossaryAudioModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::RegisterBuilder(TEXT("09_Audio"), &BuildGlossaryAudioAssets);
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::UnregisterBuilder(TEXT("09_Audio"));
#endif
	}
};

IMPLEMENT_MODULE(FGlossaryAudioModule, GlossaryAudio);
