#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "Animation/AnimationAsset.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "D0710_StateTreeNodes.h"
#include "Demo0703_BehaviorTree.h"
#include "Demo0711_SmartObjects.h"
#include "GlossaryAssetKit.h"
#include "SmartObjectDefinition.h"
#include "StateTree.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeEditingSubsystem.h"
#include "StateTreeEditorData.h"
#include "StateTreeState.h"
#include "UObject/UObjectHash.h"

DEFINE_LOG_CATEGORY_STATIC(LogGlossaryAI, Log, All);

/** 에셋 안의 이전 하위 객체(노드·키·상태)를 치운다 — 빌더를 여러 번 돌려도 쓰레기가 저장되지 않게 */
static void ClearSubobjects(UObject* Asset)
{
	TArray<UObject*> Inner;
	GetObjectsWithOuter(Asset, Inner, EGetObjectsFlags::None);
	for (UObject* Object : Inner)
	{
		Object->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
	}
}

/** 0703: BB_Enemy + BT_Enemy. 실행 노드는 저장되지만 BT 에디터 그래프(BTGraph)는 비어 있다 (런타임은 정상 동작) */
static void BuildBehaviorTreeAssets()
{
	using namespace GlossaryAssetKit;
	UBlackboardData* BB = CreateOrLoad<UBlackboardData>(D0703::BlackboardPath);
	UBehaviorTree* BT = CreateOrLoad<UBehaviorTree>(D0703::TreePath);
	ClearSubobjects(BB);
	ClearSubobjects(BT);
	BT->BTGraph = nullptr;
	UAnimationAsset* Attack = LoadObject<UAnimationAsset>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01"));
	D0703::BuildEnemyTree(BT, BB, Attack, 200.f);
	SaveAsset(BB);
	SaveAsset(BT);
	UE_LOG(LogGlossaryAI, Display, TEXT("[Glossary] 07: BT_Enemy / BB_Enemy 생성 (키 %d개)"), BB->Keys.Num());
}

/** 0710: StateTree 를 에디터 데이터(상태·태스크·전환·조건)로 작성하고 컴파일 → ST_Enemy. StateTree 편집기에서 그대로 열린다 */
static void BuildStateTreeAsset()
{
	using namespace GlossaryAssetKit;
	UStateTree* StateTree = CreateOrLoad<UStateTree>(D0710::StateTreePath);
	ClearSubobjects(StateTree);

	UStateTreeEditorData* EditorData = NewObject<UStateTreeEditorData>(StateTree, NAME_None, RF_Transactional);
	EditorData->Schema = NewObject<UStateTreeAIComponentSchema>(EditorData);
	StateTree->EditorData = EditorData;

	UStateTreeState& Root = EditorData->AddSubTree(TEXT("Root"));
	UStateTreeState& Idle = Root.AddChildState(TEXT("Idle"));
	UStateTreeState& Patrol = Root.AddChildState(TEXT("Patrol"));
	UStateTreeState& Chase = Root.AddChildState(TEXT("Chase"));
	UStateTreeState& Attack = Root.AddChildState(TEXT("Attack"));

	Idle.AddTask<FD0710_Task_Idle>().GetNode().Duration = 2.f;
	Patrol.AddTask<FD0710_Task_Patrol>();
	Chase.AddTask<FD0710_Task_Chase>();
	Attack.AddTask<FD0710_Task_Attack>().GetNode().Interval = 1.f;

	// 보이면 추격 (Idle·Patrol 공통). 전환은 위에 적은 것이 먼저 검사된다
	for (UStateTreeState* State : { &Idle, &Patrol })
	{
		FStateTreeTransition& ToChase = State->AddTransition(EStateTreeTransitionTrigger::OnTick, EStateTreeTransitionType::GotoState, &Chase);
		ToChase.AddConditionWithOuter<FD0710_Cond_CanSeePlayer>(State);
	}
	Idle.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Patrol);
	Patrol.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Idle);

	FStateTreeTransition& ToAttack = Chase.AddTransition(EStateTreeTransitionTrigger::OnTick, EStateTreeTransitionType::GotoState, &Attack);
	ToAttack.AddConditionWithOuter<FD0710_Cond_PlayerInRange>(&Chase).GetNode().Range = 200.f;
	FStateTreeTransition& LostSight = Chase.AddTransition(EStateTreeTransitionTrigger::OnTick, EStateTreeTransitionType::GotoState, &Idle);
	{
		FD0710_Cond_CanSeePlayer& Lost = LostSight.AddConditionWithOuter<FD0710_Cond_CanSeePlayer>(&Chase).GetNode();
		Lost.Memory = 2.f;
		Lost.bInvert = true;
	}
	FStateTreeTransition& BackToChase = Attack.AddTransition(EStateTreeTransitionTrigger::OnTick, EStateTreeTransitionType::GotoState, &Chase);
	{
		FD0710_Cond_PlayerInRange& Out = BackToChase.AddConditionWithOuter<FD0710_Cond_PlayerInRange>(&Attack).GetNode();
		Out.Range = 250.f;
		Out.bInvert = true;
	}

	// 에디터의 Compile 버튼과 같은 경로
	UStateTreeEditingSubsystem::ValidateStateTree(StateTree);
	FStateTreeCompilerLog Log;
	const bool bCompiled = UStateTreeEditingSubsystem::CompileStateTree(StateTree, Log);
	if (!bCompiled)
	{
		Log.DumpToLog(LogGlossaryAI);
	}
	SaveAsset(StateTree);
	UE_LOG(LogGlossaryAI, Display, TEXT("[Glossary] 07: ST_Enemy 컴파일 %s (ReadyToRun=%d)"), bCompiled ? TEXT("성공") : TEXT("실패"), StateTree->IsReadyToRun() ? 1 : 0);
}

/** 0711: 벤치 Smart Object 정의 (슬롯 3개 + Sit 행동 정의) */
static void BuildSmartObjectAsset()
{
	using namespace GlossaryAssetKit;
	USmartObjectDefinition* Definition = CreateOrLoad<USmartObjectDefinition>(D0711::DefinitionPath);
	ClearSubobjects(Definition);
	D0711::FillBenchDefinition(Definition);
	SaveAsset(Definition);
	UE_LOG(LogGlossaryAI, Display, TEXT("[Glossary] 07: SOD_Bench 생성 (슬롯 %d개)"), Definition->GetSlots().Num());
}

/** 07_AI 분야 데모가 쓰는 생성 에셋 (/Game/Glossary/07_AI) */
static void BuildGlossaryAIAssets()
{
	BuildBehaviorTreeAssets();
	BuildStateTreeAsset();
	BuildSmartObjectAsset();
}
#endif

class FGlossaryAIModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::RegisterBuilder(TEXT("07_AI"), &BuildGlossaryAIAssets);
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::UnregisterBuilder(TEXT("07_AI"));
#endif
	}
};

IMPLEMENT_MODULE(FGlossaryAIModule, GlossaryAI);
