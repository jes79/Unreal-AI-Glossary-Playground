#include "Demo0111_CameraLag.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GlossaryCharacter.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0111_CameraLag::ADemo0111_CameraLag()
{
	DemoId = TEXT("0111");
	Controls = TEXT("WASD 이동 · 마우스 시점 · R : 원형 트랙 자동 달리기 ON/OFF\n")
		TEXT("C : Camera Lag 전체 ON/OFF · 1/2/3 : Lag Speed 2 / 5 / 10\n")
		TEXT("관찰: Lag ON 이면 달릴 때 캐릭터가 화면 중앙에서 밀려나고 멈추면 카메라가 부드럽게 따라온다");
	Showcase = TEXT("Spring Arm Enable Camera Lag(Speed 5)·Rotation Lag(Speed 10) ON/OFF, 원형 트랙 자동 달리기로 지연 거리 표시");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-100.f, 0.f, 100.f));
}

void ADemo0111_CameraLag::Apply()
{
	if (AGlossaryCharacter* Character = Cast<AGlossaryCharacter>(GetPlayerPawn()))
	{
		USpringArmComponent* Arm = Character->GetCameraBoom();
		// 핵심 네 줄: 위치 지연과 회전 지연을 각각 켜고 속도를 정한다 (값이 작을수록 더 늦게 따라온다)
		Arm->bEnableCameraLag = bEnableCameraLag;
		Arm->CameraLagSpeed = CameraLagSpeed;
		Arm->bEnableCameraRotationLag = bEnableCameraRotationLag;
		Arm->CameraRotationLagSpeed = CameraRotationLagSpeed;
	}
}

void ADemo0111_CameraLag::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 원형 트랙을 기둥으로 표시
	for (int32 i = 0; i < 24; ++i)
	{
		const float A = 2.f * PI * i / 24.f;
		const FVector P = TrackCenter + FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * (TrackRadius + 180.f);
		SpawnShape(World, EShape::Cylinder, P + FVector(0.f, 0.f, 75.f), FVector(40.f, 40.f, 150.f), i % 2 ? Colors::Accent : Colors::Neutral);
	}
	SpawnShape(World, EShape::Cylinder, TrackCenter + FVector(0.f, 0.f, 5.f), FVector(TrackRadius * 2.f - 200.f, TrackRadius * 2.f - 200.f, 10.f), Colors::Platform);
	SpawnLabel(World, TEXT("원형 트랙 (R 자동 달리기)"), TrackCenter + FVector(0.f, 0.f, 200.f), Colors::Item, 18.f);

	// 빨리 달리면 차이가 더 잘 보인다
	if (ACharacter* Character = GetPlayerCharacter())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = 700.f;
	}
	Apply();
}

void ADemo0111_CameraLag::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	bool bChanged = false;
	if (WasKeyPressed(EKeys::C)) { bEnableCameraLag = !bEnableCameraLag; bEnableCameraRotationLag = bEnableCameraLag; bChanged = true; }
	if (WasKeyPressed(EKeys::One)) { CameraLagSpeed = 2.f; bChanged = true; }
	if (WasKeyPressed(EKeys::Two)) { CameraLagSpeed = 5.f; bChanged = true; }
	if (WasKeyPressed(EKeys::Three)) { CameraLagSpeed = 10.f; bChanged = true; }
	if (WasKeyPressed(EKeys::R)) { bAutoRun = !bAutoRun; }
	if (bChanged)
	{
		Apply();
	}

	AGlossaryCharacter* Character = Cast<AGlossaryCharacter>(GetPlayerPawn());
	if (!Character)
	{
		return;
	}

	if (bAutoRun)
	{
		// 트랙 위의 목표점이 원을 따라 돌고, 캐릭터는 그 점을 향해 달린다
		const float Speed = Character->GetCharacterMovement()->MaxWalkSpeed;
		TrackAngle += Speed / TrackRadius * DeltaSeconds;
		const FVector Target = TrackCenter + FVector(FMath::Cos(TrackAngle + 0.3f), FMath::Sin(TrackAngle + 0.3f), 0.f) * TrackRadius;
		Character->AddMovementInput((Target - Character->GetActorLocation()).GetSafeNormal2D(), 1.f);
		// 카메라가 달리는 방향을 바라보도록 Control Rotation 도 함께 돌린다 (회전 지연이 보이도록)
		if (APlayerController* PC = GetPC())
		{
			const FRotator Desired(-20.f, Character->GetVelocity().Rotation().Yaw, 0.f);
			if (!Character->GetVelocity().IsNearlyZero())
			{
				PC->SetControlRotation(FMath::RInterpTo(PC->GetControlRotation(), Desired, DeltaSeconds, 3.f));
			}
		}
	}

	// 지연량: 지연이 없을 때 카메라가 있어야 할 자리 vs 실제 자리
	const USpringArmComponent* Arm = Character->GetCameraBoom();
	const FVector Ideal = Arm->GetComponentLocation() - Character->GetControlRotation().Vector() * Arm->TargetArmLength;
	const float LagDistance = FVector::Dist(Ideal, Character->GetFollowCamera()->GetComponentLocation());
	SetStatus(FString::Printf(TEXT("Camera Lag %s (Speed %.0f) · Rotation Lag %s (Speed %.0f)%s\n카메라가 목표 위치에서 %.0fcm 떨어짐 · 속도 %.0f cm/s"),
		bEnableCameraLag ? TEXT("ON") : TEXT("OFF"), CameraLagSpeed,
		bEnableCameraRotationLag ? TEXT("ON") : TEXT("OFF"), CameraRotationLagSpeed,
		bAutoRun ? TEXT(" · 자동 달리기") : TEXT(""),
		LagDistance, Character->GetVelocity().Size2D()));
}
