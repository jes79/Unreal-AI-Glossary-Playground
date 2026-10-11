#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0116_MovementModes.generated.h"

class AGlossaryLabel;
class UMovementSidePanel;

/**
 * 0116 이동 모드 (Walking · Falling · Swimming · Flying)
 * 레벨 빌더가 Physics Volume(Water Volume = true) 를 놓고(BuildLevel), 들어가면 CharacterMovement 가 자동으로 Swimming.
 * F 키로 Flying 토글. 머리 위 라벨과 오른쪽 기록으로 모드 전환을 보여 준다.
 */
UCLASS()
class GLOSSARYMOVEMENT_API ADemo0116_MovementModes : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0116_MovementModes();

#if WITH_EDITOR
	virtual void BuildLevel(UWorld* World) override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 물 영역 (Water Volume 과 반투명 물 상자가 같은 크기) */
	UPROPERTY(EditAnywhere, Category = "Movement Modes") FVector WaterCenter = FVector(1000.f, 0.f, 200.f);
	UPROPERTY(EditAnywhere, Category = "Movement Modes") FVector WaterSize = FVector(1600.f, 1600.f, 400.f);
	/** 물의 저항 (Physics Volume > Fluid Friction) */
	UPROPERTY(EditAnywhere, Category = "Movement Modes") float FluidFriction = 0.3f;

private:
	TWeakObjectPtr<AGlossaryLabel> ModeLabel;
	UPROPERTY(Transient) TObjectPtr<UMovementSidePanel> Panel;
	bool bHasWaterVolume = false;
};
