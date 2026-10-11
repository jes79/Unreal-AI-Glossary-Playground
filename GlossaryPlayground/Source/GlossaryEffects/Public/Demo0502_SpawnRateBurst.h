#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0502_SpawnRateBurst.generated.h"

class UNiagaraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APostProcessVolume;

/**
 * 0502 Spawn Rate / Spawn Burst — 왼쪽 연기는 Spawn Rate 50(계속), 오른쪽 파편은 Spawn Burst Instantaneous 100(한꺼번에).
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0502_SpawnRateBurst : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0502_SpawnRateBurst();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 연기 이미터의 초당 스폰 수 (시스템 User.SmokeSpawnRate 에 연결) */
	UPROPERTY(EditAnywhere, Category = "Spawn") float SpawnRate = 50.f;
	/** 파편 이미터가 한 번에 터뜨리는 수 (빌더에서 Spawn Burst Instantaneous.Spawn Count 로 설정) */
	UPROPERTY(VisibleAnywhere, Category = "Spawn") int32 BurstCount = 100;
	/** 자동으로 다시 터뜨리는 간격(초) */
	UPROPERTY(EditAnywhere, Category = "Spawn") float BurstInterval = 2.5f;

private:
	void FireBurst();
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> Smoke;
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> Debris;
	float BurstTimer = 0.f;
	int32 BurstCounter = 0;
};
