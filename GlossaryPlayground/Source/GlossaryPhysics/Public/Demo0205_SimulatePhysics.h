#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0205_SimulatePhysics.generated.h"

class ACameraActor;
class AStaticMeshActor;

/**
 * 0205 Simulate Physics / Impulse
 * 왼쪽 상자 더미는 Simulate Physics ON, 오른쪽은 OFF.
 * 클릭한 지점(HitResult.ImpactPoint)에 AddImpulseAtLocation 으로 시선 방향 충격을 준다 → 맞은 자리에 따라 회전도 생긴다.
 */
UCLASS()
class GLOSSARYPHYSICS_API ADemo0205_SimulatePhysics : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0205_SimulatePhysics();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 충격량 = 방향 × ImpulseStrength × 질량 (질량과 무관하게 같은 속도 변화를 주려고 질량을 곱한다) */
	UPROPERTY(EditAnywhere, Category = "Simulate Physics")
	float ImpulseStrength = 900.f;

private:
	void SpawnStacks();
	void PushAt(const FHitResult& Hit, const FVector& Direction);

	UPROPERTY() TObjectPtr<ACameraActor> Camera;
	UPROPERTY() TArray<TObjectPtr<AStaticMeshActor>> Boxes;
	FString LastResult;
	FVector LastPoint = FVector::ZeroVector;
	FVector LastDir = FVector::ZeroVector;
	float ArrowTime = 0.f;
};
