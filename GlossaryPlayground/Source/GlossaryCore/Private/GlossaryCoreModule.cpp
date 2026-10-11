#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "GlossaryAssetKit.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"

/** 모든 데모가 쓰는 공통 에셋: M_Color (GlossaryKit::SetColor 가 MID 로 사용) */
static void BuildCommonAssets()
{
	using namespace GlossaryAssetKit;
	UMaterial* Material = CreateMaterial(TEXT("/Game/Glossary/Common/M_Color"));
	// 캐릭터(스켈레탈)·인스턴스 메시에도 쓰므로 Usage 를 미리 켠다 (안 켜면 실행 중 기본 회색 머티리얼로 바뀜)
	Material->bUsedWithSkeletalMesh = true;
	Material->bUsedWithInstancedStaticMeshes = true;
	Material->bUsedWithNiagaraMeshParticles = true;
	auto* Color = AddVectorParam(Material, TEXT("Color"), FLinearColor(0.8f, 0.8f, 0.8f), -600, -200);
	auto* Roughness = AddScalarParam(Material, TEXT("Roughness"), 0.6f, -600, 0);
	auto* Metallic = AddScalarParam(Material, TEXT("Metallic"), 0.f, -600, 100);
	auto* Emissive = AddScalarParam(Material, TEXT("Emissive"), 0.f, -600, 200);
	auto* EmissiveColor = AddExpression<UMaterialExpressionMultiply>(Material, -300, 200);
	Connect(Color, TEXT(""), EmissiveColor, TEXT("A"));
	Connect(Emissive, TEXT(""), EmissiveColor, TEXT("B"));
	ConnectToPin(Color, MP_BaseColor);
	ConnectToPin(Roughness, MP_Roughness);
	ConnectToPin(Metallic, MP_Metallic);
	ConnectToPin(EmissiveColor, MP_EmissiveColor);
	FinishMaterial(Material);
}
#endif

class FGlossaryCoreModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::RegisterBuilder(TEXT("00_Common"), &BuildCommonAssets);
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::UnregisterBuilder(TEXT("00_Common"));
#endif
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FGlossaryCoreModule, GlossaryCore, "GlossaryCore");
