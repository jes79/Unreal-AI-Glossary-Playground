#pragma once

#include "CoreMinimal.h"
#include "GlossaryMaterialDemoBase.h"
#include "Demo0406_Fresnel.generated.h"

class UMaterialInstanceDynamic;

/**
 * 0406 Fresnel (림 라이트)
 * M_Fresnel: Fresnel 노드(Exponent 3) × 파랑 → Emissive. 구·마네킹·큐브의 가장자리만 파랗게 빛난다.
 * C 로 ON/OFF, [ ] 로 Exponent 를 바꿔 테두리 두께가 어떻게 변하는지 본다.
 */
UCLASS()
class GLOSSARYMATERIAL_API ADemo0406_Fresnel : public AGlossaryMaterialDemoBase
{
	GENERATED_BODY()

public:
	ADemo0406_Fresnel();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Fresnel Exponent (요청: 3). 클수록 테두리가 얇아진다 */
	UPROPERTY(EditAnywhere, Category = "Fresnel", meta = (ClampMin = 0.1))
	float FresnelExponent = 3.f;

	UPROPERTY(EditAnywhere, Category = "Fresnel")
	FLinearColor RimColor = FLinearColor(0.05f, 0.35f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Fresnel", meta = (ClampMin = 0))
	float RimIntensity = 8.f;

private:
	void Apply();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;

	bool bRimOn = true;
};
