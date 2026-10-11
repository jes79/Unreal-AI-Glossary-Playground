#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Physics/Experimental/ChaosEventType.h"
#include "Demo0211_ChaosDestruction.generated.h"

class ACameraActor;
class AStaticMeshActor;
class UGeometryCollection;
class UGeometryCollectionComponent;
class UFieldSystemComponent;
class UPrimitiveComponent;

/**
 * 0211 Chaos Destruction (Geometry Collection)
 * 빌더가 Fracture 모드와 같은 방식(PlanarCut Voronoi)으로 만든 GC_Wall(20조각)을 세우고,
 * 총알이 맞은 지점에 Field System(External Strain + Radial Velocity)을 걸어 부순다.
 */
UCLASS()
class GLOSSARYPHYSICS_API ADemo0211_ChaosDestruction : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0211_ChaosDestruction();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Chaos Destruction")
	TObjectPtr<UGeometryCollection> WallCollection;

	/** 맞은 지점에서 이 반경 안의 클러스터 연결을 끊는다 */
	UPROPERTY(EditAnywhere, Category = "Chaos Destruction")
	float FieldRadius = 130.f;

	/** External Strain 크기 (Damage Threshold 보다 커야 부서진다) */
	UPROPERTY(EditAnywhere, Category = "Chaos Destruction")
	float StrainMagnitude = 1000000.f;

	/** 떨어져 나간 조각을 밀어내는 속도 (cm/s) */
	UPROPERTY(EditAnywhere, Category = "Chaos Destruction")
	float PushVelocity = 500.f;

	UPROPERTY(EditAnywhere, Category = "Chaos Destruction")
	float BulletSpeed = 4000.f;

	UPROPERTY(VisibleAnywhere, Category = "Chaos Destruction")
	TObjectPtr<UFieldSystemComponent> FieldSystem;

	UFUNCTION()
	void OnWallBreak(const FChaosBreakEvent& BreakEvent);

	UFUNCTION()
	void OnBulletHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	void SpawnWall();
	void FireBullet(const FVector& Target);
	void BreakAt(const FVector& Location, const FVector& Direction);

	UPROPERTY() TObjectPtr<ACameraActor> Camera;
	UPROPERTY() TObjectPtr<AActor> WallActor;
	UPROPERTY() TObjectPtr<UGeometryCollectionComponent> WallComponent;
	UPROPERTY() TArray<TObjectPtr<AStaticMeshActor>> Bullets;
	int32 NumPieces = 0;
	TArray<FVector> InitialPieceLocations;
	int32 BreakEvents = 0;
	int32 Shots = 0;
	FVector LastBreak = FVector::ZeroVector;
	float BreakFlash = 0.f;
};
