#pragma once

#include "CoreMinimal.h"
#include "GlossaryMaterialDemoBase.h"
#include "Demo0411_Outline.generated.h"

class UPostProcessComponent;
class UPrimitiveComponent;
class UMaterialInstanceDynamic;

/**
 * 0411 외곽선 (Outline, Custom Depth)
 * 선택된 적의 메시에만 Render CustomDepth Pass 를 켜고, 포스트 프로세스 머티리얼(M_OutlinePostProcess)이
 * Custom Depth 버퍼의 경계(이웃 픽셀엔 물체가 있고 자기 픽셀은 비어 있는 곳)를 노랗게 칠한다.
 */
UCLASS()
class GLOSSARYMATERIAL_API ADemo0411_Outline : public AGlossaryMaterialDemoBase
{
	GENERATED_BODY()

public:
	ADemo0411_Outline();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 외곽선 두께 (픽셀) */
	UPROPERTY(EditAnywhere, Category = "Outline", meta = (ClampMin = 1))
	float OutlineThickness = 3.f;

	UPROPERTY(EditAnywhere, Category = "Outline")
	FLinearColor OutlineColor = FLinearColor(1.f, 0.85f, 0.f);

	/** Post Process Volume 대신 데모 액터에 붙인 Unbound Post Process (블렌더블 = 외곽선 머티리얼) */
	UPROPERTY(VisibleAnywhere, Category = "Outline")
	TObjectPtr<UPostProcessComponent> OutlinePostProcess;

private:
	void Select(int32 Index);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPrimitiveComponent>> Enemies;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OutlineMaterial;

	int32 Selected = 0;
	bool bOutline = true;
};
