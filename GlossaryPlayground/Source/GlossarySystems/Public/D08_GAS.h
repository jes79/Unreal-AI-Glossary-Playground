#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "AttributeSet.h"
#include "GameplayEffect.h"
#include "GlossaryCharacter.h"
#include "D08_GAS.generated.h"

class AGlossaryLabel;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

// 언리얼 문서의 표준 매크로: Get/Set/Init 접근자와 FGameplayAttribute 게터를 만든다
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/** 0808·0809 공용 Attribute Set: 체력·마나 */
UCLASS()
class GLOSSARYSYSTEMS_API UD08_AttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UD08_AttributeSet();

	UPROPERTY(BlueprintReadOnly, Category = "Attributes") FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UD08_AttributeSet, Health)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes") FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UD08_AttributeSet, MaxHealth)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes") FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UD08_AttributeSet, Mana)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes") FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UD08_AttributeSet, MaxMana)

	/** 현재값이 바뀌기 직전에 0~최대로 자른다 */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	/** Instant/Periodic GE 가 기본값을 바꾼 뒤에도 자른다 */
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
};

// ---------------------------------------------------------------- Gameplay Effects (C++ 로 설정한 GE 클래스)

/** 파이어볼 비용: 마나 -20 (Instant) */
UCLASS()
class GLOSSARYSYSTEMS_API UD0808_GE_FireballCost : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UD0808_GE_FireballCost(const FObjectInitializer& ObjectInitializer);
};

/** 파이어볼 쿨타임: 2초 동안 Ability.Cooldown.Fireball 태그를 붙인다 (HasDuration) */
UCLASS()
class GLOSSARYSYSTEMS_API UD0808_GE_FireballCooldown : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UD0808_GE_FireballCooldown(const FObjectInitializer& ObjectInitializer);
};

/** 파이어볼 명중: 체력 -25 (Instant) */
UCLASS()
class GLOSSARYSYSTEMS_API UD0808_GE_FireballDamage : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UD0808_GE_FireballDamage(const FObjectInitializer& ObjectInitializer);
};

/** 마나 재생: 1초마다 +5 (Infinite + Period) */
UCLASS()
class GLOSSARYSYSTEMS_API UD08_GE_ManaRegen : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UD08_GE_ManaRegen(const FObjectInitializer& ObjectInitializer);
};

/** 독: 5초 동안(Duration) 1초마다(Period 1) 체력 -10, State.Poisoned 태그 부여 */
UCLASS()
class GLOSSARYSYSTEMS_API UD0809_GE_Poison : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UD0809_GE_Poison(const FObjectInitializer& ObjectInitializer);
};

/** 회복: 체력 +40 (Instant) */
UCLASS()
class GLOSSARYSYSTEMS_API UD0809_GE_Heal : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UD0809_GE_Heal(const FObjectInitializer& ObjectInitializer);
};

// ---------------------------------------------------------------- Gameplay Ability

/**
 * 파이어볼: Commit(마나 20 소모 + 쿨타임 2초) 에 성공하면 가장 가까운 표적에 불덩이를 날린다.
 * 비용·쿨타임 판정은 GAS 가 CostGameplayEffectClass / CooldownGameplayEffectClass 로 자동 처리.
 */
UCLASS()
class GLOSSARYSYSTEMS_API UD0808_GA_Fireball : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UD0808_GA_Fireball(const FObjectInitializer& ObjectInitializer);

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

// ---------------------------------------------------------------- 액터

/** ASC + AttributeSet 을 가진 플레이어 캐릭터 */
UCLASS()
class GLOSSARYSYSTEMS_API AD08_GASCharacter : public AGlossaryCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AD08_GASCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }
	const UD08_AttributeSet* GetAttributes() const { return Attributes; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "GAS") TObjectPtr<UAbilitySystemComponent> AbilitySystem;
	UPROPERTY(VisibleAnywhere, Category = "GAS") TObjectPtr<UD08_AttributeSet> Attributes;
};

/** ASC + AttributeSet 을 가진 표적(원기둥). 체력이 바뀌면 숫자를 띄운다 */
UCLASS()
class GLOSSARYSYSTEMS_API AD08_GASDummy : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AD08_GASDummy();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }
	const UD08_AttributeSet* GetAttributes() const { return Attributes; }
	void SetDisplayName(const FString& InName) { DisplayName = InName; }

	/** 체력 변화 기록 (시각, 이전, 이후) — 0809 틱 표시용 */
	TArray<FString> HealthLog;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;
	void OnHealthChanged(const FOnAttributeChangeData& Data);

	UPROPERTY(VisibleAnywhere, Category = "GAS") TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere, Category = "GAS") TObjectPtr<UAbilitySystemComponent> AbilitySystem;
	UPROPERTY(VisibleAnywhere, Category = "GAS") TObjectPtr<UD08_AttributeSet> Attributes;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BodyMID;
	UPROPERTY(Transient) TObjectPtr<AGlossaryLabel> Label;

	FString DisplayName = TEXT("표적");
	float DeadTimer = 0.f;
};

namespace D08GAS
{
	GLOSSARYSYSTEMS_API FGameplayTag CooldownFireballTag();
	GLOSSARYSYSTEMS_API FGameplayTag PoisonedTag();
	/** Source 가 Target 에게 GE 를 적용 (MakeOutgoingSpec → ApplyGameplayEffectSpecToTarget) */
	GLOSSARYSYSTEMS_API FActiveGameplayEffectHandle ApplyEffect(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target, TSubclassOf<UGameplayEffect> Effect);
}
