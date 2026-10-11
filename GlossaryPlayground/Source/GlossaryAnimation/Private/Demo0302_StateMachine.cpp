#include "Demo0302_StateMachine.h"

#include "Animation/AnimClassInterface.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimStateMachineTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "D03_GraphWidget.h"
#include "D03_Mannequin.h"
#include "GameFramework/Character.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0302_StateMachine::ADemo0302_StateMachine()
{
	DemoId = TEXT("0302");
	Controls = TEXT("WASD 이동 · Space 점프 (플레이어도 같은 ABP_Unarmed)\n")
		TEXT("관찰: NPC 가 서기→걷기→점프→착지를 반복하면 오른쪽 아래 다이어그램의 현재 상태가 노랗게 바뀐다\n")
		TEXT("상태 이름은 ABP_Unarmed 의 실제 State Machine 에서 GetCurrentStateName 으로 읽은 값");
	Showcase = TEXT("ABP_Unarmed State Machine 의 현재 상태를 실시간 조회해 Idle·Locomotion·Jump·Fall·Land 다이어그램에 강조, 전이 조건 표시");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-700.f, 0.f, 100.f));
}

void ADemo0302_StateMachine::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	Npc = D03::SpawnMannequin(World, FVector(100.f, -350.f, 0.f), 90.f);
	SpawnLabel(World, TEXT("State Machine: Idle ⇄ Locomotion → Jump → Fall → Land"), FVector(400.f, 0.f, 420.f), Colors::Item, 20.f);
	Diagram = UD03_GraphWidget::Create(GetPC(), TEXT("ABP_Unarmed State Machine (실시간)"));
	if (Diagram)
	{
		Diagram->PanelSize = FVector2D(600.f, 330.f);
		Diagram->XMin = 0.f; Diagram->XMax = 100.f; Diagram->YMin = 0.f; Diagram->YMax = 60.f;
		Diagram->GridX = 0; Diagram->GridY = 0;
	}
}

FString ADemo0302_StateMachine::ReadStates(ACharacter* Character, FString& OutLeaf) const
{
	// AnimBP 클래스에 구워진(Baked) State Machine 목록을 돌며 각 머신의 현재 상태 이름을 읽는다
	FString Result;
	TArray<FString> States;
	UAnimInstance* Anim = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	const IAnimClassInterface* AnimClass = Anim ? IAnimClassInterface::GetFromClass(Anim->GetClass()) : nullptr;
	if (!AnimClass)
	{
		return TEXT("(AnimBP 없음)");
	}
	for (const FBakedAnimationStateMachine& Machine : AnimClass->GetBakedStateMachines())
	{
		const int32 Index = Anim->GetStateMachineIndex(Machine.MachineName);
		if (Index == INDEX_NONE || !Anim->GetStateMachineInstance(Index))
		{
			continue;
		}
		const FName State = Anim->GetCurrentStateName(Index);
		Result += FString::Printf(TEXT("%s%s → %s"), Result.IsEmpty() ? TEXT("") : TEXT("  ·  "), *Machine.MachineName.ToString(), *State.ToString());
		States.Add(State.ToString());
	}
	// 다이어그램에 쓸 대표 상태: 공중 상태가 가장 우선, 다음 Idle, 나머지는 Locomotion
	for (const TCHAR* Key : { TEXT("Jump"), TEXT("Fall"), TEXT("Land"), TEXT("Idle") })
	{
		for (const FString& S : States)
		{
			if (OutLeaf.IsEmpty() && S.Contains(Key)) { OutLeaf = S; }
		}
	}
	if (OutLeaf.IsEmpty())
	{
		OutLeaf = States.Num() > 0 ? States.Last() : TEXT("Locomotion");
	}
	return Result;
}

