#include "Demo0209_Ragdoll.h"

#include "Animation/AnimSequence.h"
#include "Camera/CameraActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GlossaryCharacter.h"
#include "GlossaryKit.h"
#include "PhysicsDemoUtil.h"

using namespace GlossaryKit;

namespace
{
	const int32 NumDummies = 4;
	const float DummyY0 = 0.f;
	const float DummySpacing = 220.f;
}

ADemo0209_Ragdoll::ADemo0209_Ragdoll()
{
	DemoId = TEXT("0209");
	Controls = TEXT("E : 다음 캐릭터를 래그돌로 죽이기 · Q : 비교용 '죽음 애니메이션만' 재생\n")
		TEXT("R : 모두 되살리기\n")
		TEXT("관찰: 래그돌은 캡슐을 끄고 Mesh 물리를 켜서 관절(PA_Mannequin)대로 축 늘어지며 쓰러진다. 애니메이션은 늘 같은 동작");
	Showcase = TEXT("Manny 가 죽으면 캡슐 콜리전 끄고 Mesh SetSimulatePhysics(true) 래그돌 전환, 죽음 애니메이션과 비교");
	PlayerPawnClass = nullptr;
}

void ADemo0209_Ragdoll::BeginPlay()
{
	Super::BeginPlay();
	if (!DeathAnimation)
	{
		DeathAnimation = LoadAsset<UAnimSequence>(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01"));
	}
	Camera = PhysicsDemoUtil::SpawnFixedCamera(GetWorld(), FVector(-720.f, 420.f, 260.f), FRotator(-12.f, -4.f, 0.f), 70.f);
	SpawnDummies();
	// 래그돌이 부딪혀 볼 상자 몇 개
	SpawnShape(GetWorld(), EShape::Cube, FVector(180.f, DummyY0 + DummySpacing * 1.f, 40.f), FVector(80.f, 120.f, 80.f), Colors::Platform);
	SpawnShape(GetWorld(), EShape::Cube, FVector(180.f, DummyY0 + DummySpacing * 3.f, 40.f), FVector(80.f, 120.f, 80.f), Colors::Platform);
}

void ADemo0209_Ragdoll::SpawnDummies()
{
	for (ACharacter* Dummy : Dummies)
	{
		if (Dummy)
		{
			if (AController* C = Dummy->GetController())
			{
				C->Destroy();
			}
			Dummy->Destroy();
		}
	}
	Dummies.Reset();
	States.Reset();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 i = 0; i < NumDummies; ++i)
	{
		const FVector Loc(0.f, DummyY0 + i * DummySpacing, 100.f);
		// 카메라(-X 쪽)를 바라보게
		AGlossaryCharacter* Dummy = GetWorld()->SpawnActor<AGlossaryCharacter>(Loc, FRotator(0.f, 180.f, 0.f), Params);
		if (!Dummy)
		{
			continue;
		}
		Dummy->SpawnDefaultController();   // AI 컨트롤러가 있어야 CharacterMovement 가 동작(착지)
		Dummies.Add(Dummy);
		States.Add(TEXT("살아 있음"));
	}
	LastAction = TEXT("");
}

ACharacter* ADemo0209_Ragdoll::NextAlive() const
{
	for (int32 i = 0; i < Dummies.Num(); ++i)
	{
		if (States[i] == TEXT("살아 있음"))
		{
			return Dummies[i];
		}
	}
	return nullptr;
}

void ADemo0209_Ragdoll::KillWithRagdoll(ACharacter* Character)
{
	USkeletalMeshComponent* Mesh = Character->GetMesh();
	// 1) 캡슐 콜리전 끄기 — 안 끄면 보이지 않는 캡슐이 서 있어 시체 위로 걸리고, 몸과 캡슐이 서로 밀어낸다
	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 2) 이동 정지
	Character->GetCharacterMovement()->StopMovementImmediately();
	Character->GetCharacterMovement()->DisableMovement();
	// 3) 메시를 Ragdoll 콜리전 프로필로 + Physics Asset(PA_Mannequin) 바디 시뮬레이션 시작
	Mesh->SetCollisionProfileName(TEXT("Ragdoll"));
	Mesh->SetSimulatePhysics(true);
	Mesh->WakeAllRigidBodies();
	Mesh->bBlendPhysics = true;
	// 맞은 방향으로 살짝 밀어 넘어지는 방향을 준다 (카메라 반대쪽 = +X)
	Mesh->AddImpulse(FVector(DeathPush, 0.f, 50.f), TEXT("spine_03"), true);
}

void ADemo0209_Ragdoll::KillWithAnimation(ACharacter* Character)
{
	// 비교: 물리 없이 미리 만든 죽음 애니메이션만 재생 (캡슐도 그대로)
	Character->GetCharacterMovement()->DisableMovement();
	if (DeathAnimation)
	{
		Character->GetMesh()->PlayAnimation(DeathAnimation, false);
	}
}

void ADemo0209_Ragdoll::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PhysicsDemoUtil::KeepViewTarget(this, Camera);

	if (WasKeyPressed(EKeys::R))
	{
		SpawnDummies();
	}
	const bool bRagdoll = WasKeyPressed(EKeys::E) || WasKeyPressed(EKeys::LeftMouseButton);
	const bool bAnim = WasKeyPressed(EKeys::Q);
	if (bRagdoll || bAnim)
	{
		if (ACharacter* Target = NextAlive())
		{
			const int32 Index = Dummies.IndexOfByKey(Target);
			if (bRagdoll)
			{
				KillWithRagdoll(Target);
				States[Index] = TEXT("래그돌");
			}
			else
			{
				KillWithAnimation(Target);
				States[Index] = TEXT("죽음 애니메이션");
			}
			LastAction = FString::Printf(TEXT("%d번 → %s"), Index + 1, *States[Index]);
		}
	}

	FString Line;
	for (int32 i = 0; i < States.Num(); ++i)
	{
		Line += FString::Printf(TEXT("%d:%s  "), i + 1, *States[i]);
	}
	SetStatus(FString::Printf(TEXT("%s\n%s"), *Line, LastAction.IsEmpty() ? TEXT("E 로 래그돌, Q 로 애니메이션 죽음") : *LastAction));
}
