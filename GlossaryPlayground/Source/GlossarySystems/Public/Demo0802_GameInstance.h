#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Demo0802_GameInstance.generated.h"

class AGlossaryLabel;
class UD08_SidePanel;

/**
 * GameInstance 와 같은 수명(게임 실행 ~ 종료)을 갖는 저장소.
 * 레벨을 열어도(OpenLevel) 월드의 액터는 모두 사라지지만 GameInstance 와 그 서브시스템은 남는다.
 * 실제 프로젝트라면 UGameInstance 를 상속한 클래스에 Coins 를 두고 Project Settings > Maps & Modes > Game Instance Class 로 지정한다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API UD0802_CoinKeeper : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** 스테이지를 넘어가도 유지되는 코인 수 */
	UPROPERTY(VisibleAnywhere, Category = "GameInstance") int32 Coins = 0;
	/** 몇 번째 스테이지인지 (레벨을 다시 열 때마다 +1) */
	UPROPERTY(VisibleAnywhere, Category = "GameInstance") int32 Stage = 1;
	/** 이 게임 실행에서 0802 레벨이 열린 횟수 */
	UPROPERTY(VisibleAnywhere, Category = "GameInstance") int32 LevelLoads = 0;
	/** 게임 시작 후 지난 실제 시간 기준점 */
	double CreatedAt = 0.0;
};

/**
 * 0802 GameInstance
 * 코인을 모은 뒤 N 으로 '다음 스테이지'(같은 레벨 다시 열기)로 넘어가면,
 * 액터 변수는 0 으로 돌아가고 GameInstance 쪽 코인은 그대로 남는 것을 나란히 보여 준다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API ADemo0802_GameInstance : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0802_GameInstance();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 비교용: 레벨 액터의 변수 — 레벨을 다시 열면 0 */
	UPROPERTY(VisibleAnywhere, Category = "GameInstance") int32 ActorCoins = 0;
	UPROPERTY(EditAnywhere, Category = "GameInstance") int32 CoinsPerStage = 8;
	UPROPERTY(EditAnywhere, Category = "GameInstance") float PickupRadius = 110.f;

private:
	UD0802_CoinKeeper* GetKeeper() const;
	void Collect(AActor* Coin);
	void GoToNextStage();

	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Coins;
	UPROPERTY(Transient) TObjectPtr<UD08_SidePanel> Panel;
};
