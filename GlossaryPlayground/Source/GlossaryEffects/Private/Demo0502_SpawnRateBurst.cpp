#include "Demo0502_SpawnRateBurst.h"

#include "EffectsDemoKit.h"
#include "GlossaryKit.h"
#include "NiagaraComponent.h"

using namespace GlossaryKit;

ADemo0502_SpawnRateBurst::ADemo0502_SpawnRateBurst()
{
	DemoId = TEXT("0502");
	Controls = TEXT("Space : 파편 Spawn Burst 다시 터뜨리기   [ / ] : 연기 Spawn Rate -10 / +10\n")
		TEXT("관찰: 왼쪽 연기는 초당 50개씩 끊임없이 나오고(Spawn Rate),\n")
		TEXT("오른쪽 파편은 한 프레임에 100개가 한꺼번에 나온 뒤 끝난다(Spawn Burst Instantaneous)");
	Showcase = TEXT("Spawn Rate 50 연기(계속) vs Spawn Burst Instantaneous 100 파편(한꺼번에), 2.5초마다 재폭발");
	PlayerPawnClass = nullptr;
}

void ADemo0502_SpawnRateBurst::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	SpawnBlock(World, FVector(0.f, -350.f, 0.f), FVector(160.f, 160.f, 30.f), Colors::Platform);
	SpawnBlock(World, FVector(0.f, 350.f, 0.f), FVector(160.f, 160.f, 30.f), Colors::Platform);

	// 왼쪽: Spawn Rate (EmitterUpdate 단계의 SpawnRate 모듈이 매 프레임 Rate×dt 만큼 스폰)
	Smoke = EffectsKit::AddNiagara(this, TEXT("NS_SmokeRate"), FVector(0.f, -350.f, 40.f));
	// 오른쪽: Spawn Burst Instantaneous (시작 순간 100개, 반복 없음)
	Debris = EffectsKit::AddNiagara(this, TEXT("NS_DebrisBurst"), FVector(0.f, 350.f, 60.f));

	SpawnLabel(World, TEXT("Spawn Rate 50 /초 — 계속"), FVector(0.f, -350.f, 420.f), Colors::Cyan, 22.f);
	SpawnLabel(World, TEXT("Spawn Burst 100 — 한꺼번에"), FVector(0.f, 350.f, 420.f), Colors::Accent, 22.f);
	EffectsKit::SetupFixedCamera(this, FVector(-1100.f, -420.f, 330.f), FVector(0.f, -260.f, 220.f), 65.f);
	if (Smoke)
	{
		Smoke->SetVariableFloat(TEXT("SmokeSpawnRate"), SpawnRate);
	}
	BurstCounter = 1;
}

void ADemo0502_SpawnRateBurst::FireBurst()
{
	if (Debris)
	{
		// ResetSystem = 시스템을 처음부터 다시 실행 → Spawn Burst 가 다시 한 번 터진다
		Debris->ResetSystem();
		++BurstCounter;
	}
	BurstTimer = 0.f;
}

void ADemo0502_SpawnRateBurst::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	BurstTimer += DeltaSeconds;
	if (WasKeyPressed(EKeys::SpaceBar) || BurstTimer >= BurstInterval)
	{
		FireBurst();
	}
	if (WasKeyPressed(EKeys::LeftBracket) || WasKeyPressed(EKeys::RightBracket))
	{
		SpawnRate = FMath::Clamp(SpawnRate + (WasKeyPressed(EKeys::RightBracket) ? 10.f : -10.f), 0.f, 300.f);
		if (Smoke)
		{
			Smoke->SetVariableFloat(TEXT("SmokeSpawnRate"), SpawnRate);
		}
	}
	SetStatus(FString::Printf(TEXT("연기 Spawn Rate %.0f /초 (계속) · 파편 Spawn Burst %d 개 × %d 회 (다음 %.1f초)"),
		SpawnRate, BurstCount, BurstCounter, FMath::Max(0.f, BurstInterval - BurstTimer)));
}
