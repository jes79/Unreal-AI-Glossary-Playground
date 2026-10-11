#pragma once

#include "CoreMinimal.h"
#include "GlossaryMaterialDemoBase.h"
#include "Demo0401_PBR.generated.h"

class UMaterialInstanceDynamic;

/**
 * 0401 PBR 머티리얼 (Base Color·Metallic·Roughness)
 * 5×5 구 격자: 가로 = Metallic 0→1, 세로 = Roughness 0→1 (M_PBR 의 MID).
 * 옆에는 요청 예문 그대로 상수 노드로 만든 M_PBR_RedMetal(빨강·Metallic 1·Roughness 0.2) 구.
 */
UCLASS()
class GLOSSARYMATERIAL_API ADemo0401_PBR : public AGlossaryMaterialDemoBase
{
	GENERATED_BODY()

public:
	ADemo0401_PBR();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 격자 구의 Base Color (1/2/3 키로 흰색·빨강·금색) */
	UPROPERTY(EditAnywhere, Category = "PBR")
	FLinearColor BaseColor = FLinearColor(0.9f, 0.9f, 0.9f);

private:
	void ApplyBaseColor();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> GridMaterials;

	FString ColorName = TEXT("흰색");
};
