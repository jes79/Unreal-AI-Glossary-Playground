#include "GlossaryEnvironment.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "EngineUtils.h"
#include "GlossaryKit.h"
#include "Materials/MaterialInterface.h"

AGlossaryEnvironment::AGlossaryEnvironment()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Root->SetMobility(EComponentMobility::Static);

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(Root);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetAtmosphereSunLight(true);
	Sun->SetRelativeRotation(FRotator(-40.f, -35.f, 0.f));

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	Atmosphere->SetupAttachment(Root);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(Root);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->SetRealTimeCaptureEnabled(true);

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
	Fog->SetupAttachment(Root);
	Fog->SetFogDensity(0.01f);

	Clouds = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("VolumetricCloud"));
	Clouds->SetupAttachment(Root);

	Floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor"));
	Floor->SetupAttachment(Root);
	Floor->SetMobility(EComponentMobility::Static);
	Floor->SetCollisionProfileName(TEXT("BlockAll"));

	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetupAttachment(Root);
	PostProcess->bUnbound = true;
	PostProcess->Priority = -1.f; // 데모가 놓는 Post Process Volume 이 항상 이긴다
}

AGlossaryEnvironment* AGlossaryEnvironment::Find(const UObject* WorldContext)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AGlossaryEnvironment> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void AGlossaryEnvironment::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplySettings();
}

void AGlossaryEnvironment::BeginPlay()
{
	Super::BeginPlay();
	ApplySettings();
}

void AGlossaryEnvironment::ApplySettings()
{
	const bool bOutdoor = Settings.Sky == EGlossarySky::Day || Settings.Sky == EGlossarySky::Sunset || Settings.Sky == EGlossarySky::Night;
	const bool bAnyLight = Settings.Sky != EGlossarySky::None;

	Sun->SetVisibility(bOutdoor);
	Atmosphere->SetVisibility(bOutdoor);
	SkyLight->SetVisibility(bAnyLight);
	Fog->SetVisibility(Settings.bFog && bAnyLight);
	Clouds->SetVisibility(Settings.bClouds && bOutdoor);

	switch (Settings.Sky)
	{
	case EGlossarySky::Day:
		Sun->SetRelativeRotation(FRotator(-40.f, -35.f, 0.f));
		Sun->SetIntensity(8.f);
		Sun->SetLightColor(FLinearColor(1.f, 0.97f, 0.92f));
		SkyLight->SetIntensity(1.f);
		break;
	case EGlossarySky::Sunset:
		Sun->SetRelativeRotation(FRotator(-6.f, -60.f, 0.f));
		Sun->SetIntensity(6.f);
		Sun->SetLightColor(FLinearColor(1.f, 0.75f, 0.5f));
		SkyLight->SetIntensity(1.f);
		break;
	case EGlossarySky::Night:
		Sun->SetRelativeRotation(FRotator(-35.f, 20.f, 0.f));
		Sun->SetIntensity(0.25f);
		Sun->SetLightColor(FLinearColor(0.6f, 0.7f, 1.f));
		SkyLight->SetIntensity(0.3f);
		break;
	case EGlossarySky::Indoor:
		SkyLight->SetIntensity(0.25f);
		break;
	default:
		break;
	}

	// 실내는 하늘이 없으니 Sky Light 가 캡처할 게 없다 → 단색 큐브맵 대신 낮은 강도의 하늘색을 쓴다
	SkyLight->SetLowerHemisphereColor(FLinearColor(0.05f, 0.05f, 0.06f));
	if (Clouds->IsVisible() && !Clouds->GetMaterial())
	{
		Clouds->SetMaterial(GlossaryKit::LoadAsset<UMaterialInterface>(TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst")));
	}

	Floor->SetVisibility(Settings.bFloor);
	Floor->SetCollisionEnabled(Settings.bFloor ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	if (Settings.bFloor)
	{
		Floor->SetStaticMesh(GlossaryKit::GetShapeMesh(GlossaryKit::EShape::Plane));
		Floor->SetRelativeScale3D(FVector(Settings.FloorSize / 100.f, Settings.FloorSize / 100.f, 1.f));
		if (UMaterialInterface* Grid = GlossaryKit::LoadAsset<UMaterialInterface>(TEXT("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray")))
		{
			Floor->SetMaterial(0, Grid);
		}
	}

	FPostProcessSettings& PP = PostProcess->Settings;
	PP.bOverride_AutoExposureMethod = Settings.bFixedExposure;
	PP.AutoExposureMethod = AEM_Manual;
	PP.bOverride_AutoExposureBias = Settings.bFixedExposure;
	// Manual 노출에서 Exposure Compensation 은 EV100 처럼 동작한다(Apply Physical Camera Exposure 끔)
	PP.AutoExposureBias = Settings.bFixedExposure ? (Settings.ExposureEV100 - 10.f) : 0.f;
	PP.bOverride_AutoExposureApplyPhysicalCameraExposure = Settings.bFixedExposure;
	PP.AutoExposureApplyPhysicalCameraExposure = false;
}
