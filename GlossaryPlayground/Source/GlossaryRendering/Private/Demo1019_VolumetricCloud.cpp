#include "Demo1019_VolumetricCloud.h"

#include "Camera/CameraActor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "GlossaryEnvironment.h"
#include "GlossaryKit.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace GlossaryKit;

ADemo1019_VolumetricCloud::ADemo1019_VolumetricCloud()
{
	DemoId = TEXT("1019");
	Controls = TEXT("[ / ] : 구름 양 -0.1 / +0.1 (머티리얼 파라미터)   - / = : Layer Bottom Altitude -1km / +1km\n")
		TEXT(", / . : 해 고도   ←→↑↓ 카메라 회전\n")
		TEXT("관찰: 구름 양을 올리면 하늘이 두꺼운 구름으로 덮이고 그림자도 진해진다. 고도를 내리면 구름이 가깝고 크게 보인다.");
	Showcase = TEXT("Volumetric Cloud Layer Bottom Altitude 5km, 구름 양을 머티리얼 파라미터(Cloud_GlobalCoverage)로 실시간 조절, 해 고도");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Day;
	Environment.bClouds = true;
	Environment.bFixedExposure = false;
}

void ADemo1019_VolumetricCloud::BeginPlay()
{
	Super::BeginPlay();
	SpawnBlock(GetWorld(), FVector(1500.f, 0.f, 0.f), FVector(400.f, 400.f, 1200.f), FLinearColor(0.7f, 0.7f, 0.68f));
	SpawnBlock(GetWorld(), FVector(2500.f, -1200.f, 0.f), FVector(500.f, 500.f, 800.f), FLinearColor(0.6f, 0.62f, 0.66f));
	Camera = RenderKit::SpawnViewCamera(this, FVector(-600.f, 0.f, 200.f), FRotator(25.f, 0.f, 0.f), 90.f);

	if (AGlossaryEnvironment* Env = AGlossaryEnvironment::Find(this))
	{
		Cloud = Env->FindComponentByClass<UVolumetricCloudComponent>();
	}
	Apply();
}

void ADemo1019_VolumetricCloud::Apply()
{
	if (!Cloud)
	{
		return;
	}
	Cloud->SetVisibility(true);
	if (!CloudMID)
	{
		// 구름 머티리얼(엔진 기본)을 Dynamic Material Instance 로 감싸 파라미터를 실행 중에 바꾼다
		UMaterialInterface* Base = Cloud->GetMaterial();
		if (!Base)
		{
			Base = LoadAsset<UMaterialInterface>(TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst"));
		}
		if (Base)
		{
			CloudMID = UMaterialInstanceDynamic::Create(Base, this);
			Cloud->SetMaterial(CloudMID);
			// 이 머티리얼이 가진 스칼라 파라미터 목록 (정보 표시용)
			TArray<FMaterialParameterInfo> Infos;
			TArray<FGuid> Ids;
			CloudMID->GetAllScalarParameterInfo(Infos, Ids);
			float Default = 0.f;
			if (CloudMID->GetScalarParameterValue(FHashedMaterialParameterInfo(CoverageParameter), Default))
			{
				ParamInfo = FString::Printf(TEXT("%s 기본값 %.2f · 스칼라 파라미터 %d개"), *CoverageParameter.ToString(), Default, Infos.Num());
			}
			else
			{
				ParamInfo = FString::Printf(TEXT("파라미터 %s 없음 (스칼라 %d개)"), *CoverageParameter.ToString(), Infos.Num());
			}
		}
	}
	// 핵심: 구름 층 고도·두께(km)와 구름 양 파라미터
	Cloud->SetLayerBottomAltitude(LayerBottomAltitude);
	Cloud->SetLayerHeight(LayerHeight);
	if (CloudMID)
	{
		CloudMID->SetScalarParameterValue(CoverageParameter, CloudCoverage);
	}

	if (AGlossaryEnvironment* Env = AGlossaryEnvironment::Find(this))
	{
		Env->GetSun()->SetWorldRotation(FRotator(-SunElevation, -35.f, 0.f));
	}
}

void ADemo1019_VolumetricCloud::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	bool bChanged = !bApplied;
	bApplied = true;
	if (WasKeyPressed(EKeys::LeftBracket))  { CloudCoverage = FMath::Clamp(CloudCoverage - 0.1f, 0.f, 1.f); bChanged = true; }
	if (WasKeyPressed(EKeys::RightBracket)) { CloudCoverage = FMath::Clamp(CloudCoverage + 0.1f, 0.f, 1.f); bChanged = true; }
	if (WasKeyPressed(EKeys::Hyphen)) { LayerBottomAltitude = FMath::Max(0.5f, LayerBottomAltitude - 1.f); bChanged = true; }
	if (WasKeyPressed(EKeys::Equals)) { LayerBottomAltitude = FMath::Min(15.f, LayerBottomAltitude + 1.f); bChanged = true; }
	if (WasKeyPressed(EKeys::Comma))  { SunElevation = FMath::Clamp(SunElevation - 5.f, -5.f, 90.f); bChanged = true; }
	if (WasKeyPressed(EKeys::Period)) { SunElevation = FMath::Clamp(SunElevation + 5.f, -5.f, 90.f); bChanged = true; }
	if (bChanged)
	{
		Apply();
	}
	RenderKit::TickFlyCamera(this, Camera, DeltaSeconds, 300.f);

	Stats.Update(DeltaSeconds);
	SetStatus(FString::Printf(TEXT("Layer Bottom Altitude %.1fkm · Layer Height %.1fkm · 구름 양(%s) %.1f · 해 고도 %.0f°\n%s\n%s"),
		LayerBottomAltitude, LayerHeight, *CoverageParameter.ToString(), CloudCoverage, SunElevation,
		Cloud ? *ParamInfo : TEXT("Volumetric Cloud 컴포넌트 없음"), *Stats.ToString(false)));
}
