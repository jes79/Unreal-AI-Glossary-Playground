#include "Demo0801_GameFramework.h"

#include "D08_Common.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- GameMode

AD0801_GameMode::AD0801_GameMode()
{
	// 핵심: 이 게임모드가 쓸 GameState·PlayerState 클래스를 지정한다
	GameStateClass = AD0801_GameState::StaticClass();
	PlayerStateClass = AD0801_PlayerState::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
}

void AD0801_GameMode::StartPlay()
{
	Super::StartPlay();
	RestartMatch();
}

void AD0801_GameMode::RestartMatch()
{
	if (AD0801_GameState* GS = GetGameState<AD0801_GameState>())
	{
		GS->RemainingTime = TimeLimit;
		GS->MatchPhase = TEXT("진행 중");
		GS->bMatchOver = false;
	}
	// 모든 플레이어 점수 초기화 (PlayerState 는 GameState 의 PlayerArray 에 모여 있다)
	if (GameState)
	{
		for (APlayerState* PS : GameState->PlayerArray)
		{
			PS->SetScore(0.f);
		}
	}
}

void AD0801_GameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AD0801_GameState* GS = GetGameState<AD0801_GameState>();
	if (!GS || GS->bMatchOver)
	{
		return;
	}
	// 규칙: 시간이 흐르게 하는 것은 GameMode, 그 값을 담는 곳은 GameState
	GS->RemainingTime = FMath::Max(0.f, GS->RemainingTime - DeltaSeconds);
	if (GS->RemainingTime <= 0.f)
	{
		EndMatch(false);
	}
}

void AD0801_GameMode::OnCoinCollected(APlayerController* Collector)
{
	AD0801_GameState* GS = GetGameState<AD0801_GameState>();
	if (!Collector || !GS || GS->bMatchOver)
	{
		return;
	}
	AD0801_PlayerState* PS = Collector->GetPlayerState<AD0801_PlayerState>();
	if (!PS)
	{
		return;
	}
	PS->AddCoin(); // 점수는 플레이어별 → PlayerState
	if (PS->GetCoins() >= CoinsToWin)
	{
		EndMatch(true); // 승리 조건 판정은 규칙 → GameMode
	}
}

void AD0801_GameMode::EndMatch(bool bWon)
{
	if (AD0801_GameState* GS = GetGameState<AD0801_GameState>())
	{
		GS->bMatchOver = true;
		GS->MatchPhase = bWon ? TEXT("승리!") : TEXT("시간 초과 — 패배");
	}
}

// ---------------------------------------------------------------- Demo

ADemo0801_GameFramework::ADemo0801_GameFramework()
{
	DemoId = TEXT("0801");
	// 플레이어 컨트롤러(입력 처리) 다음에 Tick 해서 WasKeyPressed 를 놓치지 않게
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	Controls = TEXT("WASD 이동 · 노란 코인에 닿으면 획득 · C : 가장 가까운 코인 줍기(테스트) · R : 경기 다시 시작\n")
		TEXT("관찰: 60초·10개 규칙은 GameMode, 남은 시간은 GameState, 내 점수는 PlayerState 에 따로 들어 있다");
	Showcase = TEXT("60초 안에 코인 10개 모으기: 규칙=GameMode, 남은 시간=GameState, 점수=PlayerState 로 나눠 표시");
}

#if WITH_EDITOR
void ADemo0801_GameFramework::BuildLevel(UWorld* World)
{
	// 이 레벨만 전용 GameMode 를 쓰도록 World Settings > GameMode Override 를 지정한다
	World->GetWorldSettings()->DefaultGameMode = AD0801_GameMode::StaticClass();
}
#endif

void ADemo0801_GameFramework::BeginPlay()
{
	Super::BeginPlay();
	SpawnCoins();
	Panel = UD08_SidePanel::Create(GetPC(), TEXT("경기"), UD08_SidePanel::EPlace::TopRight, 440.f);
	SpawnLabel(GetWorld(), TEXT("코인 10개를 60초 안에!"), FVector(400.f, 0.f, 300.f), Colors::Item, 24.f);
}

