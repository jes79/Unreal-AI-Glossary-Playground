#pragma once

#include "CoreMinimal.h"
#include "GlossaryMaterialDemoBase.h"
#include "Demo0420_Refraction.generated.h"

class UMaterialInstanceDynamic;

/**
 * 0420 Refraction / 왜곡
 * 모닥불 위에 보이지 않는 판(M_HeatHaze: Translucent, Opacity 0)을 세우고, 위로 흐르는 노이즈 Panner 두 장을
 * Refraction(2D Offset 모드) 에 넣어 뒤 화면을 일렁이게 한다 — 열 아지랑이. 뒤 줄무늬 벽이 휘어 보인다.
 */
UCLASS()
class GLOSSARYMATERIAL_API ADemo0420_Refraction : public AGlossaryMaterialDemoBase
{
	GENERATED_BODY()

public:
	ADemo0420_Refraction();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 화면 공간 오프셋 세기 (2D Offset) */
	UPROPERTY(EditAnywhere, Category = "Refraction", meta = (ClampMin = 0))
	float HazeStrength = 0.03f;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;

	bool bHaze = true;
};
