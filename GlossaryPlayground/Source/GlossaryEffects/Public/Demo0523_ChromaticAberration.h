#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0523_ChromaticAberration.generated.h"

class APostProcessVolume;
class UMaterialInstanceDynamic;
class UPointLightComponent;

/**
 * 0523 Chromatic Aberration — 피격 순간 Scene Fringe Intensity 를 5 로 0.2초 올렸다가 되돌린다.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0523_ChromaticAberration : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0523_ChromaticAberration();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Chromatic Aberration") float HitFringeIntensity = 5.f;
	UPROPERTY(EditAnywhere, Category = "Chromatic Aberration") float HitDuration = 0.2f;

private:
	UPROPERTY(Transient) TObjectPtr<APostProcessVolume> Volume;
	float HitTimer = 0.f;
	bool bHold = false;
	float AutoTimer = 0.f;
};
