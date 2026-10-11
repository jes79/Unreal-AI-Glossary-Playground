#include "Demo0506_MeshParticles.h"

#include "EffectsDemoKit.h"
#include "GlossaryKit.h"
#include "NiagaraComponent.h"

using namespace GlossaryKit;

ADemo0506_MeshParticles::ADemo0506_MeshParticles()
{
	DemoId = TEXT("0506");
	Controls = TEXT("Space : 다시 터뜨리기\n")
		TEXT("관찰: 오른쪽은 Mesh Renderer — 입자 하나하나가 큐브 메시로 그려져 회전하며 떨어진다.\n")
		TEXT("왼쪽은 같은 움직임을 Sprite Renderer 로 그린 것(납작한 사각형, 회전 입체감 없음)");
	Showcase = TEXT("큐브 메시 입자 파편이 회전하며 떨어지는 Mesh Renderer vs 같은 움직임의 Sprite 비교");
	PlayerPawnClass = nullptr;
}

void ADemo0506_MeshParticles::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	MeshDebris = EffectsKit::AddNiagara(this, TEXT("NS_MeshDebris"), FVector(0.f, 300.f, 150.f));
	SpriteDebris = EffectsKit::AddNiagara(this, TEXT("NS_SpriteDebris"), FVector(0.f, -300.f, 150.f));
	SpawnLabel(World, TEXT("Sprite Renderer"), FVector(0.f, -300.f, 600.f), FLinearColor::White, 22.f);
	SpawnLabel(World, TEXT("Mesh Renderer (Cube)"), FVector(0.f, 300.f, 600.f), Colors::Item, 22.f);
	EffectsKit::SetupFixedCamera(this, FVector(-1100.f, 0.f, 420.f), FVector(0.f, 0.f, 220.f), 65.f);
}

void ADemo0506_MeshParticles::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Timer += DeltaSeconds;
	if (WasKeyPressed(EKeys::SpaceBar) || Timer >= BurstInterval)
	{
		Timer = 0.f;
		if (MeshDebris) MeshDebris->ResetSystem();
		if (SpriteDebris) SpriteDebris->ResetSystem();
	}
	SetStatus(FString::Printf(TEXT("Mesh Renderer: 큐브 40개 · 무작위 회전 속도 + 중력 · 다음 폭발 %.1f초"), FMath::Max(0.f, BurstInterval - Timer)));
}
