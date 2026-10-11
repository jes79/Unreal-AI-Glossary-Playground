#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0520_PostProcessVolume.generated.h"

class APostProcessVolume;
class UMaterialInstanceDynamic;
class UPointLightComponent;

/**
 * 0520 Post Process Volume — 화면 전체에 적용되는 Unbound(Infinite Extent) 볼륨 + 동굴 구역에만 적용되는 상자 볼륨(Blend Radius 로 경계에서 섞임).
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0520_PostProcessVolume : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0520_PostProcessVolume();

#if WITH_EDITOR
	virtual void BuildLevel(UWorld* World) override;
#endif
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 동굴 볼륨 경계에서 섞이는 거리(cm) */
	UPROPERTY(EditAnywhere, Category = "Post Process Volume") float CaveBlendRadius = 400.f;

private:
	UPROPERTY(Transient) TObjectPtr<APostProcessVolume> GlobalVolume;
	UPROPERTY(Transient) TObjectPtr<APostProcessVolume> CaveVolume;
};
