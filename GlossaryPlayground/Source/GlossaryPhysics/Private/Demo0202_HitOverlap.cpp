#include "Demo0202_HitOverlap.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Character.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

namespace
{
	const float BallLaneY = 380.f;
	const float WallX = 700.f;
	const float BallStartX = -350.f;
}

ADemo0202_HitOverlap::ADemo0202_HitOverlap()
{
	DemoId = TEXT("0202");
	Controls = TEXT("WASD 이동 · 마우스 시점 — 앞으로 걸어 코인을 먹고 벽에 부딪혀 본다\n")
		TEXT("1 : 코인 Generate Overlap Events ON/OFF\n")
		TEXT("관찰: 벽(Block)은 OnComponentHit 로 빨갛게 깜빡, 코인(Overlap)은 OnComponentBeginOverlap 으로 +10 — 오른쪽 공도 같은 이벤트");
	Showcase = TEXT("벽 OnComponentHit(막힘) vs 코인 OnComponentBeginOverlap(통과+점수 10), Generate Overlap Events 끄기 비교");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-650.f, 0.f, 100.f));
}

void ADemo0202_HitOverlap::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 벽: 두 레인을 모두 막는 WorldStatic. Block → OnComponentHit
	AStaticMeshActor* Wall = SpawnShape(World, EShape::Cube, FVector(WallX, 190.f, 150.f), FVector(60.f, 1100.f, 300.f), Colors::Wall);
	UStaticMeshComponent* WallMesh = Wall->GetStaticMeshComponent();
	WallMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	WallMesh->SetNotifyRigidBodyCollision(true);
	WallMesh->OnComponentHit.AddDynamic(this, &ADemo0202_HitOverlap::OnWallHit);
	Walls.Add(Wall);

	// 공: 키네마틱으로 스윕 이동 → 코인은 Overlap, 벽은 Block
	Ball = SpawnShape(World, EShape::Sphere, FVector(BallStartX, BallLaneY, 56.f), FVector(100.f), Colors::Player);
	UStaticMeshComponent* BallMesh = Ball->GetStaticMeshComponent();
	BallMesh->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	BallMesh->SetGenerateOverlapEvents(true);

	SpawnCoins();
	SpawnLabel(World, TEXT("벽 = Block → OnComponentHit"), FVector(WallX, 190.f, 380.f), Colors::Enemy, 20.f);
	SpawnLabel(World, TEXT("코인 = Overlap → OnComponentBeginOverlap"), FVector(0.f, 0.f, 230.f), Colors::Item, 20.f);
	SpawnLabel(World, TEXT("자동 공"), FVector(BallStartX, BallLaneY, 180.f), FLinearColor::White, 18.f);
}

void ADemo0202_HitOverlap::SpawnCoins()
{
	UWorld* World = GetWorld();
	auto MakeCoin = [&](const FVector& Location)
	{
		AStaticMeshActor* Coin = SpawnShape(World, EShape::Cylinder, Location, FVector(70.f, 70.f, 12.f), Colors::Item, FRotator(90.f, 0.f, 0.f));
		SetColor(Coin->GetStaticMeshComponent(), Colors::Item, 0.3f, 1.f, 1.5f);
		UStaticMeshComponent* Mesh = Coin->GetStaticMeshComponent();
		// 겹침만 감지: OverlapAllDynamic 프리셋 + Generate Overlap Events
		Mesh->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
		Mesh->SetGenerateOverlapEvents(bGenerateOverlapEvents);
		Mesh->OnComponentBeginOverlap.AddDynamic(this, &ADemo0202_HitOverlap::OnCoinOverlap);
		Coins.Add(Coin);
	};
	for (int32 i = 0; i < 5; ++i)
	{
		MakeCoin(FVector(-350.f + i * 170.f, 0.f, 70.f));          // 플레이어 레인
		MakeCoin(FVector(-100.f + i * 150.f, BallLaneY, 50.f));      // 공 레인
	}
}

