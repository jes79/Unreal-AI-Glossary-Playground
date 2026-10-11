#include "Demo1009_ReflectionCapture.h"

#include "Camera/CameraActor.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SphereReflectionCaptureComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/PointLight.h"
#include "Engine/SphereReflectionCapture.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

namespace
{
	constexpr float RoomOffsetY = 650.f;
}

ADemo1009_ReflectionCapture::ADemo1009_ReflectionCapture()
{
	DemoId = TEXT("1009");
	Controls = TEXT("1 : Reflection Capture ON/OFF   2 : 비교용 Lumen GI·반사 ON/OFF\n")
		TEXT("WASD·QE·←→↑↓ 카메라\n")
		TEXT("관찰: Lumen 을 끈 상태에서 캡처가 있으면 크롬 구·바닥이 자기 방 색(주황/파랑)을 비춘다.\n")
		TEXT("캡처를 끄면 두 방 모두 하늘빛만 비쳐 똑같이 회색이 된다.");
	Showcase = TEXT("Lumen 끈 모바일 설정, 주황·파란 방마다 Sphere Reflection Capture, 크롬 구 반사 ON/OFF·Lumen 비교");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Day;
}

#if WITH_EDITOR
void ADemo1009_ReflectionCapture::BuildLevel(UWorld* World)
{
	// 방마다 Sphere Reflection Capture 하나 (레벨에 저장). 방 벽은 실행 중에 생기므로 Runtime Capture 로 다시 찍는다
	for (const float Y : {-RoomOffsetY, RoomOffsetY})
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ASphereReflectionCapture* Capture = World->SpawnActor<ASphereReflectionCapture>(FVector(0.f, Y, 200.f), FRotator::ZeroRotator, Params);
		USphereReflectionCaptureComponent* C = Cast<USphereReflectionCaptureComponent>(Capture->GetCaptureComponent());
		C->InfluenceRadius = 700.f;
		C->bRuntimeCapture = true;
		Capture->SetActorLabel(Y < 0.f ? TEXT("ReflectionCapture_OrangeRoom") : TEXT("ReflectionCapture_BlueRoom"));
	}
}
#endif

void ADemo1009_ReflectionCapture::BuildRoom(float Y, const FLinearColor& WallColor, const FLinearColor& LightColor)
{
	UWorld* World = GetWorld();
	// 방: X -450~450, 폭 1000(Y±500), 높이 450, 카메라 쪽(-X) 열림
	SpawnShape(World, EShape::Cube, FVector(465.f, Y, 225.f), FVector(30.f, 1000.f, 450.f), WallColor);
	SpawnShape(World, EShape::Cube, FVector(0.f, Y - 515.f, 225.f), FVector(960.f, 30.f, 450.f), WallColor);
	SpawnShape(World, EShape::Cube, FVector(0.f, Y + 515.f, 225.f), FVector(960.f, 30.f, 450.f), WallColor);
	SpawnShape(World, EShape::Cube, FVector(0.f, Y, 465.f), FVector(960.f, 1060.f, 30.f), WallColor);
	// 광택 바닥 (Roughness 0.1)
	AStaticMeshActor* Floor = SpawnShape(World, EShape::Cube, FVector(0.f, Y, 2.f), FVector(900.f, 1000.f, 4.f), FLinearColor::White);
	SetColor(Floor->GetStaticMeshComponent(), FLinearColor(0.05f, 0.05f, 0.05f), 0.1f, 0.f);
	// 크롬 구 (Metallic 1, Roughness 0.05) + 금속 상자
	AStaticMeshActor* Chrome = SpawnShape(World, EShape::Sphere, FVector(50.f, Y, 130.f), FVector(240.f), FLinearColor::White);
	SetColor(Chrome->GetStaticMeshComponent(), FLinearColor(0.95f, 0.95f, 0.95f), 0.05f, 1.f);
	AStaticMeshActor* Box = SpawnShape(World, EShape::Cube, FVector(250.f, Y + 250.f, 80.f), FVector(160.f), FLinearColor::White, FRotator(0.f, 30.f, 0.f));
	SetColor(Box->GetStaticMeshComponent(), FLinearColor(0.9f, 0.8f, 0.5f), 0.15f, 1.f);
	// 방을 밝히는 색 조명
	APointLight* Light = World->SpawnActor<APointLight>(FVector(100.f, Y - 200.f, 380.f), FRotator::ZeroRotator);
	Light->PointLightComponent->SetMobility(EComponentMobility::Movable);
	Light->PointLightComponent->SetIntensityUnits(ELightUnits::Candelas);
	Light->PointLightComponent->SetIntensity(60.f);
	Light->PointLightComponent->SetAttenuationRadius(1500.f);
	Light->PointLightComponent->SetLightColor(LightColor);
}

