#include "Demo0108_Dash.h"

#include "D0108_DashCharacter.h"
#include "Engine/World.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"

using namespace GlossaryKit;

ADemo0108_Dash::ADemo0108_Dash()
{
	DemoId = TEXT("0108");
	Controls = TEXT("WASD 이동 · Shift(패드 B) 대시 · C : 무적 ON/OFF\n")
		TEXT("포탑이 쏘는 공에 맞기 직전 대시해 보세요\n")
		TEXT("관찰: 대시 0.2초 동안 공이 몸을 통과(회피), 쿨타임 1초 동안은 Shift 가 무시된다");
	Showcase = TEXT("Shift 로 LaunchCharacter 대시 0.2초 + 무적(공 통과) + 쿨타임 1초, 잔상·피격/회피 카운트");
	PlayerPawnClass = AD0108_DashCharacter::StaticClass();
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-500.f, 0.f, 100.f));
}

void ADemo0108_Dash::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 좌우 포탑 4개
	Turrets = { FVector(400.f, -1100.f, 0.f), FVector(400.f, 1100.f, 0.f), FVector(1100.f, -700.f, 0.f), FVector(1100.f, 700.f, 0.f) };
	for (const FVector& T : Turrets)
	{
		SpawnBlock(World, T, FVector(120.f, 120.f, 100.f), Colors::Dark);
		SpawnShape(World, EShape::Sphere, T + FVector(0.f, 0.f, 130.f), FVector(90.f), Colors::Enemy);
	}
	SpawnLabel(World, TEXT("포탑 — 공을 대시(무적)로 통과해 보세요"), FVector(400.f, 0.f, 300.f), Colors::Item, 18.f);

	// 바닥 대시 거리 눈금 (1m 간격)
	for (int32 i = 0; i <= 10; ++i)
	{
		SpawnBlock(World, FVector(-500.f + i * 100.f, -350.f, 0.f), FVector(4.f, 60.f, 3.f), FLinearColor::White);
	}
	SpawnLabel(World, TEXT("1m 눈금"), FVector(0.f, -350.f, 60.f), FLinearColor::White, 14.f);

	HeadLabel = SpawnLabel(World, TEXT(""), FVector::ZeroVector, Colors::Cyan, 16.f);
}

void ADemo0108_Dash::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Projectiles.Clear();
	Super::EndPlay(EndPlayReason);
}

void ADemo0108_Dash::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AD0108_DashCharacter* Character = Cast<AD0108_DashCharacter>(GetPlayerPawn());
	if (!Character)
	{
		return;
	}
	if (WasKeyPressed(EKeys::C))
	{
		Character->bInvincibilityEnabled = !Character->bInvincibilityEnabled;
	}

	// 포탑이 차례로 플레이어를 겨냥해 발사
	FireTimer += DeltaSeconds;
	if (FireTimer >= FireInterval && Turrets.Num() > 0)
	{
		FireTimer = 0.f;
		const FVector From = Turrets[TurretIndex++ % Turrets.Num()] + FVector(0.f, 0.f, 130.f);
		FVector Aim = Character->GetActorLocation() + FVector(0.f, 0.f, 10.f) - From;
		Projectiles.Fire(GetWorld(), From, Aim.GetSafeNormal() * ProjectileSpeed, Colors::Accent, 25.f, 4.f);
	}

	// 명중 판정 — 무적이면 통과(회피), 아니면 피격
	Projectiles.Tick(DeltaSeconds, Character, Character->IsInvincible(), [this, Character](bool bDodged)
	{
		if (bDodged) { ++Dodges; } else { ++Hits; }
		if (AGlossaryLabel* L = SpawnLabel(GetWorld(), bDodged ? TEXT("회피! (무적)") : TEXT("피격!"),
			Character->GetActorLocation() + FVector(0.f, 0.f, 80.f), bDodged ? Colors::Cyan : Colors::Enemy, 18.f))
		{
			L->SetLifeSpan(1.f);
		}
	});

	if (AGlossaryLabel* L = HeadLabel.Get())
	{
		L->SetActorLocation(Character->GetActorLocation() + FVector(0.f, 0.f, 130.f));
		const float Cool = Character->GetCooldownRemaining();
		L->SetText(Character->IsInvincible() ? FString(TEXT("무적")) : (Cool > 0.f ? FString::Printf(TEXT("쿨타임 %.2f"), Cool) : FString(TEXT("대시 가능"))));
		L->SetColor(Character->IsInvincible() ? Colors::Cyan : (Cool > 0.f ? Colors::Accent : Colors::Ally));
	}

	SetStatus(FString::Printf(TEXT("대시 %d회 · 쿨타임으로 무시 %d회 · 무적 %s\n피격 %d · 회피 %d · 쿨타임 남음 %.2f초%s"),
		Character->GetDashCount(), Character->GetBlockedByCooldown(), Character->bInvincibilityEnabled ? TEXT("ON") : TEXT("OFF"),
		Hits, Dodges, Character->GetCooldownRemaining(), Character->IsDashing() ? TEXT(" · 대시 중") : TEXT("")));
}
