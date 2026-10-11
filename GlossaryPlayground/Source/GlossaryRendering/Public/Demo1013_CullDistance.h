#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "RenderingDemoKit.h"
#include "Demo1013_CullDistance.generated.h"

class ACameraActor;
class ACullDistanceVolume;
class UStaticMeshComponent;

/**
 * 1013 Cull Distance (거리 컬링)
 * 빌더(BuildLevel)가 Cull Distance Volume(크기 50cm 이하 → 3000cm 밖에서 안 그림)을 놓는다.
 * 작은 소품 수백 개가 30m 밖에서 사라지고, 노란 가로등은 개별 Max Draw Distance(1500cm)로 더 일찍 사라진다.
 */
UCLASS()
class GLOSSARYRENDERING_API ADemo1013_CullDistance : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1013_CullDistance();

#if WITH_EDITOR
	virtual void BuildLevel(UWorld* World) override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Cull Distance Volume: 이 크기(cm) 이하의 물체는 */
	UPROPERTY(EditAnywhere, Category = "Cull Distance") float SmallPropSize = 50.f;
	/** 이 거리(cm) 밖에서 그리지 않는다 */
	UPROPERTY(EditAnywhere, Category = "Cull Distance") float SmallPropCullDistance = 3000.f;
	/** 가로등 개별 설정: Rendering > Max Draw Distance (LDMaxDrawDistance) */
	UPROPERTY(EditAnywhere, Category = "Cull Distance") float LampMaxDrawDistance = 1500.f;

	UPROPERTY(Transient) TObjectPtr<ACullDistanceVolume> Volume;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Props;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Lamps;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;

private:
	void SetCullingEnabled(bool bEnabled);
	bool bCulling = true;
	RenderKit::FFrameStats Stats;
};
