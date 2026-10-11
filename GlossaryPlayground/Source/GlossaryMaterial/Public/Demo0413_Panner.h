#pragma once

#include "CoreMinimal.h"
#include "GlossaryMaterialDemoBase.h"
#include "Demo0413_Panner.generated.h"

class UMaterialInstanceDynamic;

/**
 * 0413 Panner (UV 스크롤)
 * M_Lava: TexCoord → Panner(Speed = (PanSpeed, 0)) → 용암 텍스처. PanSpeed 0.1 = 초당 UV 0.1 만큼 흐른다.
 * 강처럼 긴 용암 판과, 같은 머티리얼이 PanSpeed 0 인 판을 나란히 놓아 비교한다.
 */
UCLASS()
class GLOSSARYMATERIAL_API ADemo0413_Panner : public AGlossaryMaterialDemoBase
{
	GENERATED_BODY()

public:
	ADemo0413_Panner();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 요청: 초당 0.1 (UV 단위) */
	UPROPERTY(EditAnywhere, Category = "Panner")
	float PanSpeed = 0.1f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LavaMaterial;

	bool bPanning = true;
};
