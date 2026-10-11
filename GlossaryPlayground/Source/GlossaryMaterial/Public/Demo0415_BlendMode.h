#pragma once

#include "CoreMinimal.h"
#include "GlossaryMaterialDemoBase.h"
#include "Demo0415_BlendMode.generated.h"

class UMaterialInstanceDynamic;

/**
 * 0415 Blend Mode (Opaque·Masked·Translucent·Additive)
 * 줄무늬 벽 앞에 네 블렌드 모드를 나란히: 불투명 큐브, 잎사귀(Masked, Clip 0.33), 유리(Translucent), 불빛(Additive).
 */
UCLASS()
class GLOSSARYMATERIAL_API ADemo0415_BlendMode : public AGlossaryMaterialDemoBase
{
	GENERATED_BODY()

public:
	ADemo0415_BlendMode();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 유리 Opacity (Translucent) */
	UPROPERTY(EditAnywhere, Category = "Blend Mode", meta = (ClampMin = 0, ClampMax = 1))
	float GlassOpacity = 0.25f;

	/** 불빛 세기 (Additive) */
	UPROPERTY(EditAnywhere, Category = "Blend Mode", meta = (ClampMin = 0))
	float GlowIntensity = 6.f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlassMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlowMaterial;
};
