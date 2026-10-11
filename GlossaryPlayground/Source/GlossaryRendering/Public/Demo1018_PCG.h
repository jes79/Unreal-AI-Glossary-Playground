#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "RenderingDemoKit.h"
#include "Demo1018_PCG.generated.h"

class ACameraActor;
class AStaticMeshActor;
class UPCGComponent;
class USplineComponent;

/**
 * 1018 PCG (절차적 배치)
 * 빌더가 만든 PCG 그래프(PCG_ForestScatter):
 *   Get Spline Data(PCG_Area) → Create Surface From Spline → Surface Sampler(0.3개/㎡) → Difference ← Bounds Modifier ← Spline Sampler ← Get Spline Data(PCG_Path)
 *   → Transform Points(무작위 회전·크기) → Static Mesh Spawner(SM_Tree)
 * 실행 중 PCG 컴포넌트에 그래프를 넣고 Generate 하면 영역 안에 나무가 뿌려지고 길 주변은 비워진다.
 */
UCLASS()
class GLOSSARYRENDERING_API ADemo1018_PCG : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1018_PCG();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 숲 영역 한 변 길이 (cm) */
	UPROPERTY(EditAnywhere, Category = "PCG") float AreaSize = 4000.f;

	UPROPERTY(Transient) TObjectPtr<UPCGComponent> PCG;
	UPROPERTY(Transient) TObjectPtr<USplineComponent> AreaSpline;
	UPROPERTY(Transient) TObjectPtr<USplineComponent> PathSpline;
	UPROPERTY(Transient) TArray<TObjectPtr<AStaticMeshActor>> PathTiles;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;

private:
	USplineComponent* SpawnSplineActor(FName Tag);
	void BuildPath();
	void Generate();
	int32 CountInstances() const;
	float PathOffset = 0.f;
	double GenerateStart = 0.0;
	float LastGenerateSeconds = 0.f;
	bool bWaiting = false;
	RenderKit::FFrameStats Stats;
};
