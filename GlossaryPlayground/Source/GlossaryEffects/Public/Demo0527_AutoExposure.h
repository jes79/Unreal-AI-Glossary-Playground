#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0527_AutoExposure.generated.h"

class APostProcessVolume;
class UMaterialInstanceDynamic;
class UPointLightComponent;

/**
 * 0527 Auto Exposure — 어두운 동굴 안에 있다가 밖으로 나오면 눈이 부셨다가 천천히 적응한다(Speed Up/Down, Min/Max EV100 고정).
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0527_AutoExposure : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0527_AutoExposure();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Auto Exposure") float SpeedUp = 0.6f;
	UPROPERTY(EditAnywhere, Category = "Auto Exposure") float SpeedDown = 2.f;
	UPROPERTY(EditAnywhere, Category = "Auto Exposure") float MinEV100 = -2.f;
	UPROPERTY(EditAnywhere, Category = "Auto Exposure") float MaxEV100 = 14.f;

private:
	void Teleport(bool bInside);
	UPROPERTY(Transient) TObjectPtr<APostProcessVolume> Volume;
	bool bAuto = true;
	FString Where = TEXT("동굴 안");
};
