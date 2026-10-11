#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0213_PhysicsHandle.generated.h"

class UPhysicsHandleComponent;
class UPrimitiveComponent;

/**
 * 0213 Physics Handle (물건 잡기)
 * 좌클릭을 누르면 화면 가운데 물리 물체를 PhysicsHandleComponent 로 잡아 카메라 앞 300cm 에 띄워 들고,
 * 버튼을 놓으면 Release + 시선 방향 Impulse 로 던진다 (그래비티 건).
 */
UCLASS()
class GLOSSARYPHYSICS_API ADemo0213_PhysicsHandle : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0213_PhysicsHandle();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Physics Handle")
	TObjectPtr<UPhysicsHandleComponent> PhysicsHandle;

	/** 잡을 수 있는 거리 */
	UPROPERTY(EditAnywhere, Category = "Physics Handle")
	float GrabRange = 1200.f;

	/** 들고 있을 때 카메라 앞 거리 */
	UPROPERTY(EditAnywhere, Category = "Physics Handle")
	float HoldDistance = 300.f;

	/** 던질 때 속도 변화 (cm/s) */
	UPROPERTY(EditAnywhere, Category = "Physics Handle")
	float ThrowSpeed = 2200.f;

private:
	void SpawnProps();
	void Grab();
	void Throw();

	TWeakObjectPtr<UPrimitiveComponent> Held;
	int32 Throws = 0;
	FString LastAction;
};
