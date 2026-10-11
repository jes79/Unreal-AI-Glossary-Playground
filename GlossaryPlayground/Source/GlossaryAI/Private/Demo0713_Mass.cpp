#include "Demo0713_Mass.h"

#include "Camera/CameraActor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "D07Common.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryKit.h"
#include "HAL/PlatformTime.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "MassExecutor.h"
#include "MassProcessingContext.h"

using namespace GlossaryKit;

namespace
{
	/** 사각 순환로 둘레 위 거리 S → 위치·진행 방향 */
	void RectanglePoint(const FVector2D& Half, float S, float Direction, FVector& OutLocation, FVector& OutForward)
	{
		const float Perimeter = 4.f * (Half.X + Half.Y);
		S = FMath::Fmod(FMath::Fmod(S, Perimeter) + Perimeter, Perimeter);
		if (Direction < 0.f)
		{
			S = Perimeter - S;   // 반대 방향 차선
		}
		const float Edges[4] = { 2.f * Half.X, 2.f * Half.Y, 2.f * Half.X, 2.f * Half.Y };
		const FVector Starts[4] = { FVector(-Half.X, -Half.Y, 0.f), FVector(Half.X, -Half.Y, 0.f), FVector(Half.X, Half.Y, 0.f), FVector(-Half.X, Half.Y, 0.f) };
		const FVector Dirs[4] = { FVector(1, 0, 0), FVector(0, 1, 0), FVector(-1, 0, 0), FVector(0, -1, 0) };
		for (int32 Edge = 0; Edge < 4; ++Edge)
		{
			if (S <= Edges[Edge] || Edge == 3)
			{
				OutLocation = Starts[Edge] + Dirs[Edge] * FMath::Min(S, Edges[Edge]);
				OutForward = Dirs[Edge] * Direction;
				return;
			}
			S -= Edges[Edge];
		}
	}
}

// ============================================================ Processors

UD0713_LaneFollowProcessor::UD0713_LaneFollowProcessor()
	: EntityQuery(*this)
{
	// 전역 처리 단계에 자동 등록하지 않고 데모가 직접 실행한다 (다른 레벨에 영향 없음)
	bAutoRegisterWithProcessingPhases = false;
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::All);
}

void UD0713_LaneFollowProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	// 이 프로세서가 읽고 쓰는 Fragment 를 선언 → 이 조합을 가진 모든 아키타입의 청크가 대상
	EntityQuery.AddRequirement<FD0713_LaneFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FD0713_TransformFragment>(EMassFragmentAccess::ReadWrite);
}

void UD0713_LaneFollowProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const float DeltaTime = Context.GetDeltaTimeSeconds();
	LastChunkCount = 0;
	EntityQuery.ForEachEntityChunk(Context, [this, DeltaTime](FMassExecutionContext& ChunkContext)
	{
		++LastChunkCount;
		// 청크 = 같은 아키타입 엔티티들의 Fragment 가 연속 메모리에 놓인 블록. 배열처럼 돈다
		const TArrayView<FD0713_LaneFragment> Lanes = ChunkContext.GetMutableFragmentView<FD0713_LaneFragment>();
		const TArrayView<FD0713_TransformFragment> Transforms = ChunkContext.GetMutableFragmentView<FD0713_TransformFragment>();
		for (int32 Index = 0; Index < ChunkContext.GetNumEntities(); ++Index)
		{
			FD0713_LaneFragment& Lane = Lanes[Index];
			Lane.Distance += Lane.Speed * DeltaTime;
			Lane.Phase += DeltaTime;
			FVector Location, Forward;
			RectanglePoint(LaneHalfExtents[Lane.Lane], Lane.Distance, LaneDirections[Lane.Lane], Location, Forward);
			const FVector Right(-Forward.Y, Forward.X, 0.f);
			const float Sway = FMath::Sin(Lane.Phase * 2.2f) * 8.f;
			Transforms[Index].Location = Location + Right * (Lane.Lateral + Sway);
			Transforms[Index].Yaw = Forward.Rotation().Yaw;
		}
	});
}

UD0713_RepresentationProcessor::UD0713_RepresentationProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = false;
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::All);
}

void UD0713_RepresentationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FD0713_LaneFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FD0713_TransformFragment>(EMassFragmentAccess::ReadOnly);
}

void UD0713_RepresentationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	TransformsByLane[0].Reset();
	TransformsByLane[1].Reset();
	EntityQuery.ForEachEntityChunk(Context, [this](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FD0713_LaneFragment> Lanes = ChunkContext.GetFragmentView<FD0713_LaneFragment>();
		const TConstArrayView<FD0713_TransformFragment> Transforms = ChunkContext.GetFragmentView<FD0713_TransformFragment>();
		for (int32 Index = 0; Index < ChunkContext.GetNumEntities(); ++Index)
		{
			TransformsByLane[Lanes[Index].Lane < 2 ? 0 : 1].Add(FTransform(FRotator(0.f, Transforms[Index].Yaw, 0.f), Transforms[Index].Location));
		}
	});
}

