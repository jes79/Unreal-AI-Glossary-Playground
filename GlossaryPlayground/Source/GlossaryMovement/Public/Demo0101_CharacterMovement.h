#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0101_CharacterMovement.generated.h"

/**
 * 0101 Character Movement Component
 * 경사 30° / 45° / 50° 램프와 높이가 다른 계단(30cm·45cm·60cm)을 놓고,
 * Walkable Floor Angle 과 Max Step Height 가 어디까지 오르게 하는지 보여 준다.
 */
UCLASS()
class GLOSSARYMOVEMENT_API ADemo0101_CharacterMovement : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0101_CharacterMovement();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 이 각도까지의 경사만 걸어서 오른다 (CharacterMovement > Walkable Floor Angle) */
	UPROPERTY(EditAnywhere, Category = "Character Movement", meta = (ClampMin = 0, ClampMax = 89))
	float WalkableFloorAngle = 45.f;

	/** 이 높이까지의 턱은 계단처럼 자동으로 올라간다 (CharacterMovement > Max Step Height) */
	UPROPERTY(EditAnywhere, Category = "Character Movement", meta = (ClampMin = 0))
	float MaxStepHeight = 45.f;

private:
	void ApplyMovementSettings();
	void SpawnRamp(float Angle, const FVector& Base);
	void SpawnStep(float Height, const FVector& Base);
};
