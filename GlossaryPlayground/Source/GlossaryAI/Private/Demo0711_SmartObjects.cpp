#include "Demo0711_SmartObjects.h"

#include "AIController.h"
#include "Camera/CameraActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "D07Common.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GlossaryKit.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "SmartObjectComponent.h"
#include "SmartObjectRequestTypes.h"
#include "SmartObjectSubsystem.h"

using namespace GlossaryKit;

// ============================================================ 정의 채우기

namespace D0711
{
	void FillBenchDefinition(USmartObjectDefinition* Definition)
	{
		// 슬롯·기본 행동 배열은 에디터 전용 setter 만 있어서 리플렉션으로 비운다(빌더를 여러 번 돌려도 같은 결과)
		if (TArray<FSmartObjectSlotDefinition>* Slots = static_cast<TArray<FSmartObjectSlotDefinition>*>(D07::FindPropertyPtr(Definition, TEXT("Slots"))))
		{
			Slots->Reset();
		}
		for (int32 Index = 0; Index < 3; ++Index)
		{
			FSmartObjectSlotDefinition& Slot = Definition->DebugAddSlot();
			Slot.Name = *FString::Printf(TEXT("Seat%d"), Index);
			Slot.ID = FGuid::NewGuid();
			Slot.Offset = FVector3f(-110.f + 110.f * Index, 75.f, 0.f);   // 벤치 앞 (벤치 기준 로컬)
			Slot.Rotation = FRotator3f(0.f, 90.f, 0.f);                   // 벤치 반대쪽(+Y)을 보고 앉는다
			Slot.DEBUG_DrawColor = FColor::Green;
			// 슬롯 선택 조건(World Condition)의 스키마 — 에디터에서 슬롯을 추가할 때 자동으로 채워지는 값
			Slot.SelectionPreconditions.SetSchemaClass(Definition->GetWorldConditionSchemaClass());
		}
		if (!Definition->GetPreconditions().GetSchemaClass())
		{
			Definition->GetMutablePreconditions().SetSchemaClass(Definition->GetWorldConditionSchemaClass());
		}
		if (TArray<TObjectPtr<USmartObjectBehaviorDefinition>>* Behaviors =
			static_cast<TArray<TObjectPtr<USmartObjectBehaviorDefinition>>*>(D07::FindPropertyPtr(Definition, TEXT("DefaultBehaviorDefinitions"))))
		{
			Behaviors->Reset();
			Behaviors->Add(NewObject<UD0711_SitBehaviorDefinition>(Definition, NAME_None, RF_Public | RF_Transactional));
		}
	}
}

// ============================================================ 데모

ADemo0711_SmartObjects::ADemo0711_SmartObjects()
{
	DemoId = TEXT("0711");
	Controls = TEXT("V 카메라 전환 (WASD 이동)\n")
		TEXT("관찰: 초록 NPC 들이 벤치의 빈 자리를 찾아 예약(노랑) → 앉아 사용(빨강) → 6초 뒤 반납(초록).\n")
		TEXT("자리 3개가 다 차면 나머지는 '빈 자리 없음' 으로 돌아다니며 기다린다");
	Showcase = TEXT("SOD_Bench(슬롯 3개) Smart Object 벤치를 NPC 5명이 Find → Claim → Occupied → Free 순서로 나눠 쓴다");
	bNeedsNavMesh = true;
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-900.f, -900.f, 100.f));
}

