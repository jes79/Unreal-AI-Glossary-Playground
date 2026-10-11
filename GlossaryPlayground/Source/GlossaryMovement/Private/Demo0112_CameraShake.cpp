#include "Demo0112_CameraShake.h"

#include "Camera/PlayerCameraManager.h"
#include "D0112_CameraShakes.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"

using namespace GlossaryKit;

ADemo0112_CameraShake::ADemo0112_CameraShake()
{
	DemoId = TEXT("0112");
	Controls = TEXT("WASD 이동 · 빨간 포탑이 1.5초마다 공을 쏜다(맞으면 흔들림)\n")
		TEXT("1 : 피격 흔들림(Perlin 0.2초) · 2 : 폭발 흔들림(Wave 1초) · C : 흔들림 ON/OFF\n")
		TEXT("관찰: 짧고 불규칙한 Perlin 은 '툭' 맞는 느낌, 긴 사인파 Wave 는 '쿵' 울리는 느낌");
	Showcase = TEXT("피격 시 PlayerCameraManager->StartCameraShake 로 Perlin Noise 0.2초 흔들림, Wave Oscillator 폭발 흔들림과 비교·ON/OFF");
	HitShake = UD0112_PerlinHitShake::StaticClass();
	ExplosionShake = UD0112_WaveExplosionShake::StaticClass();
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-400.f, 0.f, 100.f));
}

void ADemo0112_CameraShake::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	SpawnBlock(World, TurretPos, FVector(160.f, 160.f, 120.f), Colors::Dark);
	SpawnShape(World, EShape::Sphere, TurretPos + FVector(0.f, 0.f, 170.f), FVector(120.f), Colors::Enemy);
	SpawnLabel(World, TEXT("포탑 — 맞으면 Camera Shake"), TurretPos + FVector(0.f, 0.f, 330.f), Colors::Item, 18.f);

	// 폭발 위치 표시용 드럼통
	for (int32 i = 0; i < 3; ++i)
	{
		SpawnShape(World, EShape::Cylinder, FVector(300.f, 500.f + i * 120.f, 60.f), FVector(80.f, 80.f, 120.f), Colors::Accent);
	}
	SpawnLabel(World, TEXT("2 : 드럼통 폭발"), FVector(300.f, 620.f, 220.f), Colors::Accent, 16.f);
}

void ADemo0112_CameraShake::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Projectiles.Clear();
	Super::EndPlay(EndPlayReason);
}

void ADemo0112_CameraShake::PlayShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale, const FString& What)
{
	LastShake = What;
	LastShakeTime = GetWorld()->GetTimeSeconds();
	LastShakeDuration = ShakeClass == ExplosionShake ? 1.f : 0.2f;
	if (!bShakeEnabled)
	{
		LastShake += TEXT(" (흔들림 OFF — 재생 안 함)");
		return;
	}
	if (APlayerController* PC = GetPC())
	{
		// ★ 핵심 한 줄: 카메라 매니저에 흔들림 클래스를 넘긴다
		PC->PlayerCameraManager->StartCameraShake(ShakeClass, Scale);
	}
}

void ADemo0112_CameraShake::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APawn* Pawn = GetPlayerPawn();
	if (!Pawn)
	{
		return;
	}
	if (WasKeyPressed(EKeys::C)) { bShakeEnabled = !bShakeEnabled; }
	if (WasKeyPressed(EKeys::One)) { PlayShake(HitShake, HitShakeScale, TEXT("1: 피격 (Perlin Noise 0.2초)")); }
	if (WasKeyPressed(EKeys::Two))
	{
		PlayShake(ExplosionShake, 1.f, TEXT("2: 폭발 (Wave Oscillator 1초)"));
		if (AStaticMeshActor* Boom = SpawnShape(GetWorld(), EShape::Sphere, FVector(300.f, 620.f, 100.f), FVector(400.f), Colors::Accent))
		{
			Boom->SetActorEnableCollision(false);
			SetColor(Boom->GetStaticMeshComponent(), Colors::Accent, 0.5f, 0.f, 8.f);
			Boom->SetLifeSpan(0.25f);
		}
	}

	FireTimer += DeltaSeconds;
	if (FireTimer >= FireInterval)
	{
		FireTimer = 0.f;
		const FVector From = TurretPos + FVector(0.f, 0.f, 170.f);
		Projectiles.Fire(GetWorld(), From, (Pawn->GetActorLocation() - From).GetSafeNormal() * 1100.f, Colors::Enemy, 30.f, 3.f);
	}
	Projectiles.Tick(DeltaSeconds, Pawn, false, [this, Pawn](bool)
	{
		++HitCount;
		PlayShake(HitShake, HitShakeScale, TEXT("포탑 공 피격 (Perlin Noise 0.2초)"));
		if (AGlossaryLabel* L = SpawnLabel(GetWorld(), TEXT("피격!"), Pawn->GetActorLocation() + FVector(0.f, 0.f, 120.f), Colors::Enemy, 22.f))
		{
			L->SetLifeSpan(0.8f);
		}
	});

	const float Since = GetWorld()->GetTimeSeconds() - LastShakeTime;
	SetStatus(FString::Printf(TEXT("Camera Shake %s · 피격 %d회\n마지막: %s%s"),
		bShakeEnabled ? TEXT("ON") : TEXT("OFF"), HitCount,
		LastShake.IsEmpty() ? TEXT("-") : *LastShake,
		Since < LastShakeDuration ? *FString::Printf(TEXT(" · 흔들리는 중 %.2f / %.1f초"), Since, LastShakeDuration) : TEXT("")));
}
