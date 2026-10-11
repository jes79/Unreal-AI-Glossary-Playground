#include "Demo0803_Subsystem.h"

#include "D08_Common.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GlossaryKit.h"
#include "Kismet/GameplayStatics.h"

using namespace GlossaryKit;

DEFINE_LOG_CATEGORY_STATIC(LogGlossary0803, Log, All);

namespace
{
	const FName StatJump(TEXT("Jump"));
	const FName StatCoin(TEXT("Coin"));
}

// ---------------------------------------------------------------- 업적 서브시스템

void UD0803_AchievementSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// 업적 정의 (실제 프로젝트라면 DataTable/DataAsset 에서 읽는다)
	auto Add = [this](const TCHAR* Id, const TCHAR* Title, FName Stat, int32 Goal)
	{
		FD0803_Achievement A;
		A.Id = Id;
		A.Title = Title;
		A.Stat = Stat;
		A.Goal = Goal;
		Achievements.Add(A);
	};
	Add(TEXT("FirstJump"), TEXT("첫 점프"), StatJump, 1);
	Add(TEXT("Jumper"), TEXT("점프 5번"), StatJump, 5);
	Add(TEXT("Collector"), TEXT("코인 3개"), StatCoin, 3);
	Add(TEXT("Rich"), TEXT("코인 10개 (레벨을 넘어 누적)"), StatCoin, 10);
	UE_LOG(LogGlossary0803, Display, TEXT("[0803] 업적 서브시스템 Initialize — 엔진이 GameInstance 와 함께 자동 생성"));
}

void UD0803_AchievementSubsystem::Deinitialize()
{
	OnAchievementUnlocked.Clear();
	Super::Deinitialize();
}

void UD0803_AchievementSubsystem::AddStat(FName Stat, int32 Delta)
{
	const int32 Value = (Stats.FindOrAdd(Stat) += Delta);
	for (FD0803_Achievement& A : Achievements)
	{
		if (!A.bUnlocked && A.Stat == Stat && Value >= A.Goal)
		{
			A.bUnlocked = true;
			OnAchievementUnlocked.Broadcast(A); // 누가 듣는지 몰라도 된다 (UI·사운드·플랫폼 업적 등)
		}
	}
}

// ---------------------------------------------------------------- 월드 서브시스템

void UD0803_LevelStatsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CreatedAt = FPlatformTime::Seconds();
}

// ---------------------------------------------------------------- 데모

ADemo0803_Subsystem::ADemo0803_Subsystem()
{
	DemoId = TEXT("0803");
	// 플레이어 컨트롤러(입력 처리) 다음에 Tick 해서 WasKeyPressed 를 놓치지 않게
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	Controls = TEXT("WASD 이동 · Space 점프 · 코인에 닿으면 획득 (C : 가장 가까운 코인 줍기)\n")
		TEXT("N : 레벨 다시 열기\n")
		TEXT("관찰: 업적(GameInstanceSubsystem)은 레벨을 넘어 남고, 레벨 통계(WorldSubsystem)는 새로 시작");
	Showcase = TEXT("점프·코인 업적을 GameInstanceSubsystem 으로, 레벨 코인 수를 WorldSubsystem 으로 관리 — 레벨을 다시 열면 차이");
}

void ADemo0803_Subsystem::BeginPlay()
{
	Super::BeginPlay();
	// 싱글톤 Get() 대신: 엔진이 만든 서브시스템을 GameInstance 에서 꺼낸다
	if (UD0803_AchievementSubsystem* Ach = GetGameInstance()->GetSubsystem<UD0803_AchievementSubsystem>())
	{
		UnlockHandle = Ach->OnAchievementUnlocked.AddUObject(this, &ADemo0803_Subsystem::HandleUnlocked);
	}
	for (int32 i = 0; i < 6; ++i)
	{
		const FVector Location(200.f + i * 180.f, (i % 2 ? 1.f : -1.f) * 250.f, 70.f);
		AStaticMeshActor* Coin = SpawnShape(GetWorld(), EShape::Cylinder, Location, FVector(60.f, 60.f, 10.f), Colors::Item, FRotator(90.f, 0.f, 0.f));
		Coin->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetColor(Coin->GetStaticMeshComponent(), Colors::Item, 0.3f, 0.8f, 0.6f);
		Coins.Add(Coin);
	}
	SpawnLabel(GetWorld(), TEXT("점프하고 코인을 모아 업적을 열어 보자"), FVector(600.f, 0.f, 300.f), Colors::Item, 22.f);

	Panel = UD08_SidePanel::Create(GetPC(), TEXT("업적 (GameInstanceSubsystem)"), UD08_SidePanel::EPlace::TopRight, 440.f);
	Toast = UD08_SidePanel::Create(GetPC(), TEXT(""), UD08_SidePanel::EPlace::BottomCenter, 420.f);
	Toast->SetVisibility(ESlateVisibility::Collapsed);
}