// ============================================================ 데모

ADemo0713_Mass::ADemo0713_Mass()
{
	DemoId = TEXT("0713");
	Controls = TEXT("C : 시뮬레이션 일시정지 · V 카메라 전환 (WASD 이동)\n")
		TEXT("관찰: 보행자 1000명(주황=바깥 시계 반대, 하늘=안쪽 시계 방향)이 4개 차선을 걷는다.\n")
		TEXT("각자는 액터가 아니라 Mass 엔티티(Fragment 데이터) — Processor 가 청크 단위로 한 번에 움직인다");
	Showcase = TEXT("Mass Entity 코어(Fragment·아키타입·Processor)로 보행자 1000명을 차선 따라 걷게 하고 ISM 으로 표시");
	Implementation = EGlossaryImplementation::Alternative;
	AlternativeNote = TEXT("Mass Entity 코어 API(Fragment·아키타입·BatchCreateEntities·Processor)는 실제로 쓰고, ")
		TEXT("Mass Spawner·EntityConfig 에셋(Trait)·ZoneGraph 차선·MassCrowd 회피·Mass Representation 대신 ")
		TEXT("사각 순환 차선 계산과 Instanced Static Mesh 갱신을 직접 구현");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(0.f, -2700.f, 100.f));
}

void ADemo0713_Mass::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 가운데 건물과 보도
	SpawnBlock(World, FVector(0.f, 0.f, 0.f), FVector(2200.f, 900.f, 600.f), Colors::Platform);
	for (const FVector2D& Ring : { FVector2D(1525.f, 725.f), FVector2D(2175.f, 1375.f) })
	{
		const float W = 260.f;
		SpawnShape(World, EShape::Cube, FVector(0.f, -Ring.Y, 1.f), FVector(Ring.X * 2.f + W, W, 2.f), FLinearColor(0.5f, 0.5f, 0.52f))->SetActorEnableCollision(false);
		SpawnShape(World, EShape::Cube, FVector(0.f, Ring.Y, 1.f), FVector(Ring.X * 2.f + W, W, 2.f), FLinearColor(0.5f, 0.5f, 0.52f))->SetActorEnableCollision(false);
		SpawnShape(World, EShape::Cube, FVector(-Ring.X, 0.f, 1.f), FVector(W, Ring.Y * 2.f + W, 2.f), FLinearColor(0.5f, 0.5f, 0.52f))->SetActorEnableCollision(false);
		SpawnShape(World, EShape::Cube, FVector(Ring.X, 0.f, 1.f), FVector(W, Ring.Y * 2.f + W, 2.f), FLinearColor(0.5f, 0.5f, 0.52f))->SetActorEnableCollision(false);
	}

	// 보행자 표현: 방향 그룹별 몸통(원기둥)·머리(구) Instanced Static Mesh
	const FLinearColor GroupColors[2] = { Colors::Accent, Colors::Cyan };
	for (int32 Group = 0; Group < 2; ++Group)
	{
		Bodies[Group] = NewObject<UInstancedStaticMeshComponent>(this);
		Bodies[Group]->SetStaticMesh(GetShapeMesh(EShape::Cylinder));
		Heads[Group] = NewObject<UInstancedStaticMeshComponent>(this);
		Heads[Group]->SetStaticMesh(GetShapeMesh(EShape::Sphere));
		for (UInstancedStaticMeshComponent* ISM : { Bodies[Group].Get(), Heads[Group].Get() })
		{
			ISM->SetMobility(EComponentMobility::Movable);
			ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ISM->SetCanEverAffectNavigation(false);
			ISM->SetupAttachment(Root);
			ISM->RegisterComponent();
			AddInstanceComponent(ISM);
			D07::SetAgentColor(ISM, 0, GroupColors[Group]);
		}
	}

	// ---- Mass: 아키타입(= Fragment 조합) 만들고 엔티티 일괄 생성 ----
	UMassEntitySubsystem* MassSubsystem = World->GetSubsystem<UMassEntitySubsystem>();
	if (!MassSubsystem)
	{
		SetStatus(TEXT("MassEntitySubsystem 없음"));
		return;
	}
	FMassEntityManager& EntityManager = MassSubsystem->GetMutableEntityManager();
	const FMassArchetypeHandle Archetype = EntityManager.CreateArchetype({ FD0713_LaneFragment::StaticStruct(), FD0713_TransformFragment::StaticStruct() });
	EntityManager.BatchCreateEntities(Archetype, NumPedestrians, Entities);

	const TArray<FVector2D> LaneHalf = { FVector2D(2120.f, 1320.f), FVector2D(2230.f, 1430.f), FVector2D(1470.f, 670.f), FVector2D(1580.f, 780.f) };
	const TArray<float> LaneDir = { 1.f, 1.f, -1.f, -1.f };
	FRandomStream Random(713);
	for (int32 Index = 0; Index < Entities.Num(); ++Index)
	{
		// 초기값 채우기 (실제 Mass 에서는 Trait 의 InitializeTemplate·Initializer 프로세서가 하는 일)
		FD0713_LaneFragment& Lane = EntityManager.GetFragmentDataChecked<FD0713_LaneFragment>(Entities[Index]);
		Lane.Lane = Index % 4;
		const float Perimeter = 4.f * (LaneHalf[Lane.Lane].X + LaneHalf[Lane.Lane].Y);
		Lane.Distance = Random.FRandRange(0.f, Perimeter);
		Lane.Speed = Random.FRandRange(110.f, 190.f);
		Lane.Lateral = Random.FRandRange(-45.f, 45.f);
		Lane.Phase = Random.FRandRange(0.f, 6.f);
	}

	// ---- Processor: 생성하고 초기화(쿼리 구성)한 뒤 매 프레임 실행 ----
	LaneProcessor = NewObject<UD0713_LaneFollowProcessor>(this);
	LaneProcessor->LaneHalfExtents = LaneHalf;
	LaneProcessor->LaneDirections = LaneDir;
	LaneProcessor->CallInitialize(this, EntityManager.AsShared());
	RepresentationProcessor = NewObject<UD0713_RepresentationProcessor>(this);
	RepresentationProcessor->CallInitialize(this, EntityManager.AsShared());

	OverviewCamera = D07::SetupOverviewCamera(this, FVector(0.f, -4300.f, 3600.f), FRotator(-48.f, 90.f, 0.f), 75.f);
}