void ADemo1009_ReflectionCapture::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	BuildRoom(-RoomOffsetY, FLinearColor(0.9f, 0.35f, 0.05f), FLinearColor(1.f, 0.7f, 0.4f));
	BuildRoom(RoomOffsetY, FLinearColor(0.05f, 0.3f, 0.9f), FLinearColor(0.5f, 0.75f, 1.f));
	SpawnLabel(World, TEXT("주황 방"), FVector(-300.f, -RoomOffsetY, 420.f), Colors::Accent, 18.f);
	SpawnLabel(World, TEXT("파란 방"), FVector(-300.f, RoomOffsetY, 420.f), Colors::Cyan, 18.f);

	// bRuntimeCapture 캡처는 r.ReflectionCapture.Runtime 1 일 때만 그려진다 (0 = 구워 둔 캡처만)
	CVars.Set(TEXT("r.ReflectionCapture.Runtime"), 1);
	for (TActorIterator<ASphereReflectionCapture> It(World); It; ++It)
	{
		Captures.Add(It->GetCaptureComponent());
	}

	Camera = RenderKit::SpawnLookAtCamera(this, FVector(-1500.f, 0.f, 330.f), FVector(100.f, 0.f, 120.f), 70.f);
	PostProcess = RenderKit::AddPostProcess(this);
	Apply();
	RefreshFrames = 3;   // 방이 다 만들어진 다음 프레임에 캡처를 다시 찍는다
}

void ADemo1009_ReflectionCapture::Apply()
{
	// 모바일처럼: Lumen GI·반사 끔 → 반사는 Reflection Capture(+Sky Light)에서만 온다
	FPostProcessSettings& PP = PostProcess->Settings;
	PP.bOverride_DynamicGlobalIlluminationMethod = true;
	PP.DynamicGlobalIlluminationMethod = bLumen ? EDynamicGlobalIlluminationMethod::Lumen : EDynamicGlobalIlluminationMethod::None;
	PP.bOverride_ReflectionMethod = true;
	PP.ReflectionMethod = bLumen ? EReflectionMethod::Lumen : EReflectionMethod::None;

	for (UReflectionCaptureComponent* C : Captures)
	{
		// 캡처 컴포넌트를 씬에서 빼고 넣는다 (꺼지면 Sky Light 큐브맵으로 대체)
		if (bCapturesEnabled && !C->IsRegistered())
		{
			C->RegisterComponent();
			C->RefreshCapture(true);
		}
		else if (!bCapturesEnabled && C->IsRegistered())
		{
			C->UnregisterComponent();
		}
	}
}

void ADemo1009_ReflectionCapture::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (RefreshFrames > 0 && --RefreshFrames == 0)
	{
		for (UReflectionCaptureComponent* C : Captures)
		{
			if (C->IsRegistered())
			{
				C->RefreshCapture(true);
			}
		}
	}
	if (WasKeyPressed(EKeys::One)) { bCapturesEnabled = !bCapturesEnabled; Apply(); }
	if (WasKeyPressed(EKeys::Two)) { bLumen = !bLumen; Apply(); }
	RenderKit::TickFlyCamera(this, Camera, DeltaSeconds, 500.f);

	Stats.Update(DeltaSeconds);
	SetStatus(FString::Printf(TEXT("Reflection Capture %s (%d개, Runtime Capture) · Lumen %s\nGI %s · Reflection %s\n%s"),
		bCapturesEnabled ? TEXT("ON") : TEXT("OFF"), Captures.Num(), bLumen ? TEXT("ON") : TEXT("OFF (모바일 설정)"),
		bLumen ? TEXT("Lumen") : TEXT("None"), bLumen ? TEXT("Lumen") : TEXT("None (캡처 + 스카이라이트)"), *Stats.ToString(false)));
}

void ADemo1009_ReflectionCapture::EndPlay(const EEndPlayReason::Type Reason)
{
	CVars.RestoreAll();
	Super::EndPlay(Reason);
}
