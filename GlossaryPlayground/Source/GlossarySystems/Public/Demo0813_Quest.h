#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GlossaryDemoBase.h"
#include "Demo0813_Quest.generated.h"

class UD08_SidePanel;

/** 퀘스트 정의 = 데이터. 새 퀘스트는 코드 수정 없이 에셋만 하나 더 만든다 */
UCLASS(BlueprintType)
class GLOSSARYSYSTEMS_API UD0813_QuestData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest") FString Title;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest", meta = (MultiLine = true)) FString Description;
	/** 처치 대상 종류 (적의 TypeId 와 비교) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest") FName TargetType = TEXT("Slime");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest", meta = (ClampMin = 1)) int32 RequiredCount = 5;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest") int32 RewardGold = 100;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest") FString RewardItem;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(TEXT("Quest"), GetFName()); }
};

/**
 * 0813 퀘스트 시스템
 * DA_0813_SlimeHunt('슬라임 5마리 처치')를 불러와 진행도를 HUD 에 표시하고, 완료하면 보상(골드)을 준다.
 * 박쥐는 TargetType 이 달라 진행도에 들어가지 않는다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API ADemo0813_Quest : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0813_Quest();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Quest") TSoftObjectPtr<UD0813_QuestData> Quest;
	UPROPERTY(EditAnywhere, Category = "Quest") float StompRadius = 130.f;

private:
	struct FMob
	{
		TWeakObjectPtr<AActor> Actor;
		FName TypeId;
		FVector Home;
		float Phase = 0.f;
	};
	void SpawnMobs();
	void Kill(int32 Index);
	void OnEnemyKilled(FName TypeId);
	void UpdateHud();

	UPROPERTY(Transient) TObjectPtr<UD0813_QuestData> LoadedQuest;
	UPROPERTY(Transient) TObjectPtr<UD08_SidePanel> Hud;
	TArray<FMob> Mobs;
	int32 Progress = 0;
	bool bCompleted = false;
	int32 Gold = 0;
	float Time = 0.f;
	FString LastEvent;
};
