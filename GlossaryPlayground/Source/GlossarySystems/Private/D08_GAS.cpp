#include "D08_GAS.h"
#include "D08_Tags.h"

#include "Components/StaticMeshComponent.h"
#include "D08_Common.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectExtension.h"
#include "GlossaryKit.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace GlossaryKit;

FGameplayTag D08GAS::CooldownFireballTag()
{
	return D08Tags::Ability_Cooldown_Fireball;
}

FGameplayTag D08GAS::PoisonedTag()
{
	return D08Tags::State_Poisoned;
}

FActiveGameplayEffectHandle D08GAS::ApplyEffect(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target, TSubclassOf<UGameplayEffect> Effect)
{
	if (!Source || !Target)
	{
		return FActiveGameplayEffectHandle();
	}
	FGameplayEffectContextHandle Context = Source->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = Source->MakeOutgoingSpec(Effect, 1.f, Context);
	return Spec.IsValid() ? Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), Target) : FActiveGameplayEffectHandle();
}

// ---------------------------------------------------------------- AttributeSet

UD08_AttributeSet::UD08_AttributeSet()
{
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitMana(100.f);
	InitMaxMana(100.f);
}

void UD08_AttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());
	}
}

void UD08_AttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
	}
}

// ---------------------------------------------------------------- GE 들

namespace
{
	/** 속성 하나를 Amount 만큼 더하는 Modifier */
	FGameplayModifierInfo MakeAddModifier(const FGameplayAttribute& Attribute, float Amount)
	{
		FGameplayModifierInfo Info;
		Info.Attribute = Attribute;
		Info.ModifierOp = EGameplayModOp::Additive;
		Info.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Amount));
		return Info;
	}

	/** GE 가 대상에게 붙여 줄 태그 (5.3+ : UTargetTagsGameplayEffectComponent) */
	void GrantTargetTag(UGameplayEffect* Effect, UTargetTagsGameplayEffectComponent* Component, const FGameplayTag& Tag)
	{
		if (!Tag.IsValid())
		{
			return;
		}
		FInheritedTagContainer Tags;
		Tags.Added.AddTag(Tag);
		Component->SetAndApplyTargetTagChanges(Tags);
	}
}

UD0808_GE_FireballCost::UD0808_GE_FireballCost(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeAddModifier(UD08_AttributeSet::GetManaAttribute(), -20.f));
}

UD0808_GE_FireballCooldown::UD0808_GE_FireballCooldown(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(2.f));
	// 쿨타임의 정체 = "2초 동안 붙어 있는 태그". 어빌리티는 이 태그가 있으면 발동하지 않는다
	UTargetTagsGameplayEffectComponent* TagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("CooldownTags"));
	GEComponents.Add(TagsComponent);
	GrantTargetTag(this, TagsComponent, D08GAS::CooldownFireballTag());
}

UD0808_GE_FireballDamage::UD0808_GE_FireballDamage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeAddModifier(UD08_AttributeSet::GetHealthAttribute(), -25.f));
}

UD08_GE_ManaRegen::UD08_GE_ManaRegen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(1.f);
	Modifiers.Add(MakeAddModifier(UD08_AttributeSet::GetManaAttribute(), 5.f));
}

UD0809_GE_Poison::UD0809_GE_Poison(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Duration 5초 · Period 1초 → 1초마다 Modifier 가 '실행'되어 기본값(Base)을 깎는다
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.f));
	Period = FScalableFloat(1.f);
	bExecutePeriodicEffectOnApplication = false; // 적용 즉시가 아니라 1초 뒤부터
	Modifiers.Add(MakeAddModifier(UD08_AttributeSet::GetHealthAttribute(), -10.f));
	UTargetTagsGameplayEffectComponent* TagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("PoisonTags"));
	GEComponents.Add(TagsComponent);
	GrantTargetTag(this, TagsComponent, D08GAS::PoisonedTag());
}

UD0809_GE_Heal::UD0809_GE_Heal(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeAddModifier(UD08_AttributeSet::GetHealthAttribute(), 40.f));
}

// ---------------------------------------------------------------- Fireball

UD0808_GA_Fireball::UD0808_GA_Fireball(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// 비용·쿨타임은 GE 클래스로 지정 → CommitAbility 가 확인·적용을 모두 해 준다
	CostGameplayEffectClass = UD0808_GE_FireballCost::StaticClass();
	CooldownGameplayEffectClass = UD0808_GE_FireballCooldown::StaticClass();
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(D08Tags::Ability_Fireball);
	SetAssetTags(AssetTags);
}

