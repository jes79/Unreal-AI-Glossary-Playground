#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GlossaryDemoBase.h"
#include "GlossaryGameMode.h"
#include "Demo0801_GameFramework.generated.h"

class UD08_SidePanel;

/** GameState: 모든 플레이어가 같이 보는 '공유 상태' (남은 시간, 경기 단계). 멀티플레이에서는 클라이언트에도 복제된다. */
UCLASS()
class GLOSSARYSYSTEMS_API AD0801_GameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	/** 남은 시간(초). GameMode 가 줄이고, UI 는 이 값만 읽는다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Match")
	float RemainingTime = 0.f;

	/** 진행 중 / 승리 / 시간 초과 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Match")
	FString MatchPhase = TEXT("진행 중");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Match")
	bool bMatchOver = false;
};

/** PlayerState: 플레이어 한 명의 상태 (점수). 폰이 죽어도 남고, 다른 플레이어도 볼 수 있다. */
UCLASS()
class GLOSSARYSYSTEMS_API AD0801_PlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	void AddCoin() { SetScore(GetScore() + 1.f); }
	int32 GetCoins() const { return FMath::RoundToInt(GetScore()); }
};

/**
 * GameMode: 서버에만 있는 '규칙' (제한 시간, 승리 조건, 리스폰).
 * AGlossaryGameMode 를 상속해 플레이어 컨트롤러·폰 결정 방식은 그대로 쓰고, GameState·PlayerState 클래스만 바꾼다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API AD0801_GameMode : public AGlossaryGameMode
{
	GENERATED_BODY()

public:
	AD0801_GameMode();

	/** 제한 시간 (초) — 규칙이므로 GameMode 에 */
	UPROPERTY(EditAnywhere, Category = "Rules", meta = (ClampMin = 1))
	float TimeLimit = 60.f;

	/** 승리 조건: 코인 수 — 규칙이므로 GameMode 에 */
	UPROPERTY(EditAnywhere, Category = "Rules", meta = (ClampMin = 1))
	int32 CoinsToWin = 10;

	void OnCoinCollected(APlayerController* Collector);
	void RestartMatch();

protected:
	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	void EndMatch(bool bWon);
};

/**
 * 0801 GameMode · GameState · PlayerState
 * 코인 10개를 60초 안에 모으는 미니 경기. 규칙(GameMode) / 공유 상태(GameState) / 개인 점수(PlayerState)를 나눠 담는다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API ADemo0801_GameFramework : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0801_GameFramework();

#if WITH_EDITOR
	virtual void BuildLevel(UWorld* World) override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Game Framework") int32 CoinCount = 14;
	UPROPERTY(EditAnywhere, Category = "Game Framework") float PickupRadius = 110.f;

private:
	void SpawnCoins();
	void Collect(AActor* Coin);

	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Coins;
	UPROPERTY(Transient) TObjectPtr<UD08_SidePanel> Panel;
};
