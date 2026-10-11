#pragma once

#include "CoreMinimal.h"
#include "GlossaryMaterialDemoBase.h"
#include "Demo0416_Decal.generated.h"

class UDecalComponent;
class UMaterialInterface;

/**
 * 0416 Decal (데칼)
 * 총알 자국 M_BulletHoleDecal (Material Domain = Deferred Decal) 을 맞은 지점에 UGameplayStatics::SpawnDecalAtLocation 으로 붙이고,
 * UDecalComponent::SetFadeOut(10초, 1초) 로 10초 뒤 서서히 사라지게 한다. 페이드 값은 머티리얼의 Decal Lifetime Opacity 노드로 들어간다.
 */
UCLASS()
class GLOSSARYMATERIAL_API ADemo0416_Decal : public AGlossaryMaterialDemoBase
{
	GENERATED_BODY()

public:
	ADemo0416_Decal();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 요청: 10초 뒤 페이드 */
	UPROPERTY(EditAnywhere, Category = "Decal", meta = (ClampMin = 0))
	float FadeStartDelay = 10.f;

	UPROPERTY(EditAnywhere, Category = "Decal", meta = (ClampMin = 0.01))
	float FadeDuration = 1.f;

	/** 데칼 상자 크기 (X = 투사 깊이, Y·Z = 자국 크기 반지름) */
	UPROPERTY(EditAnywhere, Category = "Decal")
	FVector DecalSize = FVector(8.f, 14.f, 14.f);

	/** 자동 사격 간격 (A 로 ON/OFF) */
	UPROPERTY(EditAnywhere, Category = "Decal", meta = (ClampMin = 0.05))
	float AutoFireInterval = 0.25f;

private:
	void FireAt(const FVector& Start, const FVector& End);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> DecalMaterial;

	TArray<TWeakObjectPtr<UDecalComponent>> Decals;
	FRandomStream Random = FRandomStream(416);
	float AutoFireTimer = 0.f;
	int32 ShotCount = 0;
	bool bAutoFire = true;
};