void UD0808_GA_Fireball::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 비용(마나 20)과 쿨타임(2초)을 실제로 적용. 마나가 모자라거나 쿨타임 중이면 false
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* Avatar = ActorInfo->AvatarActor.Get();
	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();
	AD08_GASDummy* Target = Avatar ? D08::FindNearest<AD08_GASDummy>(Avatar->GetWorld(), Avatar->GetActorLocation()) : nullptr;
	if (Avatar && Target)
	{
		TWeakObjectPtr<UAbilitySystemComponent> WeakSource = SourceASC;
		AD08_Projectile::Launch(Avatar->GetWorld(), Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * 80.f + FVector(0, 0, 40.f),
			Target, FLinearColor(1.f, 0.35f, 0.05f), 45.f, 1400.f,
			[WeakSource](AActor* Hit)
			{
				// 명중: 대상 ASC 에 데미지 GE 적용
				if (IAbilitySystemInterface* AS = Cast<IAbilitySystemInterface>(Hit))
				{
					D08GAS::ApplyEffect(WeakSource.Get(), AS->GetAbilitySystemComponent(), UD0808_GE_FireballDamage::StaticClass());
				}
			});
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// ---------------------------------------------------------------- GAS 캐릭터

AD08_GASCharacter::AD08_GASCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	// 같은 액터의 서브오브젝트인 AttributeSet 은 ASC 가 초기화할 때 자동으로 등록된다
	Attributes = CreateDefaultSubobject<UD08_AttributeSet>(TEXT("Attributes"));
}

void AD08_GASCharacter::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystem->InitAbilityActorInfo(this, this); // Owner = Avatar = 이 캐릭터
}

// ---------------------------------------------------------------- GAS 표적

AD08_GASDummy::AD08_GASDummy()
{
	PrimaryActorTick.bCanEverTick = true;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	RootComponent = Body;
	InitShapeComponent(Body, EShape::Cylinder, FVector(90.f, 90.f, 180.f));
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	Attributes = CreateDefaultSubobject<UD08_AttributeSet>(TEXT("Attributes"));
}

void AD08_GASDummy::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystem->InitAbilityActorInfo(this, this);
	BodyMID = SetColor(Body, Colors::Enemy);
	Label = SpawnLabel(GetWorld(), TEXT(""), GetActorLocation() + FVector(0.f, 0.f, 160.f), FLinearColor::White, 18.f);
	Label->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	// 속성 변화 알림: 체력이 바뀔 때마다 호출 (UI 갱신의 표준 방식)
	AbilitySystem->GetGameplayAttributeValueChangeDelegate(UD08_AttributeSet::GetHealthAttribute()).AddUObject(this, &AD08_GASDummy::OnHealthChanged);
}

void AD08_GASDummy::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Label)
	{
		Label->Destroy();
	}
	Super::EndPlay(Reason);
}

void AD08_GASDummy::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	const float Delta = Data.NewValue - Data.OldValue;
	if (FMath::IsNearlyZero(Delta))
	{
		return;
	}
	const bool bPoisoned = AbilitySystem->HasMatchingGameplayTag(D08GAS::PoisonedTag());
	const FLinearColor Color = Delta > 0.f ? Colors::Cyan : (bPoisoned ? Colors::Ally : Colors::Accent);
	AD08_FloatingText::Spawn(GetWorld(), FString::Printf(TEXT("%+.0f"), Delta), GetActorLocation() + FVector(0, 0, 120), Color, 26.f);
	HealthLog.Add(FString::Printf(TEXT("t=%5.1f초  Health %.0f → %.0f"), GetWorld()->GetTimeSeconds(), Data.OldValue, Data.NewValue));
	if (HealthLog.Num() > 8)
	{
		HealthLog.RemoveAt(0);
	}
	if (BodyMID)
	{
		BodyMID->SetScalarParameterValue(TEXT("Emissive"), 3.f);
	}
	if (Data.NewValue <= 0.f)
	{
		DeadTimer = 2.f;
	}
}

void AD08_GASDummy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const bool bPoisoned = AbilitySystem->HasMatchingGameplayTag(D08GAS::PoisonedTag());
	if (BodyMID)
	{
		FLinearColor Color = bPoisoned ? Colors::Ally : Colors::Enemy;
		BodyMID->SetVectorParameterValue(TEXT("Color"), Color);
		float Glow = 0.f;
		BodyMID->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Emissive")), Glow);
		BodyMID->SetScalarParameterValue(TEXT("Emissive"), FMath::Max(0.f, Glow - DeltaSeconds * 12.f));
	}
	if (DeadTimer > 0.f)
	{
		DeadTimer -= DeltaSeconds;
		if (DeadTimer <= 0.f)
		{
			// 다시 채우기 (데모 반복용) — 기본값을 직접 설정
			AbilitySystem->SetNumericAttributeBase(UD08_AttributeSet::GetHealthAttribute(), Attributes->GetMaxHealth());
		}
	}
	if (Label)
	{
		Label->SetText(FString::Printf(TEXT("%s  HP %.0f / %.0f%s"), *DisplayName, Attributes->GetHealth(), Attributes->GetMaxHealth(),
			bPoisoned ? TEXT("  [State.Poisoned]") : TEXT("")));
	}
}
