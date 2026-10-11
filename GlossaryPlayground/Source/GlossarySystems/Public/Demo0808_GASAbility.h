#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0808_GASAbility.generated.h"

class AD08_GASDummy;
class UD08_SidePanel;

/**
 * 0808 Gameplay Ability System (Ability)
 * 플레이어(ASC + Health/Mana AttributeSet)에게 파이어볼 어빌리티를 주고 1 로 발동한다.
 * 마나 20 소모(Cost GE), 쿨타임 2초(Cooldown GE + Ability.Cooldown.Fireball 태그)를 GAS 가 자동으로 검사한다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API ADemo0808_GASAbility : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0808_GASAbility();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 마나 재생 GE (1초마다 +5) 를 켤지 */
	UPROPERTY(EditAnywhere, Category = "GAS") bool bManaRegen = true;

private:
	void TryCast();

	UPROPERTY(Transient) TObjectPtr<AD08_GASDummy> Target;
	UPROPERTY(Transient) TObjectPtr<UD08_SidePanel> Panel;
	FString LastResult;
	int32 CastCount = 0;
	int32 FailCount = 0;
};
