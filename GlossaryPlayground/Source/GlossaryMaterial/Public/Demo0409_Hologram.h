#pragma once

#include "CoreMinimal.h"
#include "GlossaryMaterialDemoBase.h"
#include "Demo0409_Hologram.generated.h"

class ASkeletalMeshActor;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/**
 * 0409 홀로그램
 * M_Hologram (Translucent · Unlit): 월드 높이 기반 스캔라인이 위로 흐르고, Fresnel 테두리, Sine(Time) 깜빡임.
 * 투사기 위 마네킹과 구에 입히고, C 로 원래 머티리얼과 비교한다.
 */
UCLASS()
class GLOSSARYMATERIAL_API ADemo0409_Hologram : public AGlossaryMaterialDemoBase
{
	GENERATED_BODY()

public:
	ADemo0409_Hologram();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 스캔라인이 위로 흐르는 속도 */
	UPROPERTY(EditAnywhere, Category = "Hologram", meta = (ClampMin = 0))
	float ScanSpeed = 0.8f;

	/** 깜빡임 세기 (0 이면 깜빡임 없음) */
	UPROPERTY(EditAnywhere, Category = "Hologram", meta = (ClampMin = 0, ClampMax = 1))
	float FlickerAmount = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Hologram")
	FLinearColor HoloColor = FLinearColor(0.1f, 0.8f, 1.f);

private:
	void Apply();

	UPROPERTY(Transient)
	TObjectPtr<ASkeletalMeshActor> Figure;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> OrbMesh;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;

	bool bHologram = true;
};
