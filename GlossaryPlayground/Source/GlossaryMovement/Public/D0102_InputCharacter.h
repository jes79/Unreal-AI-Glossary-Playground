#pragma once

#include "CoreMinimal.h"
#include "GlossaryCharacter.h"
#include "InputTriggers.h"
#include "D0102_InputCharacter.generated.h"

struct FInputActionInstance;

/**
 * 0102 Enhanced Input 데모용 캐릭터.
 * IA_Move · IA_Jump · IA_Attack · IA_HeavyAttack 와 IMC_Default 를 "실행 중에" NewObject 로 만든다.
 * (에디터에서 만드는 IA_/IMC_ 에셋과 같은 객체 — 템플릿 에셋은 /Game/Input/Actions/IA_* , /Game/Input/IMC_Default)
 * 키보드와 게임패드를 같은 액션에 매핑하고, 발생한 트리거 이벤트(Started/Ongoing/Triggered/Completed/Canceled)를 기록한다.
 */
UCLASS()
class GLOSSARYMOVEMENT_API AD0102_InputCharacter : public AGlossaryCharacter
{
	GENERATED_BODY()

public:
	AD0102_InputCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 최근 이벤트 (새 것이 위) */
	const TArray<FString>& GetEventLog() const { return EventLog; }
	FVector2D GetLastMove() const { return LastMove; }
	int32 GetMoveTriggeredCount() const { return MoveTriggeredCount; }
	float GetHeavyHoldTime() const { return HeavyHoldTime; }

	/** Hold 트리거 시간 (IA_HeavyAttack) */
	UPROPERTY(EditAnywhere, Category = "Enhanced Input")
	float HeavyAttackHoldTime = 0.5f;

protected:
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** 입력 액션·매핑 컨텍스트를 한 번만 만든다 */
	void BuildInputAssets();
	void LogEvent(const TCHAR* ActionName, ETriggerEvent Event, const FInputActionInstance& Instance);
	void SpawnAttackFlash(bool bHeavy);

	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Move;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Jump;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Attack;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_HeavyAttack;
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> IMC_Default;

	TArray<FString> EventLog;
	FVector2D LastMove = FVector2D::ZeroVector;
	int32 MoveTriggeredCount = 0;
	float HeavyHoldTime = 0.f;
};
