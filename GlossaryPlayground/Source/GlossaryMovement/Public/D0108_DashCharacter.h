#pragma once

#include "CoreMinimal.h"
#include "GlossaryCharacter.h"
#include "D0108_DashCharacter.generated.h"

/**
 * 0108 대시 캐릭터: Shift(패드 B) → LaunchCharacter 로 0.2초 돌진, 그동안 무적, 쿨타임 1초.
 * 대시 키는 실행 중에 만든 IA_Dash / IMC_Dash 로 받는다(템플릿 IMC_Default 에는 대시가 없음).
 */
UCLASS()
class GLOSSARYMOVEMENT_API AD0108_DashCharacter : public AGlossaryCharacter
{
	GENERATED_BODY()

public:
	AD0108_DashCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditAnywhere, Category = "Dash") float DashSpeed = 2500.f;
	UPROPERTY(EditAnywhere, Category = "Dash") float DashDuration = 0.2f;
	UPROPERTY(EditAnywhere, Category = "Dash") float DashCooldown = 1.f;
	/** 무적 시간 (대시 시간과 같게 시작) */
	UPROPERTY(EditAnywhere, Category = "Dash") float InvincibleTime = 0.2f;
	UPROPERTY(EditAnywhere, Category = "Dash") bool bInvincibilityEnabled = true;

	bool IsInvincible() const { return InvincibleRemaining > 0.f; }
	bool IsDashing() const { return DashRemaining > 0.f; }
	float GetCooldownRemaining() const { return CooldownRemaining; }
	int32 GetDashCount() const { return DashCount; }
	int32 GetBlockedByCooldown() const { return BlockedByCooldown; }

	UFUNCTION(BlueprintCallable, Category = "Dash")
	void Dash();

protected:
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;
	void EndDash();

	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Dash;
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> IMC_Dash;

	float DashRemaining = 0.f;
	float InvincibleRemaining = 0.f;
	float CooldownRemaining = 0.f;
	float TrailTimer = 0.f;
	FVector DashDirection = FVector::ForwardVector;
	int32 DashCount = 0;
	int32 BlockedByCooldown = 0;

	// 대시 동안 바꿨다가 되돌릴 값
	float SavedGravityScale = 1.f;
	float SavedBrakingFalling = 0.f;
	float SavedGroundFriction = 8.f;
	float SavedBrakingWalking = 2000.f;
};
