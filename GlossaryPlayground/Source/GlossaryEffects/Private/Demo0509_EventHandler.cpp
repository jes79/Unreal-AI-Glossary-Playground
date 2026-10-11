#include "Demo0509_EventHandler.h"

#include "EffectsDemoKit.h"
#include "GlossaryKit.h"
#include "NiagaraComponent.h"

using namespace GlossaryKit;

ADemo0509_EventHandler::ADemo0509_EventHandler()
{
	DemoId = TEXT("0509");
	Controls = TEXT("Space : 불꽃놀이 처음부터 다시\n")
		TEXT("관찰: Rocket 이미터의 입자가 수명을 다해 죽는 순간 Generate Death Event 가 위치를 보내고,\n")
		TEXT("Burst 이미터의 Event Handler(Spawned Particles)가 그 위치에서 폭죽 입자 80개를 스폰한다");
	Showcase = TEXT("불꽃놀이: 로켓 입자가 죽는 위치에서 Death Event → Event Handler 로 폭죽 입자 스폰");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Night;
	Environment.ExposureEV100 = 2.f;
}

void ADemo0509_EventHandler::BeginPlay()
{
	Super::BeginPlay();
	Fireworks = EffectsKit::AddNiagara(this, TEXT("NS_Fireworks"), FVector::ZeroVector);
	SpawnBlock(GetWorld(), FVector::ZeroVector, FVector(120.f, 120.f, 20.f), Colors::Wall);
	SpawnLabel(GetWorld(), TEXT("Rocket(죽을 때 DeathEvent) → Burst(Event Handler)"), FVector(0.f, 0.f, 120.f), Colors::Item, 18.f);
	EffectsKit::SetupFixedCamera(this, FVector(-2600.f, 0.f, 500.f), FVector(0.f, 0.f, 1100.f), 70.f);
}

void ADemo0509_EventHandler::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;
	if (WasKeyPressed(EKeys::SpaceBar) && Fireworks)
	{
		Fireworks->ResetSystem();
		Elapsed = 0.f;
	}
	SetStatus(FString::Printf(TEXT("Rocket: 초당 1.5발, 수명 1.2~1.6초 · Death Event 1개 → 폭죽 80개 · 경과 %.1f초"), Elapsed));
}
