#pragma once

#include "CoreMinimal.h"
#include "GlossaryMaterialDemoBase.h"
#include "Demo0419_CustomNode.generated.h"

class UMaterialInstanceDynamic;

/**
 * 0419 Custom 노드 (HLSL)
 * M_CustomChecker: Custom 노드 안에 HLSL 로 체커보드 함수를 쓰고(floor·fmod), 칸 수(CellCount)를 스칼라 파라미터로 노출한다.
 * 판·큐브·구에 입히고 [ ] 로 칸 수를 바꾼다.
 */
UCLASS()
class GLOSSARYMATERIAL_API ADemo0419_CustomNode : public AGlossaryMaterialDemoBase
{
	GENERATED_BODY()

public:
	ADemo0419_CustomNode();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Custom 노드의 Cells 입력으로 들어가는 칸 수 */
	UPROPERTY(EditAnywhere, Category = "Custom Node", meta = (ClampMin = 1))
	float CellCount = 8.f;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;

	int32 PaletteIndex = 0;
};
