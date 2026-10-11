#include "Demo1115_GameplayDebugger.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/Decorators/BTDecorator_Blackboard.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "BehaviorTree/ValueOrBBKey.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GlossaryKit.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebugger.h"
#include "GameplayDebuggerCategory.h"
#endif

using namespace GlossaryKit;

static const FName KeyTargetActor(TEXT("TargetActor"));
static const FName KeyPatrolLocation(TEXT("PatrolLocation"));

// ================================================================ UD1115_BTTask_NextPatrolPoint

UD1115_BTTask_NextPatrolPoint::UD1115_BTTask_NextPatrolPoint()
{
	NodeName = TEXT("Next Patrol Point");
}

EBTNodeResult::Type UD1115_BTTask_NextPatrolPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* Controller = OwnerComp.GetAIOwner();
	AD1115_Enemy* Enemy = Controller ? Cast<AD1115_Enemy>(Controller->GetPawn()) : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Enemy || !Blackboard || Enemy->PatrolPoints.Num() == 0)
	{
		return EBTNodeResult::Failed;
	}
	Blackboard->SetValueAsVector(PatrolLocationKey, Enemy->PatrolPoints[Enemy->NextPatrolIndex]);
	Enemy->NextPatrolIndex = (Enemy->NextPatrolIndex + 1) % Enemy->PatrolPoints.Num();
	return EBTNodeResult::Succeeded;
}

FString UD1115_BTTask_NextPatrolPoint::GetStaticDescription() const
{
	return FString::Printf(TEXT("다음 순찰 지점 → %s"), *PatrolLocationKey.ToString());
}

// ================================================================ AD1115_Enemy

