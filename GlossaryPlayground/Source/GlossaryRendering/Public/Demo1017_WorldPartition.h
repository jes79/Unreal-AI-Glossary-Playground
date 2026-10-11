#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "RenderingDemoKit.h"
#include "Demo1017_WorldPartition.generated.h"

class ACameraActor;

/**
 * 1017 World Partition
 * 768m × 768m 월드를 32m 셀 24×24 개로 나누고, 플레이어(스트리밍 소스)에서 Loading Range 12800cm 안의 셀만
 * 내용(바닥 타일·건물)을 만들고 밖으로 나간 셀은 지운다 — World Partition 런타임 그리드가 하는 일을 직접 구현.
 */
UCLASS()
class GLOSSARYRENDERING_API ADemo1017_WorldPartition : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1017_WorldPartition();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** World Settings > World Partition > Runtime Settings > Grid 의 Cell Size */
	UPROPERTY(EditAnywhere, Category = "World Partition") float CellSize = 3200.f;
	/** Loading Range (요청 예문 12800) */
	UPROPERTY(EditAnywhere, Category = "World Partition") float LoadingRange = 12800.f;
	UPROPERTY(EditAnywhere, Category = "World Partition") int32 GridCells = 24;

	UPROPERTY(Transient) TObjectPtr<ACameraActor> MapCamera;

private:
	struct FCell
	{
		TArray<TWeakObjectPtr<AActor>> Actors;
	};
	FBox2D CellBounds(const FIntPoint& Cell) const;
	void LoadCell(const FIntPoint& Cell);
	void UnloadCell(const FIntPoint& Cell);
	void UpdateStreaming(const FVector& Source);
	void DrawGrid(const FVector& Source) const;

	TMap<FIntPoint, FCell> Loaded;
	int32 TotalLoads = 0;
	int32 TotalUnloads = 0;
	bool bMapView = true;
	bool bAutoRun = false;
	float AutoRunAngle = 0.f;
	RenderKit::FFrameStats Stats;
};
