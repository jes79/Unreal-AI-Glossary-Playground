#include "Demo1001_LumenGI.h"

#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "GlossaryKit.h"
#include "RenderingDemoKit.h"

using namespace GlossaryKit;

ADemo1001_LumenGI::ADemo1001_LumenGI()
{
	DemoId = TEXT("1001");
	Controls = TEXT("1 : Lumen GI   2 : 끄기(None)   3 : Screen Space GI\n")
		TEXT("관찰: Lumen 을 켜면 빨간 벽에 맞은 빛이 흰 바닥과 상자를 붉게 물들이고 그늘도 밝아진다.\n")
		TEXT("끄면 스포트라이트가 직접 닿은 곳만 밝고 나머지는 거의 검다.");
	Showcase = TEXT("코넬 박스(빨강·초록 벽), 빨간 벽에 비춘 스포트라이트의 반사광이 바닥을 물들임, Lumen/None/SSGI 전환");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::None;   // 하늘빛 없이 방 안 조명만
	Environment.bFloor = false;
	Environment.ExposureEV100 = 10.f;
}

void ADemo1001_LumenGI::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 코넬 박스: 가로(Y) 600 · 깊이(X) 600 · 높이 500, 카메라 쪽(-X)은 열려 있다
	const FLinearColor White(0.75f, 0.75f, 0.75f), Red(0.75f, 0.04f, 0.03f), Green(0.05f, 0.6f, 0.06f);
	SpawnShape(World, EShape::Cube, FVector(0.f, 0.f, -10.f), FVector(600.f, 640.f, 20.f), White);       // 바닥
	SpawnShape(World, EShape::Cube, FVector(0.f, 0.f, 510.f), FVector(600.f, 640.f, 20.f), White);       // 천장
	SpawnShape(World, EShape::Cube, FVector(310.f, 0.f, 250.f), FVector(20.f, 640.f, 520.f), White);     // 뒷벽
	SpawnShape(World, EShape::Cube, FVector(0.f, -310.f, 250.f), FVector(600.f, 20.f, 520.f), Red);      // 왼쪽 빨간 벽
	SpawnShape(World, EShape::Cube, FVector(0.f, 310.f, 250.f), FVector(600.f, 20.f, 520.f), Green);     // 오른쪽 초록 벽
	// 안쪽 흰 상자 두 개 (키 큰 것·낮은 것)
	SpawnShape(World, EShape::Cube, FVector(120.f, 110.f, 160.f), FVector(160.f, 160.f, 320.f), White, FRotator(0.f, 20.f, 0.f));
	SpawnShape(World, EShape::Cube, FVector(-60.f, -100.f, 80.f), FVector(160.f, 160.f, 160.f), White, FRotator(0.f, -18.f, 0.f));

	// 빨간 벽을 비추는 스포트라이트 (천장 근처 오른쪽에서 왼쪽 벽으로)
	ASpotLight* Spot = World->SpawnActor<ASpotLight>(FVector(-100.f, 200.f, 470.f), (FVector(0.f, -300.f, 170.f) - FVector(-100.f, 200.f, 470.f)).Rotation());
	USpotLightComponent* SpotComp = Spot->SpotLightComponent;
	SpotComp->SetMobility(EComponentMobility::Movable);
	SpotComp->SetIntensityUnits(ELightUnits::Candelas);
	SpotComp->SetIntensity(SpotIntensity);
	SpotComp->SetInnerConeAngle(18.f);
	SpotComp->SetOuterConeAngle(30.f);
	SpotComp->SetAttenuationRadius(2000.f);
	SpotComp->SetLightColor(FLinearColor(1.f, 0.95f, 0.9f));

	// 천장 조명: 아주 약한 작은 전구(이것만으로는 방이 어둡다)
	APointLight* Bulb = World->SpawnActor<APointLight>(FVector(0.f, 0.f, 480.f), FRotator::ZeroRotator);
	Bulb->PointLightComponent->SetMobility(EComponentMobility::Movable);
	Bulb->PointLightComponent->SetIntensityUnits(ELightUnits::Candelas);
	Bulb->PointLightComponent->SetIntensity(4.f);
	Bulb->PointLightComponent->SetAttenuationRadius(1500.f);

	SpawnLabel(World, TEXT("빨간 벽 → 바닥이 붉게 물드는지"), FVector(0.f, -200.f, 30.f), Colors::Item, 18.f);

	RenderKit::SpawnLookAtCamera(this, FVector(-1050.f, 0.f, 260.f), FVector(0.f, 0.f, 220.f), 50.f);

	PostProcess = RenderKit::AddPostProcess(this);
	ApplyMethod();
}

void ADemo1001_LumenGI::ApplyMethod()
{
	// 핵심: Post Process Volume 의 Global Illumination Method 덮어쓰기
	FPostProcessSettings& PP = PostProcess->Settings;
	PP.bOverride_DynamicGlobalIlluminationMethod = true;
	PP.DynamicGlobalIlluminationMethod = DynamicGlobalIlluminationMethod;
	// Lumen 반사는 Lumen GI 가 있어야 동작하므로 GI 를 끄면 반사도 SSR 로 내린다
	PP.bOverride_ReflectionMethod = true;
	PP.ReflectionMethod = DynamicGlobalIlluminationMethod == EDynamicGlobalIlluminationMethod::Lumen ? EReflectionMethod::Lumen : EReflectionMethod::ScreenSpace;
}

void ADemo1001_LumenGI::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One))   { DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::Lumen; ApplyMethod(); }
	if (WasKeyPressed(EKeys::Two))   { DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::None; ApplyMethod(); }
	if (WasKeyPressed(EKeys::Three)) { DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::ScreenSpace; ApplyMethod(); }

	Stats.Update(DeltaSeconds);
	const TCHAR* Name = DynamicGlobalIlluminationMethod == EDynamicGlobalIlluminationMethod::Lumen ? TEXT("Lumen (켜짐)")
		: DynamicGlobalIlluminationMethod == EDynamicGlobalIlluminationMethod::None ? TEXT("None (꺼짐)") : TEXT("Screen Space GI");
	SetStatus(FString::Printf(TEXT("Global Illumination: %s\n%s"), Name, *Stats.ToString(false)));
}
