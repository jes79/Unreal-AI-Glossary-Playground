#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0206_PhysicalMaterial.generated.h"

class ACameraActor;
class AStaticMeshActor;
class UPhysicalMaterial;

/**
 * 0206 Physical Material
 * 생성 에셋 PM_Ice(Friction 0.05, Combine=Min) 와 PM_Rubber(Restitution 0.9, Combine=Max) 를
 * 기본 재질과 나란히 비교한다: 경사로에서 미끄러지는 상자, 같은 높이에서 떨어지는 공.
 */
UCLASS()
class GLOSSARYPHYSICS_API ADemo0206_PhysicalMaterial : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0206_PhysicalMaterial();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Physical Material")
	TObjectPtr<UPhysicalMaterial> IceMaterial;

	UPROPERTY(EditAnywhere, Category = "Physical Material")
	TObjectPtr<UPhysicalMaterial> RubberMaterial;

	UPROPERTY(EditAnywhere, Category = "Physical Material")
	TObjectPtr<UPhysicalMaterial> DefaultMaterial;

	/** 경사로 각도 */
	UPROPERTY(EditAnywhere, Category = "Physical Material")
	float RampAngle = 20.f;

	/** 이 시간마다 자동으로 다시 시작 */
	UPROPERTY(EditAnywhere, Category = "Physical Material")
	float ResetInterval = 7.f;

private:
	void ResetObjects();

	UPROPERTY() TObjectPtr<ACameraActor> Camera;
	UPROPERTY() TArray<TObjectPtr<AStaticMeshActor>> Sliders;  // 0 기본, 1 얼음
	UPROPERTY() TArray<TObjectPtr<AStaticMeshActor>> Balls;    // 0 기본, 1 고무
	TArray<FTransform> SliderStarts;
	TArray<FVector> BallStarts;
	TArray<float> MaxBounce;
	TArray<int32> BounceCount;
	TArray<bool> bWasFalling;
	float Timer = 0.f;
};
