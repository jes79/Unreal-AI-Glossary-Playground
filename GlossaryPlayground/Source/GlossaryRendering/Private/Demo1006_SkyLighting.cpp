#include "Demo1006_SkyLighting.h"

#include "Camera/CameraActor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "GlossaryEnvironment.h"
#include "GlossaryKit.h"
#include "RenderingAssetPaths.h"

using namespace GlossaryKit;

ADemo1006_SkyLighting::ADemo1006_SkyLighting()
{
	DemoId = TEXT("1006");
	Controls = TEXT("[ / ] : 해 고도 -10° / +10°   , / . : 해 방향   T : 하루 자동 진행\n")
		TEXT("4 : Sky Atmosphere   5 : Sky Light   6 : Volumetric Cloud   7 : Directional Light 끄기/켜기\n")
		TEXT("관찰: 해를 낮추면 대기(Sky Atmosphere)가 노을색이 되고, Sky Light(Real Time Capture)가 그늘 색까지 바꾼다.");
	Showcase = TEXT("Directional Light(Atmosphere Sun)·Sky Atmosphere·Sky Light(Real Time Capture)·Volumetric Cloud, 해 고도 조절과 구성 요소 끄기");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Day;
	Environment.bClouds = true;
	Environment.bFixedExposure = false;   // 낮·노을·밤 밝기 차이가 커서 자동 노출을 쓴다
}

void ADemo1006_SkyLighting::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 마을 느낌의 소품: 집·나무·흰 구 (하늘빛이 그늘에 어떻게 비치는지 보기 좋다)
	for (int32 i = 0; i < 5; ++i)
	{
		const FVector Base(800.f + i * 450.f, (i % 2 ? 1.f : -1.f) * 500.f, 0.f);
		SpawnBlock(World, Base, FVector(300.f, 300.f, 250.f), FLinearColor(0.8f, 0.78f, 0.72f));
		SpawnShape(World, EShape::Cone, Base + FVector(0.f, 0.f, 330.f), FVector(380.f, 380.f, 160.f), FLinearColor(0.5f, 0.15f, 0.1f));
	}
	SpawnShape(World, EShape::Sphere, FVector(600.f, 0.f, 100.f), FVector(200.f), FLinearColor(0.9f, 0.9f, 0.9f));
	if (UStaticMesh* Tree = LoadAsset<UStaticMesh>(RenderingAssets::Tree))
	{
		FRandomStream Rand(1006);
		for (int32 i = 0; i < 40; ++i)
		{
			AStaticMeshActor* A = World->SpawnActor<AStaticMeshActor>(FVector(Rand.FRandRange(1500.f, 6000.f), Rand.FRandRange(-2500.f, 2500.f), 0.f), FRotator(0.f, Rand.FRandRange(0.f, 360.f), 0.f));
			A->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
			A->GetStaticMeshComponent()->SetStaticMesh(Tree);
		}
	}

	Camera = RenderKit::SpawnViewCamera(this, FVector(-600.f, 0.f, 250.f), FRotator(12.f, 0.f, 0.f), 80.f);
}

void ADemo1006_SkyLighting::ToggleComponent(UClass* Class, const TCHAR* Name)
{
	if (AGlossaryEnvironment* Env = AGlossaryEnvironment::Find(this))
	{
		if (USceneComponent* C = Cast<USceneComponent>(Env->GetComponentByClass(Class)))
		{
			C->SetVisibility(!C->IsVisible());
		}
	}
}

FString ADemo1006_SkyLighting::ComponentState() const
{
	FString S;
	if (AGlossaryEnvironment* Env = AGlossaryEnvironment::Find(this))
	{
		auto State = [Env](UClass* Class, const TCHAR* Name)
		{
			const USceneComponent* C = Cast<USceneComponent>(Env->GetComponentByClass(Class));
			return FString::Printf(TEXT("%s %s"), Name, C && C->IsVisible() ? TEXT("ON") : TEXT("off"));
		};
		S = FString::Printf(TEXT("%s · %s · %s · %s"),
			*State(UDirectionalLightComponent::StaticClass(), TEXT("Directional")),
			*State(USkyAtmosphereComponent::StaticClass(), TEXT("Atmosphere")),
			*State(USkyLightComponent::StaticClass(), TEXT("SkyLight")),
			*State(UVolumetricCloudComponent::StaticClass(), TEXT("Cloud")));
	}
	return S;
}

void ADemo1006_SkyLighting::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::LeftBracket))  { SunElevation = FMath::Clamp(SunElevation - 10.f, -30.f, 90.f); }
	if (WasKeyPressed(EKeys::RightBracket)) { SunElevation = FMath::Clamp(SunElevation + 10.f, -30.f, 90.f); }
	if (WasKeyPressed(EKeys::Comma))  { SunAzimuth -= 15.f; }
	if (WasKeyPressed(EKeys::Period)) { SunAzimuth += 15.f; }
	if (WasKeyPressed(EKeys::T)) { bAutoDayCycle = !bAutoDayCycle; }
	if (WasKeyPressed(EKeys::Four))  { ToggleComponent(USkyAtmosphereComponent::StaticClass(), TEXT("Atmosphere")); }
	if (WasKeyPressed(EKeys::Five))  { ToggleComponent(USkyLightComponent::StaticClass(), TEXT("SkyLight")); }
	if (WasKeyPressed(EKeys::Six))   { ToggleComponent(UVolumetricCloudComponent::StaticClass(), TEXT("Cloud")); }
	if (WasKeyPressed(EKeys::Seven)) { ToggleComponent(UDirectionalLightComponent::StaticClass(), TEXT("Sun")); }

	if (bAutoDayCycle)
	{
		// 60초에 한 바퀴: -30° ~ 90° 를 사인으로 오간다
		SunElevation = FMath::Fmod(SunElevation + DeltaSeconds * 6.f + 30.f, 120.f) - 30.f;
	}

	if (AGlossaryEnvironment* Env = AGlossaryEnvironment::Find(this))
	{
		// 해 = Directional Light 의 회전. Pitch 가 음수일수록 높이 뜬 해 (빛이 아래로 향함)
		UDirectionalLightComponent* Sun = Env->GetSun();
		Sun->SetWorldRotation(FRotator(-SunElevation, SunAzimuth, 0.f));
		// 해가 지평선 아래로 가면 세기를 줄여 밤으로 (Sky Atmosphere 는 해 방향만 보고 하늘색을 계산한다)
		Sun->SetIntensity(SunElevation > 0.f ? 8.f : FMath::Max(0.05f, 8.f * (1.f + SunElevation / 6.f)));
	}

	Stats.Update(DeltaSeconds);
	SetStatus(FString::Printf(TEXT("해 고도 %.0f° · 방향 %.0f° %s\n%s\n%s"), SunElevation, SunAzimuth,
		bAutoDayCycle ? TEXT("(자동 진행)") : TEXT(""), *ComponentState(), *Stats.ToString(false)));
}
