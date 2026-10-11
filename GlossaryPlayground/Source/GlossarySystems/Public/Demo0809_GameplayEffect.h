#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0809_GameplayEffect.generated.h"

class AD08_GASDummy;
class UD08_SidePanel;

/**
 * 0809 Attribute Set / Gameplay Effect
 * Health·Mana AttributeSet 을 가진 표적에 독 GE(Duration 5초 · Period 1초 · 체력 -10)를 건다.
 * 1초마다 체력이 10씩 깎이고, 걸려 있는 동안 State.Poisoned 태그가 붙는다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API ADemo0809_GameplayEffect : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0809_GameplayEffect();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 시작 후 몇 초 뒤 자동으로 독을 걸지 (0 이하면 안 건다) */
	UPROPERTY(EditAnywhere, Category = "Gameplay Effect") float AutoPoisonDelay = 1.f;

private:
	void ApplyPoison(AD08_GASDummy* Target);

	UPROPERTY(Transient) TArray<TObjectPtr<AD08_GASDummy>> Targets;
	UPROPERTY(Transient) TObjectPtr<UD08_SidePanel> Panel;
	float AutoTimer = 0.f;
};