void ADemo0302_StateMachine::DrawDiagram(const FString& Leaf)
{
	if (!Diagram)
	{
		return;
	}
	Diagram->ClearDynamic();
	// 실제 상태 이름 → 다이어그램 상자
	FString Active = TEXT("Locomotion");
	if (Leaf.Contains(TEXT("Jump"))) { Active = TEXT("Jump"); }
	else if (Leaf.Contains(TEXT("Fall"))) { Active = TEXT("Fall"); }
	else if (Leaf.Contains(TEXT("Land"))) { Active = TEXT("Land"); }
	else if (Leaf.Contains(TEXT("Idle"))) { Active = TEXT("Idle"); }

	struct FBox { const TCHAR* Name; float X0, Y0; };
	const FBox Boxes[] = { { TEXT("Idle"), 3, 40 }, { TEXT("Locomotion"), 38, 40 }, { TEXT("Jump"), 76, 40 }, { TEXT("Fall"), 76, 8 }, { TEXT("Land"), 38, 8 } };
	for (const FBox& B : Boxes)
	{
		const bool bOn = Active == B.Name;
		Diagram->Bars.Add({ B.X0, B.X0 + 22.f, B.Y0, B.Y0 + 13.f,
			bOn ? FLinearColor(0.95f, 0.7f, 0.05f, 0.95f) : FLinearColor(0.2f, 0.25f, 0.35f, 0.9f), B.Name });
	}
	const FLinearColor Arrow(0.6f, 0.8f, 1.f, 0.8f);
	auto Link = [&](FVector2D A, FVector2D B, const TCHAR* Cond, FVector2D TextAt)
	{
		Diagram->Lines.Add({ { A, B }, Arrow, 2.f });
		// 화살촉: 끝점 근처 작은 점
		Diagram->Points.Add({ B, Arrow, TEXT(""), 7.f });
		Diagram->Texts.Add({ TextAt, Cond, FLinearColor(0.75f, 0.9f, 1.f), 10.f });
	};
	Link(FVector2D(25, 49), FVector2D(38, 49), TEXT("ShouldMove"), FVector2D(23, 59));
	Link(FVector2D(38, 43), FVector2D(25, 43), TEXT("!ShouldMove"), FVector2D(23, 40));
	Link(FVector2D(60, 47), FVector2D(76, 47), TEXT("IsFalling & Vz>0"), FVector2D(58, 59));
	Link(FVector2D(87, 40), FVector2D(87, 21), TEXT("남은 시간<0.1"), FVector2D(88, 34));
	Link(FVector2D(76, 14), FVector2D(60, 14), TEXT("!IsFalling"), FVector2D(62, 7));
	Link(FVector2D(49, 21), FVector2D(49, 40), TEXT("착지 끝"), FVector2D(50, 34));
	Link(FVector2D(60, 42), FVector2D(78, 21), TEXT(""), FVector2D(0, 0));
	Diagram->Texts.Add({ FVector2D(64, 30), TEXT("IsFalling(낙하)"), FLinearColor(0.75f, 0.9f, 1.f), 10.f });
}

void ADemo0302_StateMachine::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// NPC 행동 스크립트: 상태가 골고루 나오도록
	CycleTime += DeltaSeconds;
	if (CycleTime >= CycleLength)
	{
		CycleTime = 0.f;
		bJumpedMoving = bJumpedIdle = false;
		if (Npc) { Npc->SetActorLocation(FVector(100.f, -350.f, 97.f)); }
	}
	if (Npc)
	{
		const bool bMove = CycleTime >= 1.5f && CycleTime < 4.5f;
		Npc->AutoMoveDirection = bMove ? FVector(0.f, 1.f, 0.f) : FVector::ZeroVector;
		Npc->AutoMoveScale = 0.45f;
		if (!bJumpedMoving && CycleTime >= 2.6f) { bJumpedMoving = true; Npc->Jump(); }
		if (!bJumpedIdle && CycleTime >= 5.4f) { bJumpedIdle = true; Npc->Jump(); }
	}

	FString NpcLeaf, PlayerLeaf;
	const FString NpcStates = ReadStates(Npc, NpcLeaf);
	const FString PlayerStates = ReadStates(GetPlayerCharacter(), PlayerLeaf);
	DrawDiagram(NpcLeaf);
	if (Npc)
	{
		Npc->SetHeadLabel(NpcLeaf, Colors::Item, 18.f);
	}
	SetStatus(FString::Printf(TEXT("NPC     %s\n플레이어 %s"), *NpcStates, *PlayerStates));
}
