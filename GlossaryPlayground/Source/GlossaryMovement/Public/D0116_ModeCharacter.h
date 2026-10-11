#pragma once

#include "CoreMinimal.h"
#include "GlossaryCharacter.h"
#include "D0116_ModeCharacter.generated.h"

/**
 * 0116 이동 모드 캐릭터. Walking·Falling 은 CharacterMovement 가 알아서, Swimming 은 Water Volume 에 들어가면 자동,
 * Flying 은 ToggleFlying()(F 키)으로 SetMovementMode 한다. 수영·비행 중에는 시선(피치) 방향으로 이동하고 Space/Ctrl 로 오르내린다.
 */
UCLASS()
class GLOSSARYMOVEMENT_API AD0116_ModeCharacter : public AGlossaryCharacter
{
	GENERATED_BODY()

public:
	AD0116_ModeCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** F: Flying ↔ Falling (떨어지면서 자연스럽게 Walking 으로 돌아온다) */
	void ToggleFlying();

	virtual void DoMove(float Right, float Forward) override;
	virtual void DoJumpStart() override;

	/** 모드가 바뀐 기록 (새 것이 위) */
	const TArray<FString>& GetModeLog() const { return ModeLog; }

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

	TArray<FString> ModeLog;
};
