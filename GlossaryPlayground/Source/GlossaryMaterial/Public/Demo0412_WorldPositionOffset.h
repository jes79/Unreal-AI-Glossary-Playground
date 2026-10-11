#pragma once

#include "CoreMinimal.h"
#include "GlossaryMaterialDemoBase.h"
#include "Demo0412_WorldPositionOffset.generated.h"

class UMaterialInstanceDynamic;
class UProceduralMeshComponent;

/**
 * 0412 World Position Offset
 * 풀(M_WindGrass)과 깃발(M_Flag) 모두 WPO 에 Sine(Time) 을 연결해 정점을 옮긴다 — CPU 는 아무것도 움직이지 않는다.
 * 풀은 높이², 깃발은 깃대에서의 거리(U) 로 흔들림을 줄여 뿌리/깃대 쪽은 고정된다.
 */
UCLASS()
class GLOSSARYMATERIAL_API ADemo0412_WorldPositionOffset : public AGlossaryMaterialDemoBase
{
	GENERATED_BODY()

public:
	ADemo0412_WorldPositionOffset();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 흔들림 폭 (cm) */
	UPROPERTY(EditAnywhere, Category = "World Position Offset", meta = (ClampMin = 0))
	float WindStrength = 25.f;

	/** 초당 흔들림 횟수 (Sine 주기 1 → sin(2π × Time × 이 값)) */
	UPROPERTY(EditAnywhere, Category = "World Position Offset", meta = (ClampMin = 0))
	float WindFrequency = 0.6f;

	/** 깃발 격자 정점 수 (WPO 는 정점을 옮기므로 정점이 많아야 물결이 보인다) */
	UPROPERTY(EditAnywhere, Category = "World Position Offset", meta = (ClampMin = 2))
	FIntPoint FlagResolution = FIntPoint(32, 20);

private:
	void Apply();
	void BuildFlag(UProceduralMeshComponent* Mesh, float Width, float Height);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GrassMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FlagMaterial;

	bool bWind = true;
};