AD1115_Enemy::AD1115_Enemy()
{
	AIControllerClass = AD1115_EnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 360.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 280.f;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(GetCapsuleComponent());
	InitShapeComponent(Body, EShape::Cylinder, FVector(68.f, 68.f, 176.f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Visor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visor"));
	Visor->SetupAttachment(GetCapsuleComponent());
	InitShapeComponent(Visor, EShape::Cube, FVector(30.f, 50.f, 16.f));
	Visor->SetRelativeLocation(FVector(32.f, 0.f, 55.f));
	Visor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AD1115_Enemy::BeginPlay()
{
	Super::BeginPlay();
	SetColor(Body, Colors::Enemy);
	SetColor(Visor, Colors::Item, 0.4f, 0.f, 3.f);
}

// ================================================================ AD1115_EnemyController

AD1115_EnemyController::AD1115_EnemyController()
{
	// AI Perception: 시야 감각 하나. Gameplay Debugger 의 Perception 카테고리가 이 원뿔과 감지 결과를 그려 준다
	SetPerceptionComponent(*CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception")));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1200.f;
	SightConfig->LoseSightRadius = 1600.f;
	SightConfig->PeripheralVisionAngleDegrees = 45.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->SetMaxAge(2.f);
	GetPerceptionComponent()->ConfigureSense(*SightConfig);
	GetPerceptionComponent()->SetDominantSense(SightConfig->GetSenseImplementation());
}

void AD1115_EnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	GetPerceptionComponent()->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &AD1115_EnemyController::HandleTargetPerception);
	RuntimeTree = BuildBehaviorTree();
	RunBehaviorTree(RuntimeTree);
}

void AD1115_EnemyController::HandleTargetPerception(AActor* Actor, FAIStimulus Stimulus)
{
	if (Actor != UGameplayStatics::GetPlayerPawn(this, 0) || !GetBlackboardComponent())
	{
		return;
	}
	if (Stimulus.WasSuccessfullySensed())
	{
		GetBlackboardComponent()->SetValueAsObject(KeyTargetActor, Actor); // → Chase 가지의 데코레이터가 통과, 순찰을 중단(Abort)
	}
	else
	{
		GetBlackboardComponent()->ClearValue(KeyTargetActor);              // 놓치면 순찰로 복귀
	}
}

namespace
{
	/**
	 * 엔진 노드의 protected 설정값(BlackboardKey·AcceptableRadius 등)을 리플렉션으로 채운다.
	 * BT 에디터의 디테일 패널이 하는 일과 같은 경로라서, 엔진 클래스를 상속하지 않고도 노드를 설정할 수 있다.
	 */
	template <typename T>
	T* PropertyPtr(UObject* Object, FName PropertyName)
	{
		FProperty* Property = Object->GetClass()->FindPropertyByName(PropertyName);
		checkf(Property, TEXT("%s 에 %s 프로퍼티가 없음"), *Object->GetClass()->GetName(), *PropertyName.ToString());
		return Property->ContainerPtrToValuePtr<T>(Object);
	}

	UBTTask_MoveTo* MakeMoveTo(UBehaviorTree* Tree, FName Key, float AcceptableRadius, const TCHAR* Name)
	{
		UBTTask_MoveTo* Move = NewObject<UBTTask_MoveTo>(Tree);
		Move->NodeName = Name;
		PropertyPtr<FBlackboardKeySelector>(Move, TEXT("BlackboardKey"))->SelectedKeyName = Key;
		*PropertyPtr<FValueOrBBKey_Float>(Move, TEXT("AcceptableRadius")) = FValueOrBBKey_Float(AcceptableRadius);
		return Move;
	}

	void AddTaskChild(UBTCompositeNode* Parent, UBTTaskNode* Task)
	{
		FBTCompositeChild& Child = Parent->Children.AddDefaulted_GetRef();
		Child.ChildTask = Task;
	}
}

UBehaviorTree* AD1115_EnemyController::BuildBehaviorTree()
{
	// --- Blackboard: AI 의 메모장 (키 2개)
	RuntimeBlackboard = NewObject<UBlackboardData>(this, TEXT("BB_D1115_Enemy"));
	{
		FBlackboardEntry Target;
		Target.EntryName = KeyTargetActor;
		UBlackboardKeyType_Object* ObjectType = NewObject<UBlackboardKeyType_Object>(RuntimeBlackboard);
		ObjectType->BaseClass = AActor::StaticClass();
		Target.KeyType = ObjectType;
		RuntimeBlackboard->Keys.Add(Target);

		FBlackboardEntry Patrol;
		Patrol.EntryName = KeyPatrolLocation;
		Patrol.KeyType = NewObject<UBlackboardKeyType_Vector>(RuntimeBlackboard);
		RuntimeBlackboard->Keys.Add(Patrol);
		RuntimeBlackboard->UpdateKeyIDs();
	}

	// --- Behavior Tree (그래프 대신 노드 객체를 직접 연결)
	UBehaviorTree* Tree = NewObject<UBehaviorTree>(this, TEXT("BT_D1115_Enemy"));
	Tree->BlackboardAsset = RuntimeBlackboard;

	UBTComposite_Selector* Root = NewObject<UBTComposite_Selector>(Tree);
	Root->NodeName = TEXT("Root Selector");
	Tree->RootNode = Root;

	// 가지 1: 플레이어를 보면 추격
	UBTComposite_Sequence* Chase = NewObject<UBTComposite_Sequence>(Tree);
	Chase->NodeName = TEXT("Chase");
	UBTDecorator_Blackboard* HasTarget = NewObject<UBTDecorator_Blackboard>(Tree);
	HasTarget->NodeName = TEXT("Has TargetActor?");
	PropertyPtr<FBlackboardKeySelector>(HasTarget, TEXT("BlackboardKey"))->SelectedKeyName = KeyTargetActor;
	*PropertyPtr<uint8>(HasTarget, TEXT("OperationType")) = static_cast<uint8>(EBasicKeyOperation::Set);
	// Abort Both: 값이 생기면 낮은 우선순위(순찰)를 끊고, 없어지면 자기(추격)를 끊는다
	*PropertyPtr<TEnumAsByte<EBTFlowAbortMode::Type>>(HasTarget, TEXT("FlowAbortMode")) = EBTFlowAbortMode::Both;
	{
		FBTCompositeChild& Child = Root->Children.AddDefaulted_GetRef();
		Child.ChildComposite = Chase;
		Child.Decorators.Add(HasTarget);
	}
	AddTaskChild(Chase, MakeMoveTo(Tree, KeyTargetActor, 120.f, TEXT("Move To Player")));

	// 가지 2: 순찰
	UBTComposite_Sequence* Patrol = NewObject<UBTComposite_Sequence>(Tree);
	Patrol->NodeName = TEXT("Patrol");
	Root->Children.AddDefaulted_GetRef().ChildComposite = Patrol;
	AddTaskChild(Patrol, NewObject<UD1115_BTTask_NextPatrolPoint>(Tree));
	AddTaskChild(Patrol, MakeMoveTo(Tree, KeyPatrolLocation, 30.f, TEXT("Move To Patrol Point")));
	UBTTask_Wait* Wait = NewObject<UBTTask_Wait>(Tree);
	Wait->NodeName = TEXT("Look Around");
	*PropertyPtr<FValueOrBBKey_Float>(Wait, TEXT("WaitTime")) = FValueOrBBKey_Float(1.5f);
	*PropertyPtr<FValueOrBBKey_Float>(Wait, TEXT("RandomDeviation")) = FValueOrBBKey_Float(0.f);
	AddTaskChild(Patrol, Wait);

	return Tree;
}

// ================================================================ Gameplay Debugger 'Glossary' 카테고리 (확장 예제)

#if WITH_GAMEPLAY_DEBUGGER
namespace D1115_DebuggerCategory
{
	/**
	 * 나만의 Gameplay Debugger 카테고리: 서버에서 CollectData 로 모은 텍스트·도형이 자동 복제되어 화면에 그려진다.
	 * 디버거를 켠 뒤 숫자패드(또는 Shift+숫자)로 카테고리를 켜고 끈다.
	 */
	class FCategory : public FGameplayDebuggerCategory
	{
	public:
		FCategory()
		{
			bShowOnlyWithDebugActor = true; // 대상 액터를 골랐을 때만
		}

		static TSharedRef<FGameplayDebuggerCategory> MakeInstance()
		{
			return MakeShareable(new FCategory());
		}

		virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override
		{
			const AD1115_Enemy* Enemy = Cast<AD1115_Enemy>(DebugActor);
			if (!Enemy)
			{
				AddTextLine(TEXT("{grey}1115 적이 아님"));
				return;
			}
			AddTextLine(FString::Printf(TEXT("{yellow}Glossary 1115 {white}다음 순찰 지점 {green}#%d {white}/ %d"), Enemy->NextPatrolIndex, Enemy->PatrolPoints.Num()));
			const AAIController* Controller = Cast<AAIController>(Enemy->GetController());
			const UBlackboardComponent* BB = Controller ? Controller->GetBlackboardComponent() : nullptr;
			const UObject* Target = BB ? BB->GetValueAsObject(KeyTargetActor) : nullptr;
			AddTextLine(FString::Printf(TEXT("{white}TargetActor: %s"), Target ? TEXT("{red}플레이어 추격 중") : TEXT("{green}없음 (순찰)")));
			for (int32 i = 0; i < Enemy->PatrolPoints.Num(); ++i)
			{
				const FVector A = Enemy->PatrolPoints[i];
				const FVector B = Enemy->PatrolPoints[(i + 1) % Enemy->PatrolPoints.Num()];
				AddShape(FGameplayDebuggerShape::MakeSegment(A, B, 3.f, FColor::Cyan));
				AddShape(FGameplayDebuggerShape::MakePoint(A, 12.f, FColor::Cyan, FString::Printf(TEXT("WP%d"), i)));
			}
		}
	};

	void Register()
	{
		if (IGameplayDebugger::IsAvailable())
		{
			IGameplayDebugger& Debugger = IGameplayDebugger::Get();
			Debugger.RegisterCategory(TEXT("Glossary"), IGameplayDebugger::FOnGetCategory::CreateStatic(&FCategory::MakeInstance),
				EGameplayDebuggerCategoryState::EnabledInGameAndSimulate);
			Debugger.NotifyCategoriesChanged();
		}
	}

	void Unregister()
	{
		if (IGameplayDebugger::IsAvailable())
		{
			IGameplayDebugger& Debugger = IGameplayDebugger::Get();
			Debugger.UnregisterCategory(TEXT("Glossary"));
			Debugger.NotifyCategoriesChanged();
		}
	}
}
#endif

// ================================================================ ADemo1115

ADemo1115_GameplayDebugger::ADemo1115_GameplayDebugger()
{
	DemoId = TEXT("1115");
	Controls = TEXT("' (작은따옴표) : Gameplay Debugger 켜기/끄기 — 화면 가운데에 가장 가까운 AI 가 선택된다\n")
		TEXT("디버거가 켜진 상태에서 숫자패드 0~9 (또는 Shift+숫자) : 카테고리(AI·BehaviorTree·EQS·Perception·Glossary…) 켜고 끄기\n")
		TEXT("관찰: 빨간 적의 BT 활성 노드(Patrol/Chase), 시야 원뿔·감지 결과, 경로. 적 시야에 들어가면 Chase 로 바뀐다");
	Showcase = TEXT("Behavior Tree·Perception 으로 순찰하는 적을 ' 키 Gameplay Debugger 로 들여다보기 (+ 커스텀 카테고리)");
	bNeedsNavMesh = true;
	NavBoundsExtent = FVector(2500.f, 2500.f, 600.f);
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-900.f, 0.f, 100.f));
}

