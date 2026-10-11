#pragma once

#include "CoreMinimal.h"
#include "GlossaryMaterialDemoBase.h"
#include "Demo0405_MaterialFunction.generated.h"

class UMaterialInstanceDynamic;

/**
 * 0405 Material Function
 * 림 라이트 계산(Fresnel × RimColor × RimIntensity)을 MF_RimLight 로 한 번만 만들고,
 * 금속·돌·유령 머티리얼 세 개가 Material Function Call 노드로 같은 함수를 부른다.
 */
UCLASS()
class GLOSSARYMATERIAL_API ADemo0405_MaterialFunction : public AGlossaryMaterialDemoBase
{
	GENERATED_BODY()

public:
	ADemo0405_MaterialFunction();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 함수를 부르는 머티리얼들 (빌더가 MF_RimLight 호출 노드를 넣어 만든다) */
	UPROPERTY(EditAnywhere, Category = "Material Function")
	TArray<FString> CallerMaterials = { TEXT("M_Rim_Metal"), TEXT("M_Rim_Stone"), TEXT("M_Rim_Ghost") };

	/** 머티리얼마다 MF 입력에 넘기는 RimIntensity 배율 (C 로 0 ↔ 1) */
	UPROPERTY(EditAnywhere, Category = "Material Function", meta = (ClampMin = 0))
	float RimScale = 1.f;

private:
	void ApplyRim();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;

	TArray<float> DefaultIntensity;
	bool bRimOn = true;
};
