#include "Demo0512_MuzzleFlash.h"

#include "Components/PointLightComponent.h"
#include "EffectsDemoKit.h"
#include "GlossaryKit.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

using namespace GlossaryKit;

ADemo0512_MuzzleFlash::ADemo0512_MuzzleFlash()
{
	DemoId = TEXT("0512");
	Controls = TEXT("F : 자동 연사 ON/OFF   마우스 왼쪽 / Space : 한 발   T : 슬로 모션(×0.05)   H : 화염 계속 보기\n")
		TEXT("관찰: 총구(Muzzle) 위치에 0.05초짜리 화염 Niagara 가 붙고, 같은 0.05초 동안 Point Light 가 켜져\n")
		TEXT("벽과 바닥이 번쩍인다. 실제 속도로는 거의 한 프레임 — 슬로 모션으로 보면 화염 모양이 보인다");
	Showcase = TEXT("총구 소켓에 0.05초 화염 Niagara + Point Light 깜빡임, 자동 연사와 슬로 모션");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Night;
	Environment.ExposureEV100 = 10.f;
}

void ADemo0512_MuzzleFlash::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 총: 몸통 + 손잡이 + 총열(+Y 방향). 총구 끝에 Muzzle 컴포넌트(스켈레탈 메시의 소켓과 같은 역할)
	const FVector GunPos(0.f, -150.f, 150.f);
	AddShape(this, EShape::Cube, GunPos, FVector(14.f, 80.f, 26.f), Colors::Dark);
	AddShape(this, EShape::Cube, GunPos + FVector(0.f, -20.f, -28.f), FVector(12.f, 18.f, 40.f), Colors::Dark, nullptr, FRotator(0.f, 0.f, 15.f));
	AddShape(this, EShape::Cylinder, GunPos + FVector(0.f, 75.f, 5.f), FVector(8.f, 8.f, 80.f), FLinearColor(0.15f, 0.15f, 0.17f), nullptr, FRotator(0.f, 0.f, 90.f));
	Muzzle = NewObject<USceneComponent>(this, TEXT("Muzzle"));
	Muzzle->SetupAttachment(GetRootComponent());
	Muzzle->SetRelativeLocationAndRotation(GunPos + FVector(0.f, 118.f, 5.f), FRotator(0.f, 90.f, 0.f));   // 로컬 +X = 발사 방향
	Muzzle->RegisterComponent();

	FlashLight = EffectsKit::AddPointLight(this, FVector(0.f, 20.f, 0.f), FLinearColor(1.f, 0.6f, 0.25f), 0.f, 900.f, Muzzle);

	// 빛을 받는 벽
	SpawnBlock(World, FVector(250.f, 0.f, 0.f), FVector(40.f, 1200.f, 500.f), Colors::Wall);
	SpawnLabel(World, TEXT("Muzzle"), GunPos + FVector(0.f, 120.f, 60.f), Colors::Item, 18.f);
	EffectsKit::SetupFixedCamera(this, FVector(-420.f, -60.f, 200.f), GunPos + FVector(0.f, 90.f, 0.f), 60.f);
}

void ADemo0512_MuzzleFlash::Fire()
{
	// 핵심: 총구 컴포넌트에 화염 시스템을 붙여 스폰 (끝나면 자동 파괴)
	UNiagaraFunctionLibrary::SpawnSystemAttached(EffectsKit::LoadSystem(TEXT("NS_MuzzleFlash")), Muzzle, NAME_None,
		FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
	LightTimer = FlashDuration;
	if (FlashLight)
	{
		FlashLight->SetIntensity(LightIntensity);
	}
	++Shots;
}

void ADemo0512_MuzzleFlash::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::F)) { bAutoFire = !bAutoFire; }
	if (WasKeyPressed(EKeys::T))
	{
		bSlowMo = !bSlowMo;
		UGameplayStatics::SetGlobalTimeDilation(this, bSlowMo ? 0.05f : 1.f);
	}
	if (WasKeyPressed(EKeys::LeftMouseButton) || WasKeyPressed(EKeys::SpaceBar))
	{
		Fire();
	}
	if (WasKeyPressed(EKeys::H)) { bHold = !bHold; }
	if (bHold) { FireTimer += DeltaSeconds; if (FireTimer >= FlashDuration) { FireTimer = 0.f; Fire(); } }
	else if (bAutoFire)
	{
		FireTimer += DeltaSeconds;
		if (FireTimer >= FireInterval)
		{
			FireTimer = 0.f;
			Fire();
		}
	}
	// Point Light 는 FlashDuration(0.05초) 동안만 켜진다
	if (LightTimer > 0.f)
	{
		LightTimer -= DeltaSeconds;
		if (LightTimer <= 0.f && FlashLight)
		{
			FlashLight->SetIntensity(0.f);
		}
	}
	SetStatus(FString::Printf(TEXT("발사 %d · 화염 + 라이트 %.2f초 · 연사 간격 %.2f초 · 자동 %s · 슬로 모션 %s · 라이트 %s"),
		Shots, FlashDuration, FireInterval, bAutoFire ? TEXT("ON") : TEXT("OFF"), bSlowMo ? TEXT("ON") : TEXT("OFF"),
		LightTimer > 0.f ? TEXT("켜짐") : TEXT("꺼짐")));
}
