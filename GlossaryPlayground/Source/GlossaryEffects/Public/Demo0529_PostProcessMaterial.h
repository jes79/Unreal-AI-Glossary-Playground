#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0529_PostProcessMaterial.generated.h"

class APostProcessVolume;
class UMaterialInstanceDynamic;
class UPointLightComponent;

/**
 * 0529 Post Process Material — Post Process 도메인 머티리얼(흑백 + 스캔라인)을 Blendables 에 넣어 화면 전체에 적용.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0529_PostProcessMaterial : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0529_PostProcessMaterial();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(Transient) TObjectPtr<APostProcessVolume> Volume;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> ScanlineMID;
	bool bOn = true;
	float Time = 0.f;
};