void ADemo0801_GameFramework::SpawnCoins()
{
	for (AActor* Coin : Coins)
	{
		if (IsValid(Coin))
		{
			Coin->Destroy();
		}
	}
	Coins.Reset();
	for (int32 i = 0; i < CoinCount; ++i)
	{
		// 두 겹 원 위에 코인을 늘어놓는다
		const float Angle = 2.f * PI * i / CoinCount;
		const float Radius = (i % 2 == 0) ? 500.f : 900.f;
		const FVector Location(400.f + FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 70.f);
		AStaticMeshActor* Coin = SpawnShape(GetWorld(), EShape::Cylinder, Location, FVector(60.f, 60.f, 10.f), Colors::Item, FRotator(90.f, 0.f, 0.f));
		Coin->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetColor(Coin->GetStaticMeshComponent(), Colors::Item, 0.3f, 0.8f, 0.6f);
		Coins.Add(Coin);
	}
}

void ADemo0801_GameFramework::Collect(AActor* Coin)
{
	Coins.Remove(Coin);
	AD08_FloatingText::Spawn(GetWorld(), TEXT("+1"), Coin->GetActorLocation() + FVector(0, 0, 60), Colors::Item, 26.f);
	Coin->Destroy();
	// 데모(월드 액터)는 규칙을 직접 판단하지 않고 GameMode 에 알리기만 한다
	if (AD0801_GameMode* GM = GetWorld()->GetAuthGameMode<AD0801_GameMode>())
	{
		GM->OnCoinCollected(GetPC());
	}
}

void ADemo0801_GameFramework::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AD0801_GameMode* GM = GetWorld()->GetAuthGameMode<AD0801_GameMode>();
	AD0801_GameState* GS = GetWorld()->GetGameState<AD0801_GameState>();
	APlayerController* PC = GetPC();
	AD0801_PlayerState* PS = PC ? PC->GetPlayerState<AD0801_PlayerState>() : nullptr;
	if (!GM || !GS || !PS)
	{
		SetStatus(TEXT("이 레벨의 GameMode 가 AD0801_GameMode 가 아니다 (World Settings 확인)"));
		return;
	}

	if (WasKeyPressed(EKeys::R))
	{
		GM->RestartMatch();
		SpawnCoins();
	}

	// 코인 회전 + 획득 판정
	const float Spin = 180.f * DeltaSeconds;
	APawn* Pawn = GetPlayerPawn();
	AActor* ToCollect = nullptr;
	AActor* NearestCoin = nullptr;
	float Nearest = 1e9f;
	for (AActor* Coin : Coins)
	{
		if (!IsValid(Coin))
		{
			continue;
		}
		Coin->AddActorWorldRotation(FRotator(0.f, Spin, 0.f));
		if (Pawn)
		{
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
	}
	if (!ToCollect && WasKeyPressed(EKeys::C))
	{
		ToCollect = NearestCoin; // 테스트용: 걸어가지 않고 가장 가까운 코인을 줍는다
	}
	if (ToCollect && !GS->bMatchOver)
	{
		Collect(ToCollect);
	}

	const FString Rules = FString::Printf(TEXT("[GameMode · 규칙 · 서버에만]\n  제한 시간 %.0f초 · 승리 조건 코인 %d개"), GM->TimeLimit, GM->CoinsToWin);
	const FString Shared = FString::Printf(TEXT("[GameState · 모두가 보는 공유 상태]\n  남은 시간 %.1f초 · %s"), GS->RemainingTime, *GS->MatchPhase);
	const FString Mine = FString::Printf(TEXT("[PlayerState · 플레이어별]\n  내 점수(Score) %d"), PS->GetCoins());

	if (Panel)
	{
		Panel->SetTitle(FString::Printf(TEXT("남은 시간 %.0f초   코인 %d / %d"), GS->RemainingTime, PS->GetCoins(), GM->CoinsToWin));
		Panel->SetProgress(GS->RemainingTime / GM->TimeLimit, GS->RemainingTime < 10.f ? Colors::Enemy : Colors::Ally);
		Panel->SetBody(FString::Printf(TEXT("%s\n%s\n%s"), *Rules, *Shared, *Mine));
	}
	SetStatus(FString::Printf(TEXT("%s · 남은 시간 %.1f초 · 점수 %d · 남은 코인 %d"), *GS->MatchPhase, GS->RemainingTime, PS->GetCoins(), Coins.Num()));
}
