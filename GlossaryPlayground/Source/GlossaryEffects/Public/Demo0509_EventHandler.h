#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0509_EventHandler.generated.h"

class UNiagaraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APostProcessVolume;

/**
 * 0509 Event Handler (Death Event) — 로켓 입자가 죽을 때 Generate Death Event 를 보내고, 폭죽 이미터의 Event Handler 가 그 위치에서 입자를 스폰한다.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0509_EventHandler : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0509_EventHandler();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> Fireworks;
	float Elapsed = 0.f;
};
