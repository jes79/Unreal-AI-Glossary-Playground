#include "Demo0301_AnimBlueprint.h"

#include "Components/SkeletalMeshComponent.h"
#include "D03_AnimInstance.h"
#include "D03_Mannequin.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GlossaryKit.h"
#include "KismetAnimationLibrary.h"

using namespace GlossaryKit;

ADemo0301_AnimBlueprint::ADemo0301_AnimBlueprint()
{
	DemoId = TEXT("0301");
	Controls = TEXT("WASD 이동 · Space 점프 · 마우스 시점\n")
		TEXT("1 : C++ AnimInstance (NativeUpdateAnimation 에서 Speed·IsFalling 계산)   2 : 템플릿 ABP_Unarmed\n")
		TEXT("관찰: 걷거나 점프하면 Speed·IsFalling 이 매 프레임 바뀌고, 그 값으로 Blend Space·Fall 포즈가 골라진다");
	Showcase = TEXT("C++ AnimInstance 가 Event Graph 처럼 Speed·IsFalling·Direction 을 계산, 플레이어·NPC 머리 위 실시간 표시, ABP_Unarmed 와 전환");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-700.f, 0.f, 100.f));
}

void ADemo0301_AnimBlueprint::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 순찰하며 주기적으로 점프하는 NPC — 같은 C++ AnimInstance 를 쓴다
	Npc = D03::SpawnMannequin(World, FVector(150.f, -200.f, 0.f), 90.f);
	if (Npc)
	{
		Npc->SetPatrol(FVector(150.f, -200.f, 0.f), FVector(150.f, 450.f, 0.f));
		Npc->AutoMoveScale = 0.6f;
	}

	SpawnLabel(World, TEXT("Event Graph (NativeUpdateAnimation)\nSpeed = Velocity.Size2D()\nIsFalling = Movement->IsFalling()"),
		FVector(700.f, 500.f, 420.f), Colors::Item, 18.f);
	// 점프해 볼 낮은 단
	SpawnBlock(World, FVector(-400.f, 700.f, 0.f), FVector(300.f, 300.f, 60.f), Colors::Platform);

	ApplyAnimClass();
}

void ADemo0301_AnimBlueprint::ApplyAnimClass()
{
	// 플레이어와 NPC 모두 같은 클래스로 바꾼다 (Mesh->SetAnimInstanceClass = 디테일 패널의 Anim Class)
	UClass* AnimClass = bUseCpp ? UD03_AnimInstance::StaticClass() : D03::GetUnarmedAnimClass();
	if (ACharacter* Player = GetPlayerCharacter())
	{
		Player->GetMesh()->SetAnimInstanceClass(AnimClass);
	}
	if (Npc)
	{
		Npc->GetMesh()->SetAnimInstanceClass(AnimClass);
	}
}

void ADemo0301_AnimBlueprint::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (WasKeyPressed(EKeys::One) && !bUseCpp) { bUseCpp = true; ApplyAnimClass(); }
	if (WasKeyPressed(EKeys::Two) && bUseCpp) { bUseCpp = false; ApplyAnimClass(); }

	// NPC 주기 점프
	JumpTimer += DeltaSeconds;
	if (Npc && JumpTimer >= NpcJumpInterval)
	{
		JumpTimer = 0.f;
		Npc->Jump();
	}

	// 같은 Event Graph 계산을 표시용으로 읽는다: C++ 인스턴스면 그 변수를, ABP 면 같은 식으로 계산
	auto Describe = [this](ACharacter* C, float& OutSpeed, float& OutDir, bool& bOutFalling)
	{
		if (const UD03_AnimInstance* Anim = Cast<UD03_AnimInstance>(C->GetMesh()->GetAnimInstance()))
		{
			OutSpeed = Anim->Speed; OutDir = Anim->Direction; bOutFalling = Anim->bIsFalling;
		}
		else
		{
			OutSpeed = C->GetVelocity().Size2D();
			OutDir = UKismetAnimationLibrary::CalculateDirection(C->GetVelocity(), C->GetActorRotation());
			bOutFalling = C->GetCharacterMovement()->IsFalling();
		}
	};

	float NS = 0.f, ND = 0.f; bool bNF = false;
	if (Npc)
	{
		Describe(Npc, NS, ND, bNF);
		Npc->SetHeadLabel(FString::Printf(TEXT("NPC  Speed %.0f · IsFalling %s"), NS, bNF ? TEXT("true") : TEXT("false")),
			bNF ? Colors::Accent : FLinearColor::White);
	}
	float PS = 0.f, PD = 0.f; bool bPF = false;
	if (ACharacter* Player = GetPlayerCharacter())
	{
		Describe(Player, PS, PD, bPF);
	}
	SetStatus(FString::Printf(TEXT("Anim Class: %s\n플레이어  Speed %.0f · Direction %.0f° · IsFalling %s\nNPC      Speed %.0f · IsFalling %s"),
		bUseCpp ? TEXT("UD03_AnimInstance (C++ NativeUpdateAnimation)") : TEXT("ABP_Unarmed (템플릿 AnimBP)"),
		PS, PD, bPF ? TEXT("true") : TEXT("false"), NS, bNF ? TEXT("true") : TEXT("false")));
}
