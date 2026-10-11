#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0210_CCD.generated.h"

class ACameraActor;
class AStaticMeshActor;

/**
 * 0210 CCD (연속 충돌 감지)
 * 얇은 벽(두께 4cm) 두 레인에 같은 빠른 총알(물리 시뮬레이션, 25000cm/s)을 쏜다.
 * 위 레인은 Use CCD OFF → 한 프레임에 수 m 를 건너뛰어 벽을 뚫고, 아래 레인은 bUseCCD=true → 벽에 막힌다.
 */
UCLASS()
class GLOSSARYPHYSICS_API ADemo0210_CCD : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0210_CCD();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "CCD")
	float BulletSpeed = 25000.f;

	UPROPERTY(EditAnywhere, Category = "CCD")
	float WallThickness = 4.f;

	UPROPERTY(EditAnywhere, Category = "CCD")
	float FireInterval = 1.0f;

private:
	struct FShot
	{
		TWeakObjectPtr<AStaticMeshActor> Actor;
		int32 Lane = 0;
		FVector LastPos = FVector::ZeroVector;
		bool bCounted = false;
		float Age = 0.f;
	};

	void Fire();

	UPROPERTY() TObjectPtr<ACameraActor> Camera;
	TArray<FShot> Shots;
	int32 Passed[2] = { 0, 0 };
	int32 Blocked[2] = { 0, 0 };
	float Timer = 0.f;
};
