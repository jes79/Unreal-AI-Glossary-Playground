#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0513_HitSpark.generated.h"

class UNiagaraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APostProcessVolume;

/**
 * 0513 Hit Spark — Line Trace 가 맞은 지점에서 표면 노멀 방향으로 불꽃을 SpawnSystemAtLocation 한다.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0513_HitSpark : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0513_HitSpark();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Hit Spark") float FireInterval = 0.25f;

private:
	void FireTrace();
	FVector Muzzle = FVector::ZeroVector;
	float Timer = 0.f;
	int32 Hits = 0;
	FString LastHit;
};