void ADemo0202_HitOverlap::ApplyOverlapSetting()
{
	for (AStaticMeshActor* Coin : Coins)
	{
		if (Coin)
		{
			Coin->GetStaticMeshComponent()->SetGenerateOverlapEvents(bGenerateOverlapEvents);
			SetColor(Coin->GetStaticMeshComponent(), bGenerateOverlapEvents ? Colors::Item : Colors::Neutral, 0.3f, 1.f, bGenerateOverlapEvents ? 1.5f : 0.f);
		}
	}
}

void ADemo0202_HitOverlap::ResetBall()
{
	Ball->SetActorLocation(FVector(BallStartX, BallLaneY, 56.f));
	// 먹은 코인 되살리기
	for (AStaticMeshActor* Coin : Coins)
	{
		if (Coin && Coin->IsHidden())
		{
			Coin->SetActorHiddenInGame(false);
			Coin->SetActorEnableCollision(true);
		}
	}
}

void ADemo0202_HitOverlap::OnWallHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 캐릭터는 벽에 미는 동안 매 프레임 Hit 이 오므로 0.3초에 한 번만 센다
	if (WallFlash > 0.f)
	{
		return;
	}
	++WallHits;
	WallFlash = 0.3f;
	SetColor(HitComp, Colors::Enemy, 0.5f, 0.f, 2.f);
	LastEvent = FString::Printf(TEXT("OnComponentHit ← %s"), OtherActor == Ball ? TEXT("공") : (Cast<ACharacter>(OtherActor) ? TEXT("플레이어") : *GetNameSafe(OtherActor)));
	if (OtherActor == Ball)
	{
		BallPause = 0.8f;   // 벽 앞에서 잠깐 멈췄다가 처음으로
	}
}

void ADemo0202_HitOverlap::OnCoinOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AActor* Coin = OverlappedComp ? OverlappedComp->GetOwner() : nullptr;
	if (!Coin || Coin->IsHidden())
	{
		return;
	}
	Score += CoinScore;
	Coin->SetActorHiddenInGame(true);
	Coin->SetActorEnableCollision(false);
	LastEvent = FString::Printf(TEXT("OnComponentBeginOverlap ← %s  (+%d)"), OtherActor == Ball ? TEXT("공") : TEXT("플레이어"), CoinScore);
}

void ADemo0202_HitOverlap::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (WasKeyPressed(EKeys::One))
	{
		bGenerateOverlapEvents = !bGenerateOverlapEvents;
		ApplyOverlapSetting();
	}

	// 공: 스윕 이동(bSweep=true) — 막히면 Hit, 겹치면 Overlap 이벤트가 엔진에서 발생
	if (BallPause > 0.f)
	{
		BallPause -= DeltaSeconds;
		if (BallPause <= 0.f)
		{
			ResetBall();
		}
	}
	else if (Ball)
	{
		FHitResult Hit;
		Ball->AddActorWorldOffset(FVector(BallSpeed * DeltaSeconds, 0.f, 0.f), true, &Hit);
		Ball->AddActorLocalRotation(FRotator(-BallSpeed * DeltaSeconds * 1.15f, 0.f, 0.f));
	}

	if (WallFlash > 0.f)
	{
		WallFlash -= DeltaSeconds;
		if (WallFlash <= 0.f)
		{
			SetColor(Walls[0]->GetStaticMeshComponent(), Colors::Wall);
		}
	}

	SetStatus(FString::Printf(TEXT("점수 %d · 벽 Hit %d회 · 코인 Generate Overlap Events %s\n최근 이벤트: %s"),
		Score, WallHits, bGenerateOverlapEvents ? TEXT("ON") : TEXT("OFF (겹쳐도 이벤트 없음)"),
		LastEvent.IsEmpty() ? TEXT("-") : *LastEvent));
}
