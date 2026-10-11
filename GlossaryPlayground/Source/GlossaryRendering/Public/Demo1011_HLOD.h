#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "RenderingDemoKit.h"
#include "Demo1011_HLOD.generated.h"

class ACameraActor;
class UStaticMeshComponent;
class UPrimitiveComponent;

/**
 * 1011 HLOD (Hierarchical LOD)
 * 마을 건물 25채(건물·지붕·굴뚝 = 컴포넌트 75개)와, 빌더가 미리 합쳐 둔 프록시 메시(1개·머티리얼 1개)를 같은 자리에 둔다.
 * 카메라가 HLOD 전환 거리보다 멀어지면 개별 건물을 숨기고 프록시 하나만 그린다 — World Partition HLOD 가 하는 일.
 */
UCLASS()
class GLOSSARYRENDERING_API ADemo1011_HLOD : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1011_HLOD();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 이 거리(cm)보다 멀면 HLOD 프록시로 바꾼다 (World Partition 에서는 셀 Loading Range 밖) */
	UPROPERTY(EditAnywhere, Category = "HLOD")
	float HLODTransitionDistance = 8000.f;

	/** 0 = 거리로 자동, 1 = 개별 건물 고정, 2 = 프록시 고정 */
	UPROPERTY(EditAnywhere, Category = "HLOD")
	int32 ForceMode = 0;

	UPROPERTY(EditAnywhere, Category = "HLOD")
	bool bTintProxy = true;

	UPROPERTY(Transient) TArray<TObjectPtr<UPrimitiveComponent>> Individual;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Proxy;
	UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> ProxyMID;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;

private:
	bool bShowingProxy = false;
	RenderKit::FFrameStats Stats;
};
