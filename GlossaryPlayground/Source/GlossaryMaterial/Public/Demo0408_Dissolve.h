#pragma once

#include "CoreMinimal.h"
#include "GlossaryMaterialDemoBase.h"
#include "Demo0408_Dissolve.generated.h"

class UMaterialInstanceDynamic;

/**
 * 0408 디졸브 (Dissolve)
 * M_Dissolve(Masked): 노이즈 − DissolveAmount 가 Clip 아래인 픽셀을 버리고, 잘린 경계 근처는 주황 Emissive 로 빛낸다.
 * Space 로 1초 동안 0→1 (사라짐) 재생, 다시 누르면 1초 동안 되살아난다. 자동 반복 기본 ON.
 */
UCLASS()
class GLOSSARYMATERIAL_API ADemo0408_Dissolve : public AGlossaryMaterialDemoBase
{
	GENERATED_BODY()

public:
	ADemo0408_Dissolve();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 요청: 1초 동안 사라짐 */
	UPROPERTY(EditAnywhere, Category = "Dissolve", meta = (ClampMin = 0.05))
	float DissolveDuration = 1.f;

	/** 경계선 두께 (노이즈 값 단위) */
	UPROPERTY(EditAnywhere, Category = "Dissolve", meta = (ClampMin = 0))
	float EdgeWidth = 0.08f;

	UPROPERTY(EditAnywhere, Category = "Dissolve")
	FLinearColor EdgeColor = FLinearColor(1.f, 0.35f, 0.02f);

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;

	float Amount = 0.f;
	float Direction = 0.f;      // +1 사라지는 중, -1 나타나는 중, 0 정지
	float HoldTimer = 0.f;
	bool bAutoLoop = true;
};