void ADemo0711_SmartObjects::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	BenchDefinition = LoadAsset<USmartObjectDefinition>(D0711::DefinitionPath);
	bDefinitionFromAsset = BenchDefinition != nullptr;
	if (!BenchDefinition)
	{
		BenchDefinition = NewObject<USmartObjectDefinition>(this);
		D0711::FillBenchDefinition(BenchDefinition);
	}

	// 벤치: 좌석 + 등받이. 여기에 Smart Object 컴포넌트를 붙이면 서브시스템에 "사용할 수 있는 사물" 로 등록된다
	Bench = SpawnBlock(World, FVector(400.f, 0.f, 0.f), FVector(360.f, 80.f, 45.f), FLinearColor(0.45f, 0.28f, 0.12f));
	SpawnBlock(World, FVector(400.f, -35.f, 45.f), FVector(360.f, 12.f, 60.f), FLinearColor(0.40f, 0.24f, 0.10f));
	BenchSmartObject = NewObject<USmartObjectComponent>(Bench, TEXT("SmartObject"));
	BenchSmartObject->SetupAttachment(Bench->GetRootComponent());
	BenchSmartObject->SetDefinition(BenchDefinition);
	BenchSmartObject->RegisterComponent();
	SpawnLabel(World, TEXT("벤치 (Smart Object · 자리 3)"), FVector(400.f, 0.f, 230.f), Colors::Item, 20.f);

	for (int32 Index = 0; Index < NumNPCs; ++Index)
	{
		const float Angle = Index * 2.f * PI / NumNPCs;
		AD07Enemy* Npc = D07::SpawnEnemy(World, FVector(400.f + FMath::Cos(Angle) * 900.f, FMath::Sin(Angle) * 900.f, 0.f), 0.f, AAIController::StaticClass(), Colors::Ally);
		Npc->GetCharacterMovement()->MaxWalkSpeed = 220.f;
		NPCs.Add(Npc);
		FNpcBrain Brain;
		Brain.Timer = 0.6f + Index * 0.7f;   // 순서대로 자리를 찾기 시작
		Brains.Add(Brain);
	}

	OverviewCamera = D07::SetupOverviewCamera(this, FVector(-900.f, 0.f, 1700.f), FRotator(-48.f, 0.f, 0.f));
}

void ADemo0711_SmartObjects::Wander(AD07Enemy* Npc)
{
	AAIController* AIC = Cast<AAIController>(Npc->GetController());
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation Point;
	if (AIC && NavSys && NavSys->GetRandomReachablePointInRadius(FVector(400.f, 300.f, 0.f), 1100.f, Point))
	{
		AIC->MoveToLocation(Point.Location, 50.f);
	}
}

void ADemo0711_SmartObjects::SetSeated(AD07Enemy* Npc, bool bSeated, const FTransform& SlotTransform)
{
	// 앉는 애니메이션 대신: 몸을 슬롯 방향으로 돌리고 메시를 좌석 높이로 내려 뒤로 붙인다
	USkeletalMeshComponent* Mesh = Npc->GetMesh();
	if (bSeated)
	{
		Npc->SetActorRotation(FRotator(0.f, SlotTransform.Rotator().Yaw, 0.f));
		Npc->GetCharacterMovement()->DisableMovement();
		Mesh->SetRelativeLocation(FVector(-45.f, 0.f, -96.f - 42.f));
	}
	else
	{
		Npc->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		Mesh->SetRelativeLocation(FVector(0.f, 0.f, -96.f));
	}
}

void ADemo0711_SmartObjects::TickNpc(int32 Index, float DeltaSeconds)
{
	AD07Enemy* Npc = NPCs[Index];
	FNpcBrain& Brain = Brains[Index];
	AAIController* AIC = Npc ? Cast<AAIController>(Npc->GetController()) : nullptr;
	USmartObjectSubsystem* Subsystem = USmartObjectSubsystem::GetCurrent(GetWorld());
	if (!AIC || !Subsystem)
	{
		return;
	}
	Brain.Timer -= DeltaSeconds;

	switch (Brain.State)
	{
	case ENpcState::Wander:
		if (Brain.Timer <= 0.f)
		{
			// 1) 찾기: 주변 상자 안에서 이 NPC 가 쓸 수 있는 빈 슬롯
			FSmartObjectRequestFilter Filter;
			const FSmartObjectRequest Request(FBox::BuildAABB(Npc->GetActorLocation(), FVector(3000.f, 3000.f, 500.f)), Filter);
			TArray<FSmartObjectRequestResult> Results;
			Subsystem->FindSmartObjects(Request, Results, FConstStructView::Make(FSmartObjectActorUserData(Npc)));
			if (Results.Num() > 0)
			{
				// 2) 예약: 다른 NPC 가 같은 자리를 고르지 못하게 Claim
				Brain.Claim = Subsystem->MarkSlotAsClaimed(Results[0].SlotHandle, ESmartObjectClaimPriority::Normal, FConstStructView::Make(FSmartObjectActorUserData(Npc)));
			}
			if (Brain.Claim.IsValid())
			{
				const TOptional<FTransform> SlotTransform = Subsystem->GetSlotTransform(Brain.Claim);
				AIC->MoveToLocation(SlotTransform.IsSet() ? SlotTransform->GetLocation() : Bench->GetActorLocation(), 15.f);
				Brain.State = ENpcState::GoingToSlot;
				Brain.Note = TEXT("자리 예약 (Claimed) → 이동");
			}
			else
			{
				++FailedSearches;
				Brain.Note = TEXT("빈 자리 없음 → 대기");
				Brain.Timer = 2.5f;
				Wander(Npc);
			}
		}
		break;

	case ENpcState::GoingToSlot:
		if (AIC->GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			// 3) 사용 시작: Occupied 로 바꾸고 슬롯의 행동 정의(앉아 있는 시간)를 받는다
			const UD0711_SitBehaviorDefinition* Sit = Subsystem->MarkSlotAsOccupied<UD0711_SitBehaviorDefinition>(Brain.Claim);
			const TOptional<FTransform> SlotTransform = Subsystem->GetSlotTransform(Brain.Claim);
			if (SlotTransform.IsSet())
			{
				Npc->SetActorLocation(SlotTransform->GetLocation() + FVector(0.f, 0.f, 98.f));
				SetSeated(Npc, true, *SlotTransform);
			}
			Brain.Timer = Sit ? Sit->SitDuration : 6.f;
			Brain.State = ENpcState::Sitting;
			Brain.Note = TEXT("앉음 (Occupied)");
			++Uses;
		}
		break;

	case ENpcState::Sitting:
		if (Brain.Timer <= 0.f)
		{
			// 4) 반납: 다른 NPC 가 다시 찾을 수 있게 Free
			Subsystem->MarkSlotAsFree(Brain.Claim);
			Brain.Claim.Invalidate();
			SetSeated(Npc, false, FTransform::Identity);
			Brain.State = ENpcState::Wander;
			Brain.Timer = 5.f;
			Brain.Note = TEXT("일어나 반납 (Free) → 산책");
			Wander(Npc);
		}
		break;
	}

	const FLinearColor Color = Brain.State == ENpcState::Sitting ? Colors::Enemy : Brain.State == ENpcState::GoingToSlot ? Colors::Item : Colors::Ally;
	Npc->SetMarkerColor(Color);
	Npc->SetLabel(Brain.Note, Color);
}

