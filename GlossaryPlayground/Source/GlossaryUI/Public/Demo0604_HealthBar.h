#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "Demo0604_HealthBar.generated.h"

class UProgressBar;
class UTextBlock;

/** 빨간 바(즉시) 뒤에 흰 '고스트' 바(지연 후 FInterpTo)를 겹친 체력바 */
UCLASS()
class GLOSSARYUI_API UDemo0604_DelayedHealthBar : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	/** 피격: 빨간 바는 바로 줄고, 흰 바는 GhostDelay 뒤부터 따라온다 */
	void ApplyHealth(float NewPercent);

	/** 피격 뒤 흰 바가 기다리는 시간 (초) */
	float GhostDelay = 0.5f;
	/** FInterpTo 속도 (클수록 빨리 따라옴) */
	float GhostInterpSpeed = 4.f;
	/** false 면 비교용: 흰 바 없이 즉시 감소만 */
	bool bDelayedGhost = true;

	float GetFrontPercent() const { return FrontPercent; }
	float GetGhostPercent() const { return GhostPercent; }

	UPROPERTY(Transient) TObjectPtr<UTextBlock> Caption;

protected:
	virtual void BuildTree() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(Transient) TObjectPtr<UProgressBar> GhostBar;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> FrontBar;

	float FrontPercent = 1.f;
	float GhostPercent = 1.f;
	float DelayRemaining = 0.f;
};

/**
 * 0604 체력바 (지연 감소)
 * 피격 시 빨간 바는 즉시, 뒤쪽 흰 바는 0.5초 뒤 FInterpTo 로 따라 줄어든다.
 */
UCLASS()
class GLOSSARYUI_API ADemo0604_HealthBar : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0604_HealthBar();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "Health Bar") float MaxHealth = 100.f;
	UPROPERTY(EditAnywhere, Category = "Health Bar") float HitDamage = 15.f;
	/** 흰 바가 기다리는 시간 */
	UPROPERTY(EditAnywhere, Category = "Health Bar", meta = (ClampMin = 0)) float GhostDelay = 0.5f;
	/** FInterpTo 속도 */
	UPROPERTY(EditAnywhere, Category = "Health Bar", meta = (ClampMin = 0)) float GhostInterpSpeed = 4.f;
	/** 이 간격마다 자동 피격 (스크린샷·관찰용) */
	UPROPERTY(EditAnywhere, Category = "Health Bar") float AutoHitInterval = 1.5f;

	UPROPERTY(Transient) TObjectPtr<UDemo0604_DelayedHealthBar> Bar;

private:
	void Hit();
	float Health = 100.f;
	float HitTimer = 0.f;
	UPROPERTY(Transient) TObjectPtr<class AStaticMeshActor> Dummy;
	float FlashTime = 0.f;
};
