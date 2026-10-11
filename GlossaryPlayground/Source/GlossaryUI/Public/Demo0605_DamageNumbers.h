#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "Demo0605_DamageNumbers.generated.h"

class UCanvasPanel;
class UTextBlock;
class AStaticMeshActor;

/** 떠오르는 숫자 하나 */
USTRUCT()
struct FDemo0605FloatingNumber
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<UTextBlock> Text;
	FVector WorldLocation = FVector::ZeroVector;
	float Age = 0.f;
	bool bCritical = false;
};

/**
 * 화면 전체를 덮는 데미지 숫자 레이어. 숫자마다 월드 위치를 기억해 두고
 * 매 프레임 ProjectWorldLocationToWidgetPosition 으로 화면 좌표를 다시 구한다 (카메라가 움직여도 맞은 자리에 붙어 있다).
 */
UCLASS()
class GLOSSARYUI_API UDemo0605_DamageLayer : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	void SpawnNumber(const FVector& WorldLocation, int32 Damage, bool bCritical);

	float Lifetime = 1.0f;
	float RiseSpeed = 120.f;  // cm/s
	int32 GetAliveCount() const { return Numbers.Num(); }

protected:
	virtual void BuildTree() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY(Transient) TArray<FDemo0605FloatingNumber> Numbers;
};

/**
 * 0605 데미지 숫자 (Floating Text)
 * 적이 맞으면 머리 위에 숫자가 떠올랐다 사라지고, 치명타는 노란색으로 크게.
 */
UCLASS()
class GLOSSARYUI_API ADemo0605_DamageNumbers : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0605_DamageNumbers();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 치명타 확률 (0~1) */
	UPROPERTY(EditAnywhere, Category = "Damage Numbers", meta = (ClampMin = 0, ClampMax = 1)) float CriticalChance = 0.25f;
	/** 치명타 배율 */
	UPROPERTY(EditAnywhere, Category = "Damage Numbers") float CriticalMultiplier = 2.f;
	/** 숫자가 떠 있는 시간 */
	UPROPERTY(EditAnywhere, Category = "Damage Numbers") float NumberLifetime = 1.0f;
	/** 자동 공격 간격 (관찰용) */
	UPROPERTY(EditAnywhere, Category = "Damage Numbers") float AutoAttackInterval = 0.35f;

	UPROPERTY(Transient) TObjectPtr<UDemo0605_DamageLayer> Layer;
	UPROPERTY(Transient) TArray<TObjectPtr<AStaticMeshActor>> Enemies;

private:
	void HitEnemy(AStaticMeshActor* Enemy, bool bForceCrit = false);
	float AttackTimer = 0.f;
	bool bAutoAttack = true;
	int32 TotalHits = 0;
	int32 TotalCrits = 0;
};
