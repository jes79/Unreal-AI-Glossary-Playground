#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0507_ParticleCollision.generated.h"

class UNiagaraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APostProcessVolume;

/**
 * 0507 Particle Collision — 불똥이 바닥에 닿으면 Collision 모듈(Restitution 0.5)로 튕긴다. 왼쪽은 충돌 없음(바닥을 뚫음).
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0507_ParticleCollision : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0507_ParticleCollision();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 반발 계수 (빌더에서 Collision 모듈 Restitution 에 설정) */
	UPROPERTY(VisibleAnywhere, Category = "Collision") float Restitution = 0.5f;

private:
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> WithCollision;
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> WithoutCollision;
};
