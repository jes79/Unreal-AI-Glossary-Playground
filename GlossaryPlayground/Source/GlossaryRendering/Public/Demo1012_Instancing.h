#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "RenderingDemoKit.h"
#include "Demo1012_Instancing.generated.h"

class ACameraActor;
class AStaticMeshActor;
class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;

/**
 * 1012 Instanced Static Mesh (ISM · HISM)
 * 나무 5000그루를 (1) HierarchicalInstancedStaticMeshComponent 하나의 인스턴스 5000개, (2) StaticMeshActor 5000개로 만들어
 * 드로우콜·Game/Draw 스레드 시간을 비교한다.
 */
UCLASS()
class GLOSSARYRENDERING_API ADemo1012_Instancing : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1012_Instancing();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Instancing", meta = (ClampMin = 1))
	int32 TreeCount = 5000;

	UPROPERTY(VisibleAnywhere, Category = "Instancing")
	bool bUseHISM = true;

	UPROPERTY(Transient) TObjectPtr<UStaticMesh> TreeMesh;
	UPROPERTY(Transient) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> HISM;
	UPROPERTY(Transient) TArray<TObjectPtr<AStaticMeshActor>> TreeActors;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;

private:
	void Rebuild(bool bHISM);
	TArray<FTransform> TreeTransforms;
	double LastBuildMs = 0.0;
	RenderKit::FFrameStats Stats;
};
