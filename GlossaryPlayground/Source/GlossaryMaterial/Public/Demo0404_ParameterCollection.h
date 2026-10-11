#pragma once

#include "CoreMinimal.h"
#include "GlossaryMaterialDemoBase.h"
#include "Demo0404_ParameterCollection.generated.h"

class UMaterialParameterCollection;

/**
 * 0404 Material Parameter Collection
 * MPC_Player 의 PlayerPosition 을 매 Tick 플레이어 위치로 갱신한다.
 * 이 값을 읽는 머티리얼 두 개(M_Grass_MPC 풀 WPO, M_Soil_MPC 바닥 링)가 동시에 반응한다 — 머티리얼마다 값을 넣을 필요가 없다.
 */
UCLASS()
class GLOSSARYMATERIAL_API ADemo0404_ParameterCollection : public AGlossaryMaterialDemoBase
{
	GENERATED_BODY()

public:
	ADemo0404_ParameterCollection();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 풀이 눌리는 반경 (MPC 의 PushRadius 에 써 넣는다) */
	UPROPERTY(EditAnywhere, Category = "Parameter Collection", meta = (ClampMin = 50))
	float PushRadius = 220.f;

	/** 풀 밭 크기와 간격 */
	UPROPERTY(EditAnywhere, Category = "Parameter Collection")
	FVector2D FieldSize = FVector2D(2400.f, 2200.f);

	UPROPERTY(EditAnywhere, Category = "Parameter Collection", meta = (ClampMin = 10))
	float BladeSpacing = 28.f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollection> Collection;

	bool bUpdateCollection = true;
};