void ADemo0803_Subsystem::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UD0803_AchievementSubsystem* Ach = GI->GetSubsystem<UD0803_AchievementSubsystem>())
		{
			Ach->OnAchievementUnlocked.Remove(UnlockHandle);
		}
	}
	Super::EndPlay(Reason);
}

void ADemo0803_Subsystem::HandleUnlocked(const FD0803_Achievement& Achievement)
{
	if (Toast)
	{
		Toast->SetTitle(TEXT("업적 달성!"));
		Toast->SetBody(Achievement.Title);
		Toast->SetVisibility(ESlateVisibility::HitTestInvisible);
		ToastTimer = ToastTime;
	}
	if (APawn* Pawn = GetPlayerPawn())
	{
		AD08_FloatingText::Spawn(GetWorld(), FString::Printf(TEXT("업적: %s"), *Achievement.Title), Pawn->GetActorLocation() + FVector(0, 0, 140), Colors::Item, 26.f, 2.f);
	}
}

void ADemo0803_Subsystem::Collect(AActor* Coin)
{
	Coins.Remove(Coin);
	Coin->Destroy();
	GetWorld()->GetSubsystem<UD0803_LevelStatsSubsystem>()->CoinsThisLevel++;     // 월드 단위
	GetGameInstance()->GetSubsystem<UD0803_AchievementSubsystem>()->AddStat(StatCoin); // 게임 단위
}

void ADemo0803_Subsystem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UD0803_AchievementSubsystem* Ach = GetGameInstance()->GetSubsystem<UD0803_AchievementSubsystem>();
	UD0803_LevelStatsSubsystem* LevelStats = GetWorld()->GetSubsystem<UD0803_LevelStatsSubsystem>();
	if (!Ach || !LevelStats)
	{
		return;
	}

	// 점프: Space 를 눌렀을 때 실제로 뛰어올랐으면 보고
	if (WasKeyPressed(EKeys::SpaceBar))
	{
		Ach->AddStat(StatJump);
	}

	APawn* Pawn = GetPlayerPawn();
	AActor* ToCollect = nullptr;
	AActor* NearestCoin = nullptr;
	float Nearest = 1e9f;
	for (AActor* Coin : Coins)
	{
		if (!IsValid(Coin) || !Pawn)
		{
			continue;
		}
		Coin->AddActorWorldRotation(FRotator(0.f, 180.f * DeltaSeconds, 0.f));
		const float Dist = FVector::Dist2D(Coin->GetActorLocation(), Pawn->GetActorLocation());
		if (Dist < PickupRadius)
		{
			ToCollect = Coin;
		}
		if (Dist < Nearest)
		{
			Nearest = Dist;
			NearestCoin = Coin;
		}
	}
	if (!ToCollect && WasKeyPressed(EKeys::C))
	{
		ToCollect = NearestCoin;
	}
	if (ToCollect)
	{
		Collect(ToCollect);
	}
	if (WasKeyPressed(EKeys::N))
	{
		UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetPackage()->GetName()));
		return;
	}

	if (ToastTimer > 0.f)
	{
		ToastTimer -= DeltaSeconds;
		if (ToastTimer <= 0.f && Toast)
		{
			Toast->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	FString Body;
	for (const FD0803_Achievement& A : Ach->GetAchievements())
	{
		Body += FString::Printf(TEXT("%s %s  (%d/%d)\n"), A.bUnlocked ? TEXT("[달성]") : TEXT("[    ]"), *A.Title,
			FMath::Min(Ach->GetStat(A.Stat), A.Goal), A.Goal);
	}
	Body += FString::Printf(TEXT("\n[WorldSubsystem] 이 레벨에서 주운 코인 %d · 생성 후 %.0f초\n"),
		LevelStats->CoinsThisLevel, FPlatformTime::Seconds() - LevelStats->CreatedAt);
	Body += TEXT("GetGameInstance()->GetSubsystem<...>() / GetWorld()->GetSubsystem<...>()");
	if (Panel)
	{
		Panel->SetBody(Body);
	}
	SetStatus(FString::Printf(TEXT("점프 %d · 코인 누적 %d (GameInstance) · 이 레벨 코인 %d (World)"),
		Ach->GetStat(StatJump), Ach->GetStat(StatCoin), LevelStats->CoinsThisLevel));
}