void ADemo0713_Mass::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMassEntitySubsystem* MassSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UMassEntitySubsystem>() : nullptr)
	{
		if (Entities.Num() > 0)
		{
			MassSubsystem->GetMutableEntityManager().BatchDestroyEntities(Entities);
		}
	}
	Entities.Reset();
	Super::EndPlay(EndPlayReason);
}

void ADemo0713_Mass::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::V))
	{
		D07::ToggleOverviewCamera(this, OverviewCamera);
	}
	if (WasKeyPressed(EKeys::C))
	{
		bPaused = !bPaused;
	}
	UMassEntitySubsystem* MassSubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!MassSubsystem || !LaneProcessor)
	{
		return;
	}
	FMassEntityManager& EntityManager = MassSubsystem->GetMutableEntityManager();

	const double Start = FPlatformTime::Seconds();
	{
		// Executor 로 프로세서 실행 (MassSimulationSubsystem 의 처리 단계가 매 프레임 하는 일)
		FMassProcessingContext Context(EntityManager, bPaused ? 0.f : DeltaSeconds);
		UE::Mass::Executor::Run(*LaneProcessor, Context);
	}
	{
		FMassProcessingContext Context(EntityManager, DeltaSeconds);
		UE::Mass::Executor::Run(*RepresentationProcessor, Context);
	}
	LastProcessMs = (FPlatformTime::Seconds() - Start) * 1000.0;

	// 표현: 엔티티 Transform → ISM 인스턴스
	for (int32 Group = 0; Group < 2; ++Group)
	{
		const TArray<FTransform>& Source = RepresentationProcessor->TransformsByLane[Group];
		TArray<FTransform> BodyXf, HeadXf;
		BodyXf.Reserve(Source.Num());
		HeadXf.Reserve(Source.Num());
		for (const FTransform& T : Source)
		{
			BodyXf.Add(FTransform(T.GetRotation(), T.GetLocation() + FVector(0.f, 0.f, 75.f), FVector(0.38f, 0.38f, 1.5f)));
			HeadXf.Add(FTransform(T.GetRotation(), T.GetLocation() + FVector(0.f, 0.f, 168.f), FVector(0.3f)));
		}
		for (TPair<UInstancedStaticMeshComponent*, TArray<FTransform>*> Pair : { TPair<UInstancedStaticMeshComponent*, TArray<FTransform>*>(Bodies[Group], &BodyXf), TPair<UInstancedStaticMeshComponent*, TArray<FTransform>*>(Heads[Group], &HeadXf) })
		{
			if (Pair.Key->GetInstanceCount() != Pair.Value->Num())
			{
				Pair.Key->ClearInstances();
				Pair.Key->AddInstances(*Pair.Value, false, true);
			}
			else
			{
				Pair.Key->BatchUpdateInstancesTransforms(0, *Pair.Value, true, true, true);
			}
		}
	}

	SetStatus(FString::Printf(TEXT("엔티티 %d개 · 아키타입 [LaneFragment + TransformFragment] · 청크 %d개\n프로세서 2개 실행 %.2fms/프레임 · %s"),
		Entities.Num(), LaneProcessor->LastChunkCount, LastProcessMs, bPaused ? TEXT("일시정지") : TEXT("걷는 중")));
}
