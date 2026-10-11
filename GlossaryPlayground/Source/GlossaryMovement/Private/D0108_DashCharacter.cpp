#include "D0108_DashCharacter.h"

#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"
#include "MovementKit.h"

AD0108_DashCharacter::AD0108_DashCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
}

void AD0108_DashCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	if (!IMC_Dash)
	{
		IA_Dash = MovementKit::MakeAction(this, TEXT("IA_Dash"), EInputActionValueType::Boolean);
		IMC_Dash = MovementKit::MakeContext(this, TEXT("IMC_Dash"));
		IMC_Dash->MapKey(IA_Dash, EKeys::LeftShift).Triggers.Add(NewObject<UInputTriggerPressed>(IMC_Dash));
		IMC_Dash->MapKey(IA_Dash, EKeys::Gamepad_FaceButton_Right).Triggers.Add(NewObject<UInputTriggerPressed>(IMC_Dash));
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = MovementKit::GetInputSubsystem(Cast<APlayerController>(Controller)))
	{
		Subsystem->AddMappingContext(IMC_Dash, 0);
	}
}

void AD0108_DashCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (!IA_Dash)
	{
		IA_Dash = MovementKit::MakeAction(this, TEXT("IA_Dash"), EInputActionValueType::Boolean);
	}
	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(IA_Dash, ETriggerEvent::Triggered, this, &AD0108_DashCharacter::Dash);
	}
}

void AD0108_DashCharacter::Dash()
{
	if (CooldownRemaining > 0.f || IsDashing())
	{
		++BlockedByCooldown;   // 쿨타임 중에는 무시
		return;
	}
	UCharacterMovementComponent* Move = GetCharacterMovement();

	// 방향: 지금 입력 방향, 없으면 바라보는 방향
	DashDirection = Move->GetCurrentAcceleration().GetSafeNormal2D();
	if (DashDirection.IsNearlyZero())
	{
		DashDirection = GetActorForwardVector().GetSafeNormal2D();
	}

	// 대시 동안은 중력·마찰·제동을 꺼서 0.2초 동안 일정한 속도로 미끄러지게 한다
	SavedGravityScale = Move->GravityScale;
	SavedBrakingFalling = Move->BrakingDecelerationFalling;
	SavedGroundFriction = Move->GroundFriction;
	SavedBrakingWalking = Move->BrakingDecelerationWalking;
	Move->GravityScale = 0.f;
	Move->BrakingDecelerationFalling = 0.f;
	Move->GroundFriction = 0.f;
	Move->BrakingDecelerationWalking = 0.f;

	// ★ 핵심: LaunchCharacter — XY·Z 를 모두 덮어써서(Override) 순간 속도를 준다
	LaunchCharacter(DashDirection * DashSpeed, true, true);

	DashRemaining = DashDuration;
	CooldownRemaining = DashCooldown;
	InvincibleRemaining = bInvincibilityEnabled ? InvincibleTime : 0.f;
	TrailTimer = 0.f;
	bInputEnabled = false;   // 대시 중에는 방향 전환 불가
	++DashCount;
}

void AD0108_DashCharacter::EndDash()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->GravityScale = SavedGravityScale;
	Move->BrakingDecelerationFalling = SavedBrakingFalling;
	Move->GroundFriction = SavedGroundFriction;
	Move->BrakingDecelerationWalking = SavedBrakingWalking;
	// 남은 속도는 걷기 최고 속도로 줄여 자연스럽게 이어 달린다
	Move->Velocity = DashDirection * Move->MaxWalkSpeed;
	bInputEnabled = true;
}

void AD0108_DashCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	CooldownRemaining = FMath::Max(0.f, CooldownRemaining - DeltaSeconds);
	InvincibleRemaining = FMath::Max(0.f, InvincibleRemaining - DeltaSeconds);
	if (DashRemaining > 0.f)
	{
		DashRemaining -= DeltaSeconds;
		// 잔상: 지나간 자리에 하늘색 구슬
		TrailTimer -= DeltaSeconds;
		if (TrailTimer <= 0.f)
		{
			TrailTimer = 0.02f;
			if (AStaticMeshActor* Ghost = GlossaryKit::SpawnShape(GetWorld(), GlossaryKit::EShape::Sphere,
				GetActorLocation(), FVector(45.f), GlossaryKit::Colors::Cyan))
			{
				Ghost->SetActorEnableCollision(false);
				GlossaryKit::SetColor(Ghost->GetStaticMeshComponent(), IsInvincible() ? GlossaryKit::Colors::Cyan : GlossaryKit::Colors::Neutral, 0.5f, 0.f, 1.5f);
				Ghost->SetLifeSpan(0.3f);
			}
		}
		if (DashRemaining <= 0.f)
		{
			EndDash();
		}
	}
}