void ADemo0711_SmartObjects::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::V))
	{
		D07::ToggleOverviewCamera(this, OverviewCamera);
	}
	for (int32 Index = 0; Index < NPCs.Num(); ++Index)
	{
		TickNpc(Index, DeltaSeconds);
	}

	USmartObjectSubsystem* Subsystem = USmartObjectSubsystem::GetCurrent(GetWorld());
	if (!Subsystem || !BenchSmartObject)
	{
		return;
	}
	// 슬롯 상태 표시: Free 초록 · Claimed 노랑 · Occupied 빨강
	if (CachedSlots.Num() == 0 && BenchSmartObject->GetRegisteredHandle().IsValid())
	{
		FSmartObjectRequestFilter Filter;
		Filter.bShouldIncludeClaimedSlots = true;
		Subsystem->FindSlots(BenchSmartObject->GetRegisteredHandle(), Filter, CachedSlots);
	}
	int32 Free = 0, Claimed = 0, Occupied = 0;
	for (const FSmartObjectSlotHandle& Slot : CachedSlots)
	{
		const ESmartObjectSlotState State = Subsystem->GetSlotState(Slot);
		const TOptional<FTransform> Transform = Subsystem->GetSlotTransform(Slot);
		FColor Color = FColor::Green;
		if (State == ESmartObjectSlotState::Claimed) { Color = FColor::Yellow; ++Claimed; }
		else if (State == ESmartObjectSlotState::Occupied) { Color = FColor::Red; ++Occupied; }
		else { ++Free; }
		if (Transform.IsSet())
		{
			const FVector P = Transform->GetLocation() + FVector(0.f, 0.f, 4.f);
			DrawDebugCircle(GetWorld(), P, 40.f, 24, Color, false, -1.f, 0, 4.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
			DrawDebugDirectionalArrow(GetWorld(), P, P + Transform->GetRotation().GetForwardVector() * 70.f, 30.f, Color, false, -1.f, 0, 3.f);
		}
	}
	SetStatus(FString::Printf(TEXT("정의 %s · 등록 %s · 슬롯 %d개: Free %d · Claimed %d · Occupied %d\n사용 %d회 · 빈 자리 없어 기다린 횟수 %d"),
		bDefinitionFromAsset ? TEXT("SOD_Bench(에셋)") : TEXT("(실행 중 생성)"),
		BenchSmartObject->GetRegisteredHandle().IsValid() ? TEXT("됨") : TEXT("안 됨"),
		CachedSlots.Num(), Free, Claimed, Occupied, Uses, FailedSearches));
}
