#pragma once

#include "CoreMinimal.h"
#include "GlossaryMaterialDemoBase.h"
#include "Demo0410_Toon.generated.h"

class UMaterialInstanceDynamic;
class UPostProcessComponent;

/**
 * 0410 툰 셰이딩 (Cel Shading)
 * (1) 머티리얼 방식 M_Toon: Unlit + Dot(노멀, 라이트 방향) 을 3단계로 끊어 Emissive 로 출력. 라이트 방향은 해(Directional Light)에서 매 Tick 넘긴다.
 * (2) 포스트 프로세스 방식 M_ToonPostProcess: 화면 전체 밝기를 단계로 양자화 — 모든 Lit 물체에 한꺼번에 적용된다.
 * 왼쪽 줄 = 툰 머티리얼, 오른쪽 줄 = 일반 Lit 머티리얼.
 */
UCLASS()
class GLOSSARYMATERIAL_API ADemo0410_Toon : public AGlossaryMaterialDemoBase
{
	GENERATED_BODY()

public:
	ADemo0410_Toon();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 명암 단계 수 (요청: 3단계) */
	UPROPERTY(EditAnywhere, Category = "Toon", meta = (ClampMin = 2, ClampMax = 8))
	int32 ToonBands = 3;

	/** 그림자 단계의 밝기 */
	UPROPERTY(EditAnywhere, Category = "Toon", meta = (ClampMin = 0, ClampMax = 1))
	float ShadowTone = 0.2f;

	/** 포스트 프로세스 방식 (Unbound, 블렌더블 M_ToonPostProcess) */
	UPROPERTY(VisibleAnywhere, Category = "Toon")
	TObjectPtr<UPostProcessComponent> ToonPostProcess;

private:
	void ApplyMaterialMode();

	/** 툰 줄의 슬롯 하나: 툰 MID 와 OFF 일 때 되돌릴 Lit 머티리얼 */
	struct FToonSlot
	{
		TWeakObjectPtr<UPrimitiveComponent> Component;
		int32 Slot = 0;
	};
	TArray<FToonSlot> ToonSlots;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> ToonMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> LitMaterials;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PostProcessMaterial;

	bool bToonMaterial = true;
	bool bPostProcess = false;
	float SunYaw = 70.f;
};
