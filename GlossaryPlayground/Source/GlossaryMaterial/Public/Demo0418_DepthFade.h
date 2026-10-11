#pragma once

#include "CoreMinimal.h"
#include "GlossaryMaterialDemoBase.h"
#include "Demo0418_DepthFade.generated.h"

class UMaterialInstanceDynamic;

/**
 * 0418 Depth Fade
 * 반투명 물 평면(M_Water_DepthFade)이 바위·경사·기둥과 만나는 곳. Opacity 를 DepthFade(FadeDistance 100) 에 통과시켜
 * 물 아래 지형까지의 깊이가 얕을수록 투명해지므로 교차선이 부드럽게 사라진다. C 로 끄면(거리 ≈ 0) 딱딱한 경계선이 보인다.
 */
UCLASS()
class GLOSSARYMATERIAL_API ADemo0418_DepthFade : public AGlossaryMaterialDemoBase
{
	GENERATED_BODY()

public:
	ADemo0418_DepthFade();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 이 깊이(cm)에 걸쳐 0 → 불투명으로 바뀐다 */
	UPROPERTY(EditAnywhere, Category = "Depth Fade", meta = (ClampMin = 1))
	float FadeDistance = 100.f;

	UPROPERTY(EditAnywhere, Category = "Depth Fade", meta = (ClampMin = 0, ClampMax = 1))
	float WaterOpacity = 0.85f;

	/** 수면 높이 */
	UPROPERTY(EditAnywhere, Category = "Depth Fade")
	float WaterLevel = 150.f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WaterMaterial;

	bool bDepthFade = true;
};
