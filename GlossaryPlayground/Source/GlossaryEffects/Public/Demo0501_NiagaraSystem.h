#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0501_NiagaraSystem.generated.h"

class UNiagaraComponent;
class UPointLightComponent;

/**
 * 0501 Niagara System / Emitter
 * 빌더가 만든 NS_Campfire = 이미터 3개(Flames·Embers·Smoke)를 묶은 Niagara System.
 * 1/2/3 키로 이미터를 하나씩 끄고 켜서 "시스템 = 이미터 묶음" 을 보여 준다(User 파라미터로 Spawn Rate 0).
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0501_NiagaraSystem : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0501_NiagaraSystem();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 이미터별 Spawn Rate (초당 입자 수). 시스템의 User.FlameRate / User.EmberRate / User.SmokeRate 에 연결돼 있다 */
	UPROPERTY(EditAnywhere, Category = "Niagara System") float FlameRate = 60.f;
	UPROPERTY(EditAnywhere, Category = "Niagara System") float EmberRate = 15.f;
	UPROPERTY(EditAnywhere, Category = "Niagara System") float SmokeRate = 8.f;

private:
	void ApplyRates();

	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> Campfire;
	UPROPERTY(Transient) TObjectPtr<UPointLightComponent> FireLight;
	bool bEnabled[3] = { true, true, true };
	float Time = 0.f;
};
