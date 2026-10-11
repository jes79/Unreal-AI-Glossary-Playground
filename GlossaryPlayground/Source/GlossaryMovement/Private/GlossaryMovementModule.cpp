#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "GlossaryAssetKit.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"

/** 01_Movement 분야 데모가 쓰는 생성 에셋 (/Game/Glossary/01_Movement) */
static void BuildGlossaryMovementAssets()
{
	using namespace GlossaryAssetKit;

	// M_Water: 0116 Water Volume 을 눈으로 보이게 하는 반투명·양면 머티리얼 (Color · Opacity 파라미터)
	UMaterial* Water = CreateMaterial(TEXT("/Game/Glossary/01_Movement/M_Water"));
	Water->BlendMode = BLEND_Translucent;
	Water->TwoSided = true;   // 물속(상자 안)에서 봐도 면이 보이도록
	auto* Color = AddVectorParam(Water, TEXT("Color"), FLinearColor(0.05f, 0.35f, 0.9f), -600, -200);
	auto* Opacity = AddScalarParam(Water, TEXT("Opacity"), 0.35f, -600, 0);
	auto* Glow = AddScalarParam(Water, TEXT("Emissive"), 0.15f, -600, 150);
	auto* EmissiveColor = AddExpression<UMaterialExpressionMultiply>(Water, -300, 150);
	Connect(Color, TEXT(""), EmissiveColor, TEXT("A"));
	Connect(Glow, TEXT(""), EmissiveColor, TEXT("B"));
	ConnectToPin(Color, MP_BaseColor);
	ConnectToPin(Opacity, MP_Opacity);
	ConnectToPin(EmissiveColor, MP_EmissiveColor);
	FinishMaterial(Water);
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
