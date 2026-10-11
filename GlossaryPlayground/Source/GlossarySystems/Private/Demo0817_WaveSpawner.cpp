#include "Demo0817_WaveSpawner.h"

#include "Components/StaticMeshComponent.h"
#include "D08_Common.h"
#include "Engine/World.h"
#include "GlossaryKit.h"
#include "Kismet/GameplayStatics.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- 적

AD0817_Enemy::AD0817_Enemy()
{
	PrimaryActorTick.bCanEverTick = true;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	RootComponent = Body;
	InitShapeComponent(Body, EShape::Cube, FVector(70.f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AD0817_Enemy::BeginPlay()
{
	Super::BeginPlay();
	SetColor(Body, Colors::Enemy, 0.5f, 0.f, 0.4f);
	SetActorScale3D(FVector(0.1f));
}

void AD0817_Enemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// 등장 연출: 0.3초 동안 커진다
	SpawnAnim = FMath::Min(1.f, SpawnAnim + DeltaSeconds / 0.3f);
	SetActorScale3D(FVector(FMath::Max(0.1f, SpawnAnim)));
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}
	FVector To = Player->GetActorLocation() - GetActorLocation();
	To.Z = 0.f;
	if (To.Size() > 150.f)
	{
		AddActorWorldOffset(To.GetSafeNormal() * MoveSpeed * DeltaSeconds);
		SetActorRotation(To.Rotation());
	}
}

// ---------------------------------------------------------------- 데모

ADemo0817_WaveSpawner::ADemo0817_WaveSpawner()
{
	DemoId = TEXT("0817");
	Controls = TEXT("WASD 이동 · 1 : 주변 공격(반경 450) · K : 모두 처치(테스트)\n")
		TEXT("관찰: 웨이브 1=4마리, 2=6, 3=9, 4=14 … (×1.5). 전멸시키면 5초 카운트다운 뒤 다음 웨이브");
	Showcase = TEXT("웨이브마다 적 수 ×1.5(4·6·9·14…), 전멸하면 5초 뒤 다음 웨이브 SpawnActor");
	EnemyClass = AD0817_Enemy::StaticClass();
}

void ADemo0817_WaveSpawner::BeginPlay()
{
	Super::BeginPlay();
	// 경기장 테두리 4곳에 스폰 지점
	const float R = 1400.f;
	for (int32 i = 0; i < 4; ++i)
	{
		const float A = PI * 0.5f * i + PI * 0.25f;
		const FVector P(FMath::Cos(A) * R, FMath::Sin(A) * R, 40.f);
		SpawnPoints.Add(P);
		SpawnBlock(GetWorld(), FVector(P.X, P.Y, 0.f), FVector(200.f, 200.f, 6.f), Colors::Purple);
		SpawnLabel(GetWorld(), FString::Printf(TEXT("스폰 %d"), i + 1), FVector(P.X, P.Y, 120.f), Colors::Purple, 16.f);
	}
	Panel = UD08_SidePanel::Create(GetPC(), TEXT("웨이브"), UD08_SidePanel::EPlace::TopRight, 420.f);
	StartWave();
}

int32 ADemo0817_WaveSpawner::CountForWave(int32 InWave) const
{
	// 웨이브 n 의 적 수 = Base × Multiplier^(n-1), 반올림
	return FMath::RoundToInt(BaseCount * FMath::Pow(CountMultiplier, float(InWave - 1)));
}

void ADemo0817_WaveSpawner::StartWave()
{
	++Wave;
	ToSpawn = CountForWave(Wave);
	SpawnTimer = 0.f;
	bWaitingNext = false;
	if (APawn* Pawn = GetPlayerPawn())
	{
		AD08_FloatingText::Spawn(GetWorld(), FString::Printf(TEXT("웨이브 %d — %d마리"), Wave, ToSpawn), Pawn->GetActorLocation() + FVector(0, 0, 160), Colors::Accent, 30.f, 2.f);
	}
}

void ADemo0817_WaveSpawner::SpawnOne()
{
	const FVector Point = SpawnPoints[(Wave + ToSpawn) % SpawnPoints.Num()] + FVector(FMath::FRandRange(-80.f, 80.f), FMath::FRandRange(-80.f, 80.f), 0.f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// 핵심: 클래스로 새 적 액터를 만든다
	AD0817_Enemy* Enemy = GetWorld()->SpawnActor<AD0817_Enemy>(EnemyClass, Point, FRotator::ZeroRotator, Params);
	if (Enemy)
	{
		Enemy->MoveSpeed = 140.f + Wave * 20.f;
		Alive.Add(Enemy);
	}
	--ToSpawn;
}

void ADemo0817_WaveSpawner::KillEnemy(AActor* Enemy)
{
	AD08_FloatingText::Spawn(GetWorld(), TEXT("처치"), Enemy->GetActorLocation() + FVector(0, 0, 60), FLinearColor::White, 18.f, 0.8f);
	Alive.Remove(Enemy);
	Enemy->Destroy();
	++Killed;
}

void ADemo0817_WaveSpawner::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Alive.RemoveAll([](const TObjectPtr<AActor>& A) { return !IsValid(A); });

	// 순서대로 한 마리씩 스폰
	if (ToSpawn > 0)
	{
		SpawnTimer -= DeltaSeconds;
		if (SpawnTimer <= 0.f)
		{
			SpawnOne();
			SpawnTimer = SpawnInterval;
		}
	}

	// 공격
	APawn* Pawn = GetPlayerPawn();
	if (Pawn && WasKeyPressed(EKeys::One))
	{
		for (int32 i = Alive.Num() - 1; i >= 0; --i)
		{
			if (FVector::Dist2D(Alive[i]->GetActorLocation(), Pawn->GetActorLocation()) <= AttackRadius)
			{
				KillEnemy(Alive[i]);
			}
		}
		AD08_FloatingText::Spawn(GetWorld(), TEXT("공격!"), Pawn->GetActorLocation() + FVector(0, 0, 120), Colors::Item, 22.f, 0.6f);
	}
	if (WasKeyPressed(EKeys::K))
	{
		for (int32 i = Alive.Num() - 1; i >= 0; --i)
		{
			KillEnemy(Alive[i]);
		}
	}

	// 전멸 판정 → 카운트다운 → 다음 웨이브
	if (!bWaitingNext && ToSpawn == 0 && Alive.Num() == 0)
	{
		bWaitingNext = true;
		Countdown = TimeBetweenWaves;
	}
	if (bWaitingNext)
	{
		Countdown -= DeltaSeconds;
		if (Countdown <= 0.f)
		{
			StartWave();
		}
	}

	const int32 Total = CountForWave(Wave);
	if (Panel)
	{
		Panel->SetTitle(bWaitingNext ? FString::Printf(TEXT("웨이브 %d 클리어! 다음까지 %.1f초"), Wave, FMath::Max(0.f, Countdown))
			: FString::Printf(TEXT("웨이브 %d — 남은 적 %d / %d"), Wave, Alive.Num() + ToSpawn, Total));
		Panel->SetProgress(bWaitingNext ? 1.f - Countdown / TimeBetweenWaves : 1.f - float(Alive.Num() + ToSpawn) / Total,
			bWaitingNext ? Colors::Item : Colors::Enemy);
		FString Table;
		for (int32 W = 1; W <= 5; ++W)
		{
			Table += FString::Printf(TEXT("%s웨이브 %d : %d마리\n"), W == Wave ? TEXT("▶ ") : TEXT("   "), W, CountForWave(W));
		}
		Panel->SetBody(FString::Printf(TEXT("적 수 = %d × %.1f^(웨이브-1)\n%s총 처치 %d"), BaseCount, CountMultiplier, *Table, Killed));
	}
	SetStatus(FString::Printf(TEXT("웨이브 %d · 살아 있는 적 %d · 스폰 대기 %d · 다음 웨이브 %d마리"), Wave, Alive.Num(), ToSpawn, CountForWave(Wave + 1)));
}
