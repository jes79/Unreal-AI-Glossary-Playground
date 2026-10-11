#pragma once

#include "CoreMinimal.h"
#include "GlossaryMaterialDemoBase.h"
#include "Demo0407_Emissive.generated.h"

class UMaterialInstanceDynamic;
class UPostProcessComponent;

/**
 * 0407 Emissive (발광)
 * M_Emissive: Emissive Color = 주황 × 20. 1 을 넘는 HDR 값이라 Bloom 이 주변으로 번지고, Lumen 이 바닥을 비춘다.
 * B 로 Bloom ON/OFF, [ ] 로 세기(×0.5/×2) 를 바꿔 비교한다.
 */
UCLASS()
class GLOSSARYMATERIAL_API ADemo0407_Emissive : public AGlossaryMaterialDemoBase
{
	GENERATED_BODY()

public:
	ADemo0407_Emissive();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Emissive")
	FLinearColor EmissiveColor = FLinearColor(1.f, 0.35f, 0.03f);

	/** 요청: 주황 × 20 */
	UPROPERTY(EditAnywhere, Category = "Emissive", meta = (ClampMin = 0))
	float EmissiveStrength = 20.f;

	UPROPERTY(EditAnywhere, Category = "Emissive", meta = (ClampMin = 0))
	float BloomIntensity = 1.f;

	/** Bloom 을 켜고 끄는 데모 전용 Post Process (Unbound) */
	UPROPERTY(VisibleAnywhere, Category = "Emissive")
	TObjectPtr<UPostProcessComponent> PostProcess;

private:
	void Apply();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;

	bool bBloom = true;
};
