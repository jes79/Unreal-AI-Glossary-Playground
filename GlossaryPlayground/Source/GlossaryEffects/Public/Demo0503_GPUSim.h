#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0503_GPUSim.generated.h"

class UNiagaraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APostProcessVolume;

/**
 * 0503 GPU Sim vs CPU Sim — 같은 10만 개 이미터를 GPU Compute Sim(Fixed Bounds) 과 CPU Sim 으로 바꿔 가며 프레임 시간을 비교.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0503_GPUSim : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0503_GPUSim();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 살아 있는 입자 목표 수 (Spawn Rate × Lifetime) */
	UPROPERTY(VisibleAnywhere, Category = "GPU Sim") int32 ParticleCount = 100000;
	/** GPU 이미터의 Fixed Bounds (빌더에서 설정한 값과 같다 — 화면에 상자로 그린다) */
	UPROPERTY(VisibleAnywhere, Category = "GPU Sim") FVector FixedBoundsExtent = FVector(450.f, 450.f, 450.f);

private:
	void ApplyMode();
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> GPUSystem;
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> CPUSystem;
	bool bUseGPU = true;
	bool bShowBounds = true;
	float SmoothedMs = 0.f;
};