void ADemo1115_GameplayDebugger::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 경로가 꺾여 보이도록 장애물 몇 개 (NavMesh 는 Dynamic 생성이라 실행 중 스폰도 반영된다)
	SpawnBlock(World, FVector(400.f, 0.f, 0.f), FVector(150.f, 500.f, 150.f), Colors::Wall);
	SpawnBlock(World, FVector(900.f, -500.f, 0.f), FVector(300.f, 120.f, 150.f), Colors::Wall);
	SpawnBlock(World, FVector(900.f, 550.f, 0.f), FVector(120.f, 300.f, 150.f), Colors::Wall);

	// SpawnActorDeferred: BeginPlay(→ AI 빙의·BT 시작) 전에 순찰 지점을 넣는다
	Enemy = World->SpawnActorDeferred<AD1115_Enemy>(AD1115_Enemy::StaticClass(), FTransform(FVector(100.f, -600.f, 100.f)), nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	Enemy->PatrolPoints = { FVector(100.f, -600.f, 0.f), FVector(1200.f, -800.f, 0.f), FVector(1300.f, 700.f, 0.f), FVector(100.f, 600.f, 0.f) };
	Enemy->FinishSpawning(FTransform(FVector(100.f, -600.f, 100.f)));

	Panel = UD11_SidePanel::Create(GetPC(), TEXT("Gameplay Debugger"), FVector2D(1.f, 1.f), 13.f);
}

