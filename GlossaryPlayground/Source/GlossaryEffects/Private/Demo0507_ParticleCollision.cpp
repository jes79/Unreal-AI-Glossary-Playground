#include "Demo0507_ParticleCollision.h"

#include "EffectsDemoKit.h"
#include "GlossaryKit.h"
#include "NiagaraComponent.h"

using namespace GlossaryKit;

ADemo0507_ParticleCollision::ADemo0507_ParticleCollision()
{
	DemoId = TEXT("0507");
	Controls = TEXT("관찰: 오른쪽 불똥은 Collision 모듈(Restitution 0.5)로 바닥에 닿으면 절반 속도로 튕긴다.\n")
		TEXT("왼쪽은 같은 불똥에 Collision 이 없어서 바닥을 그대로 뚫고 내려간다");
	Showcase = TEXT("불똥 분수: Collision 모듈(Restitution 0.5)로 바닥에 튕김 vs 충돌 없음(바닥 통과)");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Night;
	Environment.ExposureEV100 = 3.f;
}

void ADemo0507_ParticleCollision::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	WithoutCollision = EffectsKit::AddNiagara(this, TEXT("NS_SparksNoCollision"), FVector(0.f, -350.f, 260.f));
	WithCollision = EffectsKit::AddNiagara(this, TEXT("NS_SparksCollision"), FVector(0.f, 350.f, 260.f));
	SpawnShape(World, EShape::Cylinder, FVector(0.f, -350.f, 130.f), FVector(20.f, 20.f, 260.f), Colors::Wall);
	SpawnShape(World, EShape::Cylinder, FVector(0.f, 350.f, 130.f), FVector(20.f, 20.f, 260.f), Colors::Wall);
	SpawnLabel(World, TEXT("Collision 없음 → 바닥 통과"), FVector(0.f, -350.f, 460.f), FLinearColor::White, 20.f);
	SpawnLabel(World, TEXT("Collision (Restitution 0.5) → 튕김"), FVector(0.f, 350.f, 460.f), Colors::Item, 20.f);
	EffectsKit::SetupFixedCamera(this, FVector(-1000.f, 0.f, 120.f), FVector(0.f, 0.f, 120.f), 70.f);
}

void ADemo0507_ParticleCollision::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	SetStatus(FString::Printf(TEXT("Collision 모듈: Restitution %.1f (튕긴 뒤 속도 = 부딪힌 속도 × %.1f) · CPU 레이 트레이스로 바닥 판정"), Restitution, Restitution));
}
