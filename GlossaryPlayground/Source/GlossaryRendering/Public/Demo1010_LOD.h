#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "RenderingDemoKit.h"
#include "Demo1010_LOD.generated.h"

class ACameraActor;
class AGlossaryLabel;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * 1010 LOD
 * 빌더가 비 Nanite 바위(20,480 삼각형)에 LOD 3단계를 자동 생성(Reduction 50/25/10%)하고 Screen Size 1.0/0.5/0.25/0.1 을 지정했다.
 * 거리가 다른 바위 5개가 지금 몇 번 LOD 인지 색과 라벨로 보여 준다. 카메라를 앞뒤로 움직이면 LOD 가 바뀐다.
 */
UCLASS()
class GLOSSARYRENDERING_API ADemo1010_LOD : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1010_LOD();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** 0 = 자동, 1~4 = 해당 LOD 강제 (Forced LOD Model) */
	UPROPERTY(EditAnywhere, Category = "LOD", meta = (ClampMin = 0, ClampMax = 4))
	int32 ForcedLOD = 0;

	UPROPERTY(EditAnywhere, Category = "LOD")
	bool bWireframe = false;

	UPROPERTY(Transient) TObjectPtr<UStaticMesh> Mesh;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Rocks;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> RockMIDs;
	UPROPERTY(Transient) TArray<TObjectPtr<AGlossaryLabel>> Labels;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;

private:
	int32 ComputeLOD(const UStaticMeshComponent* Rock, float& OutScreenSize) const;
	RenderKit::FFrameStats Stats;
};
