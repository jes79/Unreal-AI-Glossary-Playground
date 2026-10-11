#pragma once

#include "CoreMinimal.h"
#include "D08_Common.h"
#include "GameFramework/DamageType.h"
#include "GlossaryDemoBase.h"
#include "Demo0810_ApplyDamage.generated.h"

class UD08_SidePanel;

/** 데미지 종류 = UDamageType 서브클래스. 객체를 만들지 않고 클래스(의 CDO)에 담긴 값을 읽는다 */
UCLASS()
class GLOSSARYSYSTEMS_API UD0810_DamageType_Normal : public UDamageType
{
	GENERATED_BODY()
public:
	/** 받는 쪽이 곱하는 배율 */
	UPROPERTY(EditDefaultsOnly, Category = "Damage") float DamageMultiplier = 1.f;
	UPROPERTY(EditDefaultsOnly, Category = "Damage") FString DisplayName = TEXT("일반");
	UPROPERTY(EditDefaultsOnly, Category = "Damage") FLinearColor Color = FLinearColor::White;
};

/** 화염: ×2 */
UCLASS()
class GLOSSARYSYSTEMS_API UD0810_DamageType_Fire : public UD0810_DamageType_Normal
{
	GENERATED_BODY()
public:
	UD0810_DamageType_Fire()
	{
		DamageMultiplier = 2.f;
		DisplayName = TEXT("화염");
		Color = FLinearColor(1.f, 0.4f, 0.05f);
	}
};

/** 얼음: ×0.5 */
UCLASS()
class GLOSSARYSYSTEMS_API UD0810_DamageType_Ice : public UD0810_DamageType_Normal
{
	GENERATED_BODY()
public:
	UD0810_DamageType_Ice()
	{
		DamageMultiplier = 0.5f;
		DisplayName = TEXT("얼음");
		Color = FLinearColor(0.3f, 0.8f, 1.f);
	}
};

/** TakeDamage 를 오버라이드해 Damage Type 배율대로 체력을 깎는 적 */
UCLASS()
class GLOSSARYSYSTEMS_API AD0810_Enemy : public AD08_Dummy
{
	GENERATED_BODY()

public:
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	FString LastHit;
};

/**
 * 0810 데미지 처리 (ApplyDamage)
 * 1/2/3 으로 일반·화염·얼음 Damage Type 의 ApplyDamage(10) 를 보내면, 적의 TakeDamage 가 배율(×1·×2·×0.5)을 적용한다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API ADemo0810_ApplyDamage : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0810_ApplyDamage();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** ApplyDamage 에 넘기는 기본 데미지 */
	UPROPERTY(EditAnywhere, Category = "Damage") float BaseDamage = 10.f;

private:
	void Shoot(TSubclassOf<UDamageType> DamageType);

	UPROPERTY(Transient) TObjectPtr<AD0810_Enemy> Enemy;
	UPROPERTY(Transient) TObjectPtr<UD08_SidePanel> Panel;
	TArray<FString> Log;
};
