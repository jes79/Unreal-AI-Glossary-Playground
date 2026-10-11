#include "Demo0307_LayeredBlend.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "D03_AnimInstance.h"
#include "D03_Mannequin.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

namespace
{
	/** 캐릭터에 C++ AnimInstance 를 걸고 UpperBody 슬롯 + 레이어 설정 */
	UD03_AnimInstance* SetupLayered(ACharacter* C, FName Slot, FName Bone, bool bLayered)
	{
		if (!C) { return nullptr; }
		C->GetMesh()->SetAnimInstanceClass(UD03_AnimInstance::StaticClass());
		UD03_AnimInstance* Anim = Cast<UD03_AnimInstance>(C->GetMesh()->GetAnimInstance());
		if (Anim)
		{
			Anim->SlotName = Slot;           // Slot 'UpperBody'
			Anim->LayerBone = Bone;          // Layered Blend per Bone: Branch Filter = spine_01
			Anim->bLayeredBlend = bLayered;
			// 공격 시퀀스의 Root Motion 이 걷는 캐릭터를 밀지 않도록 무시
			Anim->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
		}
		return Anim;
	}
}

ADemo0307_LayeredBlend::ADemo0307_LayeredBlend()
{
	DemoId = TEXT("0307");
	Controls = TEXT("WASD 이동 · E : 플레이어 상체 공격 · C : 플레이어 레이어 ON/OFF\n")
		TEXT("관찰: 두 NPC 가 걸으면서 같은 공격을 재생한다. 왼쪽(초록)은 spine_01 위만 공격하고 다리는 계속 걷는다,\n")
		TEXT("오른쪽(빨강)은 Layered Blend 없이 슬롯이 전신을 덮어 다리가 멈춘 채 미끄러진다");
	Showcase = TEXT("C++ AnimInstance 에서 UpperBody 슬롯 + spine_01 Layered Blend per Bone, 걸으며 상체만 공격 vs 전신 덮어쓰기 비교");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-750.f, 0.f, 100.f));
}

void ADemo0307_LayeredBlend::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	AttackAnim = D03::LoadAnim(TEXT("Unarmed/Attack/MM_Attack_01"));

	Layered = D03::SpawnMannequin(World, FVector(-100.f, -450.f, 0.f), 90.f);
	FullBody = D03::SpawnMannequin(World, FVector(200.f, -450.f, 0.f), 90.f);
	SetupLayered(Layered, UpperBodySlot, LayerBone, true);
	SetupLayered(FullBody, UpperBodySlot, LayerBone, false);
	if (Layered)
	{
		Layered->SetPatrol(FVector(-100.f, -450.f, 0.f), FVector(-100.f, 450.f, 0.f));
		Layered->AutoMoveScale = 0.45f;
		Layered->SetHeadLabel(TEXT("Layered Blend ON (spine_01~)"), Colors::Ally, 16.f);
	}
	if (FullBody)
	{
		FullBody->SetPatrol(FVector(200.f, -450.f, 0.f), FVector(200.f, 450.f, 0.f));
		FullBody->AutoMoveScale = 0.45f;
		FullBody->SetHeadLabel(TEXT("OFF: 슬롯이 전신을 덮음"), Colors::Enemy, 16.f);
	}
	SetupLayered(GetPlayerCharacter(), UpperBodySlot, LayerBone, true);
}

void ADemo0307_LayeredBlend::Attack(ACharacter* Who)
{
	UAnimInstance* Anim = Who ? Who->GetMesh()->GetAnimInstance() : nullptr;
	if (Anim && AttackAnim)
	{
		// 몽타주 에셋 없이 시퀀스를 UpperBody 슬롯에 바로 재생
		Anim->PlaySlotAnimationAsDynamicMontage(AttackAnim, UpperBodySlot, 0.15f, 0.25f);
	}
}

void ADemo0307_LayeredBlend::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Timer += DeltaSeconds;
	if (Timer >= AttackInterval)
	{
		Timer = 0.f;
		Attack(Layered);
		Attack(FullBody);
	}
	if (WasKeyPressed(EKeys::E))
	{
		Attack(GetPlayerCharacter());
	}
	if (WasKeyPressed(EKeys::C))
	{
		bPlayerLayered = !bPlayerLayered;
		if (ACharacter* P = GetPlayerCharacter())
		{
			if (UD03_AnimInstance* A = Cast<UD03_AnimInstance>(P->GetMesh()->GetAnimInstance()))
			{
				A->bLayeredBlend = bPlayerLayered;
			}
		}
	}

	auto SlotWeight = [this](ACharacter* C)
	{
		UAnimInstance* A = C ? C->GetMesh()->GetAnimInstance() : nullptr;
		return A ? A->GetSlotMontageGlobalWeight(UpperBodySlot) : 0.f;
	};
	auto LegSpeed = [](ACharacter* C) { return C ? C->GetVelocity().Size2D() : 0.f; };
	SetStatus(FString::Printf(TEXT("왼쪽 ON : UpperBody 슬롯 가중치 %.2f · 이동 %.0f cm/s (다리는 Blend Space 걷기)\n오른쪽 OFF: UpperBody 슬롯 가중치 %.2f · 이동 %.0f cm/s (다리도 공격 포즈)\n플레이어 레이어 %s"),
		SlotWeight(Layered), LegSpeed(Layered), SlotWeight(FullBody), LegSpeed(FullBody), bPlayerLayered ? TEXT("ON") : TEXT("OFF")));
}
