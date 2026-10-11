#include "D0116_ModeCharacter.h"

#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PhysicsVolume.h"
#include "GameFramework/PlayerController.h"

AD0116_ModeCharacter::AD0116_ModeCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxSwimSpeed = 450.f;
	Move->MaxFlySpeed = 900.f;
	Move->BrakingDecelerationSwimming = 600.f;
	Move->BrakingDecelerationFlying = 1500.f;
	Move->Buoyancy = 1.1f;   // 1 보다 크면 물 위로 떠오른다
	Move->GetNavAgentPropertiesRef().bCanSwim = true;
	Move->GetNavAgentPropertiesRef().bCanFly = true;
}

void AD0116_ModeCharacter::ToggleFlying()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	// ★ 핵심: 이동 모드를 직접 바꾼다. Flying 은 중력이 없고 MaxFlySpeed 로 3D 이동
	Move->SetMovementMode(Move->MovementMode == MOVE_Flying ? MOVE_Falling : MOVE_Flying);
}

void AD0116_ModeCharacter::DoMove(float Right, float Forward)
{
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	if (Controller && bInputEnabled && (Move->IsSwimming() || Move->IsFlying()))
	{
		// 3D 이동: 앞 = 카메라가 보는 방향(피치 포함) → 아래를 보며 W 를 누르면 잠수/하강
		const FRotator View = Controller->GetControlRotation();
		AddMovementInput(View.Vector(), Forward);
		AddMovementInput(FRotationMatrix(FRotator(0.f, View.Yaw, 0.f)).GetUnitAxis(EAxis::Y), Right);
		return;
	}
	Super::DoMove(Right, Forward);
}

void AD0116_ModeCharacter::DoJumpStart()
{
	// 비행·수영 중 Space 는 상승(Tick 에서 처리)
	if (!GetCharacterMovement()->IsFlying() && !GetCharacterMovement()->IsSwimming())
	{
		Super::DoJumpStart();
	}
}

void AD0116_ModeCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	const APlayerController* PC = Cast<APlayerController>(Controller);
	if (PC && (Move->IsFlying() || Move->IsSwimming()))
	{
		if (PC->IsInputKeyDown(EKeys::SpaceBar) || PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom))
		{
			AddMovementInput(FVector::UpVector, 1.f);
		}
		if (PC->IsInputKeyDown(EKeys::LeftControl) || PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Right))
		{
			AddMovementInput(FVector::UpVector, -1.f);
		}
	}
}

void AD0116_ModeCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	static const TCHAR* Names[] = { TEXT("None"), TEXT("Walking"), TEXT("NavWalking"), TEXT("Falling"), TEXT("Swimming"), TEXT("Flying"), TEXT("Custom") };
	const int32 From = FMath::Clamp<int32>(PrevMovementMode, 0, 6);
	const int32 To = FMath::Clamp<int32>(GetCharacterMovement()->MovementMode, 0, 6);
	const APhysicsVolume* Volume = GetCharacterMovement()->GetPhysicsVolume();
	ModeLog.Insert(FString::Printf(TEXT("%6.2f  %s → %s%s"), GetWorld()->GetTimeSeconds(), Names[From], Names[To],
		(To == MOVE_Swimming && Volume) ? TEXT("  (Water Volume 진입)") : TEXT("")), 0);
	if (ModeLog.Num() > 8)
	{
		ModeLog.SetNum(8);
	}
}
