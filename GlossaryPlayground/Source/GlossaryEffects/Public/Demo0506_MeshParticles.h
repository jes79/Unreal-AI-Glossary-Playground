#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0506_MeshParticles.generated.h"

class UNiagaraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APostProcessVolume;

/**
 * 0506 Mesh Renderer — 파편 입자를 스프라이트 대신 큐브 메시로 그리고, 회전하며 떨어지게 한다.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0506_MeshParticles : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0506_MeshParticles();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Mesh Renderer") float BurstInterval = 2.5f;

private:
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> MeshDebris;
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> SpriteDebris;
	float Timer = 0.f;
};
