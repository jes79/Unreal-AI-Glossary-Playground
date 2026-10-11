#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "Demo0803_Subsystem.generated.h"

class UD08_SidePanel;

USTRUCT(BlueprintType)
struct FD0803_Achievement
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName Id;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString Title;
	/** 어떤 통계가 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName Stat;
	/** 몇 이상이면 달성 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Goal = 1;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bUnlocked = false;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FD0803_OnAchievementUnlocked, const FD0803_Achievement&);

/**
 * 업적 관리자. 싱글톤 static 없이 엔진이 GameInstance 와 함께 자동으로 만들고 없앤다.
 * 어디서나: GetGameInstance()->GetSubsystem<UD0803_AchievementSubsystem>()
 * (레벨이 바뀌어도 유지 — 업적은 게임 전체 단위이므로 GameInstanceSubsystem)
 */
UCLASS()
class GLOSSARYSYSTEMS_API UD0803_AchievementSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 통계를 올리고, 목표에 닿은 업적을 해제한다 */
	void AddStat(FName Stat, int32 Delta = 1);
	int32 GetStat(FName Stat) const { return Stats.FindRef(Stat); }
	const TArray<FD0803_Achievement>& GetAchievements() const { return Achievements; }

	FD0803_OnAchievementUnlocked OnAchievementUnlocked;

private:
	UPROPERTY() TArray<FD0803_Achievement> Achievements;
	UPROPERTY() TMap<FName, int32> Stats;
};

/**
 * 월드(레벨) 단위 서브시스템: 레벨이 열릴 때 만들어지고 레벨과 함께 사라진다.
 * 이 레벨에서 주운 코인 수처럼 '이번 판' 데이터에 쓴다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API UD0803_LevelStatsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override
	{
		return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
	}
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	int32 CoinsThisLevel = 0;
	double CreatedAt = 0.0;
};

/**
 * 0803 Subsystem
 * 점프·코인 줍기를 업적 서브시스템(GameInstance 단위)과 레벨 통계 서브시스템(World 단위)에 보고한다.
 * N 으로 레벨을 다시 열면 업적은 남고 레벨 통계만 0 이 된다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API ADemo0803_Subsystem : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0803_Subsystem();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Subsystem") float PickupRadius = 110.f;
	UPROPERTY(EditAnywhere, Category = "Subsystem") float ToastTime = 3.f;

private:
	void HandleUnlocked(const FD0803_Achievement& Achievement);
	void Collect(AActor* Coin);

	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Coins;
	UPROPERTY(Transient) TObjectPtr<UD08_SidePanel> Panel;
	UPROPERTY(Transient) TObjectPtr<UD08_SidePanel> Toast;
	float ToastTimer = 0.f;
	FDelegateHandle UnlockHandle;
};
