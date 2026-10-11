#pragma once

#include "CoreMinimal.h"
#include "GlossaryMaterialDemoBase.h"
#include "Demo0414_WorldAligned.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * 0414 World Aligned Texture (트라이플래너)
 * 크기가 제각각인 바위(스케일한 구·큐브) 두 줄. 뒷줄 = 메시 UV(M_Rock_UV) → 큰 바위일수록 벽돌이 늘어난다.
 * 앞줄 = M_Rock_WorldAligned (엔진 함수 WorldAlignedTexture 호출) → 월드 좌표 기준이라 크기와 상관없이 벽돌 크기가 같다.
 */
UCLASS()
class GLOSSARYMATERIAL_API ADemo0414_WorldAligned : public AGlossaryMaterialDemoBase
{
	GENERATED_BODY()

public:
	ADemo0414_WorldAligned();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 텍스처 한 장이 덮는 월드 크기 (cm) */
	UPROPERTY(EditAnywhere, Category = "World Aligned", meta = (ClampMin = 10))
	float TextureSize = 150.f;

private:
	void Apply();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> FrontRocks;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WorldAlignedMaterial;

	bool bWorldAligned = true;
};
