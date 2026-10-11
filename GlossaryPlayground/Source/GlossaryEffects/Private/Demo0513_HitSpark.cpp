#include "Demo0513_HitSpark.h"

#include "DrawDebugHelpers.h"
#include "EffectsDemoKit.h"
#include "Engine/World.h"
#include "GlossaryKit.h"
#include "NiagaraFunctionLibrary.h"

using namespace GlossaryKit;

ADemo0513_HitSpark::ADemo0513_HitSpark()
{
	DemoId = TEXT("0513");
	Controls = TEXT("Space : 한 발 더 쏘기 (자동으로 0.25초마다 발사)\n")
		TEXT("관찰: Line Trace 가 맞은 지점(Impact Point)에서 표면 노멀(초록 화살표) 방향으로 불꽃이 튄다.\n")
		TEXT("기울어진 판·구·벽 어디에 맞든 불꽃은 항상 표면 바깥쪽을 향한다");
	Showcase = TEXT("Line Trace 히트 지점에 표면 노멀 방향으로 불꽃 SpawnSystemAtLocation, 기울어진 판·구·벽 비교");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Night;
	Environment.ExposureEV100 = 12.f;   // 환경 노출 보정 = EV100-10 이라 값이 클수록 밝다 (밤 장면을 조금 밝게)
}

void ADemo0513_HitSpark::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	// 맞을 대상들: 정면 벽, 45° 기울어진 판, 구
	SpawnBlock(World, FVector(700.f, 0.f, 0.f), FVector(40.f, 1400.f, 500.f), Colors::Wall);
	SpawnShape(World, EShape::Cube, FVector(450.f, -350.f, 200.f), FVector(20.f, 300.f, 300.f), Colors::Platform, FRotator(0.f, 45.f, 0.f));
	SpawnShape(World, EShape::Sphere, FVector(450.f, 350.f, 180.f), FVector(220.f), Colors::Platform);
	SpawnShape(World, EShape::Cube, FVector(450.f, 0.f, 100.f), FVector(150.f, 150.f, 20.f), Colors::Platform, FRotator(35.f, 0.f, 0.f));

	Muzzle = FVector(-300.f, 0.f, 220.f);
	SpawnShape(World, EShape::Cube, Muzzle, FVector(60.f, 40.f, 40.f), Colors::Dark);
	EffectsKit::SetupFixedCamera(this, FVector(-650.f, -500.f, 450.f), FVector(400.f, 0.f, 180.f), 70.f);
}

void ADemo0513_HitSpark::FireTrace()
{
	UWorld* World = GetWorld();
	// 대상 쪽으로 무작위 방향 발사
	const FVector Target(600.f, FMath::FRandRange(-600.f, 600.f), FMath::FRandRange(30.f, 420.f));
	const FVector End = Muzzle + (Target - Muzzle).GetSafeNormal() * 2000.f;
	FHitResult Hit;
	FCollisionQueryParams Params(TEXT("HitSpark"), false, this);
	if (World->LineTraceSingleByChannel(Hit, Muzzle + FVector(40.f, 0.f, 0.f), End, ECC_Visibility, Params))
	{
		// 핵심: 맞은 위치 + 노멀 방향 회전으로 스폰 (시스템의 로컬 +X 가 불꽃이 튀는 방향)
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, EffectsKit::LoadSystem(TEXT("NS_HitSpark")), Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
		DrawDebugLine(World, Muzzle, Hit.ImpactPoint, FColor(255, 200, 80), false, 0.12f, 0, 1.5f);
		DrawDebugDirectionalArrow(World, Hit.ImpactPoint, Hit.ImpactPoint + Hit.ImpactNormal * 90.f, 20.f, FColor::Green, false, 0.6f, 0, 2.f);
		++Hits;
		LastHit = FString::Printf(TEXT("노멀 (%.2f, %.2f, %.2f)"), Hit.ImpactNormal.X, Hit.ImpactNormal.Y, Hit.ImpactNormal.Z);
	}
}

void ADemo0513_HitSpark::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Timer += DeltaSeconds;
	if (Timer >= FireInterval || WasKeyPressed(EKeys::SpaceBar))
	{
		Timer = 0.f;
		FireTrace();
	}
	SetStatus(FString::Printf(TEXT("히트 %d · 마지막 %s · SpawnSystemAtLocation(Location=ImpactPoint, Rotation=ImpactNormal.Rotation())"), Hits, *LastHit));
}
