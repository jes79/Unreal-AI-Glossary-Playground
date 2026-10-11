#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GlossaryDemoBase.h"
#include "Demo0807_GameplayTags.generated.h"

class AD08_Dummy;
class UD08_SidePanel;
class UStaticMeshComponent;

/**
 * 0807 Gameplay Tags
 * 적의 상태를 bool 여러 개 대신 FGameplayTagContainer 하나(State.Stunned, State.Burning …)로 관리하고
 * HasTag 로 행동을 정한다. 부모 태그 State 로 "아무 상태 이상이나 있나"도 한 줄로 검사한다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API ADemo0807_GameplayTags : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0807_GameplayTags();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 적의 현재 상태 — bool bStunned, bBurning … 대신 이것 하나 */
	UPROPERTY(VisibleAnywhere, Category = "Gameplay Tags") FGameplayTagContainer StateTags;

	UPROPERTY(EditAnywhere, Category = "Gameplay Tags") float PatrolSpeed = 250.f;
	UPROPERTY(EditAnywhere, Category = "Gameplay Tags") float BurnDps = 8.f;
	UPROPERTY(EditAnywhere, Category = "Gameplay Tags") float PoisonDps = 3.f;

private:
	void Toggle(const FGameplayTag& Tag);

	FGameplayTag TagState;     // 부모 태그 "State"
	FGameplayTag TagStunned;
	FGameplayTag TagBurning;
	FGameplayTag TagPoisoned;
	FGameplayTag TagInvincible;

	UPROPERTY(Transient) TObjectPtr<AD08_Dummy> Enemy;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Stars;
	UPROPERTY(Transient) TObjectPtr<UD08_SidePanel> Panel;
	float PatrolPhase = 0.f;
	float DotAccum = 0.f;
	float Time = 0.f;
};
