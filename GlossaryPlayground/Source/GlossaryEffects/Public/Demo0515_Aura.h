#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0515_Aura.generated.h"

class UNiagaraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APostProcessVolume;

/**
 * 0515 Aura / Buff — 발밑 회전 마법진 + 위로 올라가는 입자 오라를 캐릭터에 붙이고 켜고 끈다.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0515_Aura : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0515_Aura();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Aura") float CircleRotateSpeed = 60.f;

private:
	void SetAura(bool bOn);
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> AuraFX;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> MagicCircle;
	bool bAuraOn = true;
	bool bAttached = false;
};
