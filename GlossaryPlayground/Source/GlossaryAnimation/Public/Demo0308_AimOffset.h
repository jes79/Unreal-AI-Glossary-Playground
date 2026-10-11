#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0308_AimOffset.generated.h"

class AD03_Mannequin;
class UD03_AnimInstance;
class UD03_GraphWidget;
class AStaticMeshActor;

/**
 * 0308 Aim Offset
 * Pitch -90~90 · Yaw -90~90 조준 값을 척추 5개에 나눠 돌려 상체가 조준 방향을 따라가게 한다.
 * NPC 는 움직이는 구를, 플레이어는 카메라 방향을 조준한다.
 */
UCLASS()
class GLOSSARYANIMATION_API ADemo0308_AimOffset : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0308_AimOffset();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Aim Offset")
	FVector2D PitchRange = FVector2D(-90.f, 90.f);

	UPROPERTY(EditAnywhere, Category = "Aim Offset")
	FVector2D YawRange = FVector2D(-90.f, 90.f);

	UPROPERTY(EditAnywhere, Category = "Aim Offset")
	bool bAimOffsetEnabled = true;

private:
	UPROPERTY(Transient) TObjectPtr<AD03_Mannequin> Npc;
	UPROPERTY(Transient) TObjectPtr<UD03_AnimInstance> NpcAnim;
	UPROPERTY(Transient) TObjectPtr<UD03_AnimInstance> PlayerAnim;
	UPROPERTY(Transient) TObjectPtr<AStaticMeshActor> Target;
	UPROPERTY(Transient) TObjectPtr<UD03_GraphWidget> Graph;
	float Time = 0.f;
};
