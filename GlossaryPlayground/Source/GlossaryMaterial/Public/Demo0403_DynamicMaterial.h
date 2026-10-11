#pragma once

#include "CoreMinimal.h"
#include "GlossaryMaterialDemoBase.h"
#include "Demo0403_DynamicMaterial.generated.h"

class ASkeletalMeshActor;
class UMaterialInstanceDynamic;

/**
 * 0403 Dynamic Material Instance
 * 적 마네킹의 머티리얼(M_HitFlash)을 CreateDynamicMaterialInstance 로 복제해 두고,
 * 맞을 때마다 FlashAmount 를 1 로 올렸다가 0.1초 뒤 0 으로 내려 Emissive 가 빨갛게 번쩍이게 한다.
 */
UCLASS()
class GLOSSARYMATERIAL_API ADemo0403_DynamicMaterial : public AGlossaryMaterialDemoBase
{
	GENERATED_BODY()

public:
	ADemo0403_DynamicMaterial();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 피격 플래시 지속 시간 (요청: 0.1초) */
	UPROPERTY(EditAnywhere, Category = "Hit Flash", meta = (ClampMin = 0.01))
	float FlashDuration = 0.1f;

	/** 자동 피격 간격 (A 로 끄고 켬) */
	UPROPERTY(EditAnywhere, Category = "Hit Flash", meta = (ClampMin = 0.2))
	float AutoHitInterval = 1.5f;

	/** T 키 슬로 모션 배율 (0.1초 플래시를 눈으로 보기 쉽게) */
	UPROPERTY(EditAnywhere, Category = "Hit Flash", meta = (ClampMin = 0.01, ClampMax = 1))
	float SlowMotionScale = 0.05f;

private:
	void Hit();

	UPROPERTY(Transient)
	TObjectPtr<ASkeletalMeshActor> Enemy;

	/** 실행 중에 값을 바꾸는 머티리얼 인스턴스 (적마다 하나) */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> FlashMaterials;

	float FlashRemaining = 0.f;
	float AutoHitTimer = 0.f;
	int32 HitCount = 0;
	bool bAutoHit = true;
	bool bHoldFlash = false;
	bool bSlowMotion = false;
};
