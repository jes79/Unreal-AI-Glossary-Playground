#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0208_RadialForce.generated.h"

class ACameraActor;
class AStaticMeshActor;
class URadialForceComponent;

/**
 * 0208 Radial Force (폭발력)
 * 클릭 위치로 RadialForceComponent 를 옮기고 FireImpulse() — 반경 500 안의 물리 상자가 사방으로 날아간다.
 */
UCLASS()
class GLOSSARYPHYSICS_API ADemo0208_RadialForce : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0208_RadialForce();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 폭발 반경 (cm) */
	UPROPERTY(EditAnywhere, Category = "Radial Force")
	float Radius = 500.f;

	/** 충격 세기. bImpulseVelChange=true 라서 질량과 무관한 속도 변화(cm/s) */
	UPROPERTY(EditAnywhere, Category = "Radial Force")
	float ImpulseStrength = 1200.f;

	UPROPERTY(VisibleAnywhere, Category = "Radial Force")
	TObjectPtr<URadialForceComponent> RadialForce;

private:
	void SpawnBoxes();
	void Explode(const FVector& Location);

	UPROPERTY() TObjectPtr<ACameraActor> Camera;
	UPROPERTY() TArray<TObjectPtr<AStaticMeshActor>> Boxes;
	TArray<FVector> StartLocations;
	FVector LastBlast = FVector::ZeroVector;
	float BlastTime = 0.f;
	int32 Explosions = 0;
};