void ADemo1115_GameplayDebugger::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Enemy)
	{
		return;
	}
	const AAIController* Controller = Cast<AAIController>(Enemy->GetController());
	const UBehaviorTreeComponent* BTComp = Controller ? Cast<UBehaviorTreeComponent>(Controller->GetBrainComponent()) : nullptr;
	const UBlackboardComponent* BB = Controller ? Controller->GetBlackboardComponent() : nullptr;
	const FString ActiveTask = BTComp ? BTComp->DescribeActiveTasks() : TEXT("(BT 없음)");
	const bool bChasing = BB && BB->GetValueAsObject(KeyTargetActor) != nullptr;

#if WITH_GAMEPLAY_DEBUGGER
	const TCHAR* DebuggerState = IGameplayDebugger::IsAvailable() ? TEXT("사용 가능 (WITH_GAMEPLAY_DEBUGGER=1)") : TEXT("모듈 미로드");
#else
	const TCHAR* DebuggerState = TEXT("이 빌드에는 없음 (Shipping)");
#endif

	if (Panel)
	{
		Panel->SetBody(FString::Printf(
			TEXT("Gameplay Debugger: %s\n\n")
			TEXT("사용법\n  '  켜기/끄기 (대상 = 화면 가운데에 가까운 AI)\n")
			TEXT("  숫자패드 0~9  카테고리 토글\n  Tab  관전 카메라(자유 시점)로 둘러보기\n")
			TEXT("  콘솔: EnableGDT / gdt.Toggle\n\n")
			TEXT("주요 카테고리\n  AI — 컨트롤러·현재 이동 경로(NavMesh)\n  BehaviorTree — 활성 노드·블랙보드 값\n")
			TEXT("  Perception — 시야 원뿔·감지한 액터\n  Glossary — 이 모듈이 추가한 커스텀 카테고리\n\n")
			TEXT("지금 BT 활성 태스크: %s\nBlackboard TargetActor: %s"),
			DebuggerState, *ActiveTask, bChasing ? TEXT("플레이어 (추격)") : TEXT("없음 (순찰)")));
	}
	SetStatus(FString::Printf(TEXT("적: %s · 활성 태스크 %s"), bChasing ? TEXT("추격") : TEXT("순찰"), *ActiveTask));
}
