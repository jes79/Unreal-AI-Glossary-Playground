#include "D03_Mannequin.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "D03_AnimInstance.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"

AD03_Mannequin::AD03_Mannequin(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::Disabled;
	// 컨트롤러 없이도 CharacterMovement 가 이동·중력을 계산하게
	GetCharacterMovement()->bRunPhysicsWithNoController = true;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
}

void AD03_Mannequin::BeginPlay()
{
	Super::BeginPlay();
}

void AD03_Mannequin::EndPlay(const EEndPlayReason::Type Reason)
{
	if (HeadLabel)
	{
		HeadLabel->Destroy();
	}
	Super::EndPlay(Reason);
}

UD03_AnimInstance* AD03_Mannequin::UseCppAnimInstance()
{
	GetMesh()->SetAnimInstanceClass(UD03_AnimInstance::StaticClass());
	return Cast<UD03_AnimInstance>(GetMesh()->GetAnimInstance());
}

void AD03_Mannequin::UseTemplateAnimBP()
{
	GetMesh()->SetAnimInstanceClass(D03::GetUnarmedAnimClass());
}

void AD03_Mannequin::SetPatrol(const FVector& A, const FVector& B)
{
	PatrolA = A;
	PatrolB = B;
	bPatrolToB = true;
}

void AD03_Mannequin::SetFixedFacing(bool bFixed, float Yaw)
{
	bFixedFacing = bFixed;
	FixedYaw = Yaw;
	GetCharacterMovement()->bOrientRotationToMovement = !bFixed;
	if (bFixed)
	{
		SetActorRotation(FRotator(0.f, Yaw, 0.f));
	}
}

void AD03_Mannequin::SetHeadLabel(const FString& Text, const FLinearColor& Color, float Size)
{
	if (!HeadLabel)
	{
		HeadLabel = GlossaryKit::SpawnLabel(GetWorld(), Text, GetActorLocation() + FVector(0, 0, 130.f), Color, Size);
		if (HeadLabel)
		{
			HeadLabel->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		}
		return;
	}
	HeadLabel->SetText(Text);
	HeadLabel->SetColor(Color);
}

void AD03_Mannequin::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 왕복 순찰: 목표점 근처에 오면 방향을 바꾼다
	if (!PatrolA.Equals(PatrolB))
	{
		const FVector Target = bPatrolToB ? PatrolB : PatrolA;
		FVector ToTarget = Target - GetActorLocation();
		ToTarget.Z = 0.f;
		if (ToTarget.Size() < 60.f)
		{
			bPatrolToB = !bPatrolToB;
		}
		AutoMoveDirection = ToTarget.GetSafeNormal();
	}
	if (!AutoMoveDirection.IsNearlyZero() && AutoMoveScale > 0.f)
	{
		AddMovementInput(AutoMoveDirection, AutoMoveScale);
	}
	if (bFixedFacing)
	{
		SetActorRotation(FRotator(0.f, FixedYaw, 0.f));
	}
}

// ---------------------------------------------------------------- 헬퍼

UAnimSequence* D03::LoadAnim(const FString& RelPath)
{
	return GlossaryKit::LoadAsset<UAnimSequence>(TEXT("/Game/Characters/Mannequins/Anims/") + RelPath);
}

UAnimMontage* D03::LoadMontage(const FString& AssetName)
{
	return GlossaryKit::LoadAsset<UAnimMontage>(Folder + AssetName);
}

UClass* D03::GetUnarmedAnimClass()
{
	static TWeakObjectPtr<UClass> Cached;
	if (!Cached.IsValid())
	{
		Cached = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"));
	}
	return Cached.Get();
}

AD03_Mannequin* D03::SpawnMannequin(UWorld* World, const FVector& FeetLocation, float Yaw)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	return World->SpawnActor<AD03_Mannequin>(AD03_Mannequin::StaticClass(), FeetLocation + FVector(0, 0, 97.f), FRotator(0.f, Yaw, 0.f), Params);
}

ACameraActor* D03::SpawnViewCamera(UWorld* World, APlayerController* PC, const FVector& Location, const FVector& LookAt, float FOV)
{
	ACameraActor* Cam = World->SpawnActor<ACameraActor>(Location, (LookAt - Location).Rotation());
	if (Cam)
	{
		Cam->GetCameraComponent()->SetFieldOfView(FOV);
		Cam->GetCameraComponent()->bConstrainAspectRatio = false;
		if (PC)
		{
			PC->bAutoManageActiveCameraTarget = false;
			PC->SetViewTarget(Cam);
		}
	}
	return Cam;
}
