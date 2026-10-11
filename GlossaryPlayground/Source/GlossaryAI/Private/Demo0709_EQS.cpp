#include "Demo0709_EQS.h"

#include "AIController.h"
#include "Camera/CameraActor.h"
#include "D07Common.h"
#include "DrawDebugHelpers.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryOption.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_SimpleGrid.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Distance.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Trace.h"
#include "GlossaryKit.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

using namespace GlossaryKit;

void UD0709_EnvQueryContext_Player::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	UObject* Owner = QueryInstance.Owner.Get();
	if (AActor* Player = Owner ? UGameplayStatics::GetPlayerPawn(Owner, 0) : nullptr)
	{
		UEnvQueryItemType_Actor::SetContextHelper(ContextData, Player);
	}
}

ADemo0709_EQS::ADemo0709_EQS()
{
	DemoId = TEXT("0709");
	Controls = TEXT("WASD 이동 · R : 쿼리 즉시 실행 · V 카메라 전환\n")
		TEXT("관찰: 적 주변 격자 점 중 회색 = 플레이어에게 보여서 탈락, 색 점 = 숨은 지점(초록일수록 가까움).\n")
		TEXT("적은 가장 좋은 점(큰 초록 구)으로 숨는다. 플레이어가 돌아 들어가면 다른 엄폐 지점을 찾는다");
	Showcase = TEXT("코드로 만든 UEnvQuery(격자 생성 + Trace 가림 필터 + 거리 점수)를 1초마다 실행해 가장 가까운 엄폐 지점으로 이동");
	bNeedsNavMesh = true;
}

void ADemo0709_EQS::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 엄폐물
	SpawnBlock(World, FVector(250.f, 450.f, 0.f), FVector(80.f, 400.f, 260.f), Colors::Wall);
	SpawnBlock(World, FVector(250.f, -450.f, 0.f), FVector(80.f, 400.f, 260.f), Colors::Wall);
	SpawnBlock(World, FVector(1000.f, 0.f, 0.f), FVector(80.f, 500.f, 260.f), Colors::Wall);
	SpawnBlock(World, FVector(700.f, 900.f, 0.f), FVector(300.f, 80.f, 260.f), Colors::Wall);

	// ---- EQS 쿼리 조립 (에디터의 EQS 에셋과 같은 구조: Option = Generator + Tests) ----
	CoverQuery = NewObject<UEnvQuery>(this, TEXT("EQS_FindCover"));
	UEnvQueryOption* Option = NewObject<UEnvQueryOption>(CoverQuery);

	UEnvQueryGenerator_SimpleGrid* Grid = NewObject<UEnvQueryGenerator_SimpleGrid>(Option);
	Grid->GridSize.DefaultValue = GridHalfSize;       // 쿼리 주체(적) 주변 ±900
	Grid->SpaceBetween.DefaultValue = SpaceBetween;   // 150 간격
	Grid->GenerateAround = UEnvQueryContext_Querier::StaticClass();
	Option->Generator = Grid;

	// Trace: 점 → 플레이어 시선이 막혀야(bHit=true) 통과 = "플레이어에게 안 보이는 곳"
	UEnvQueryTest_Trace* Trace = NewObject<UEnvQueryTest_Trace>(Option);
	Trace->Context = UD0709_EnvQueryContext_Player::StaticClass();
	Trace->TestPurpose = EEnvTestPurpose::Filter;
	Trace->FilterType = EEnvTestFilterType::Match;
	Trace->BoolValue.DefaultValue = true;
	Trace->TraceData.TraceChannel = UEngineTypes::ConvertToTraceType(ECC_Visibility);
	Trace->ItemHeightOffset.DefaultValue = 90.f;
	Trace->ContextHeightOffset.DefaultValue = 60.f;

	// Distance: 적 자신까지 거리 — 가까울수록 높은 점수 (InverseLinear)
	UEnvQueryTest_Distance* Distance = NewObject<UEnvQueryTest_Distance>(Option);
	Distance->DistanceTo = UEnvQueryContext_Querier::StaticClass();
	Distance->TestMode = EEnvTestDistance::Distance2D;
	Distance->TestPurpose = EEnvTestPurpose::Score;
	Distance->ScoringEquation = EEnvTestScoreEquation::InverseLinear;

	Option->Tests.Add(Trace);
	Option->Tests.Add(Distance);
	CoverQuery->GetOptionsMutable().Add(Option);

	Enemy = D07::SpawnEnemy(World, FVector(550.f, -150.f, 0.f), 180.f, AAIController::StaticClass(), Colors::Enemy);
	Enemy->SetLabel(TEXT("EQS: 엄폐 지점 찾기"), Colors::Item);

	OverviewCamera = D07::SetupOverviewCamera(this, FVector(-1200.f, 0.f, 2300.f), FRotator(-60.f, 0.f, 0.f));
}

void ADemo0709_EQS::RunQuery()
{
	if (!Enemy || !CoverQuery)
	{
		return;
	}
	QueryCenter = Enemy->GetActorLocation();
	// 핵심: 쿼리 요청 → 비동기로 실행되고 끝나면 델리게이트로 결과(점수순 정렬)를 받는다
	FEnvQueryRequest Request(CoverQuery, Enemy);
	Request.Execute(EEnvQueryRunMode::AllMatching, FQueryFinishedSignature::CreateUObject(this, &ADemo0709_EQS::OnQueryFinished));
}

void ADemo0709_EQS::OnQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	++QueryCount;
	PassedLocations.Reset();
	PassedScores.Reset();
	bHasBest = false;
	if (!Result.IsValid() || !Result->IsSuccessful() || Result->Items.Num() == 0)
	{
		return;
	}
	for (int32 Index = 0; Index < Result->Items.Num(); ++Index)
	{
		PassedLocations.Add(Result->GetItemAsLocation(Index));
		PassedScores.Add(Result->GetItemScore(Index));
	}
	// AllMatching 결과는 점수 내림차순 → 0번이 최고점
	BestLocation = PassedLocations[0];
	bHasBest = true;

	AAIController* AIC = Cast<AAIController>(Enemy->GetController());
	if (AIC && FVector::Dist2D(AIC->GetPathFollowingComponent()->GetCurrentTargetLocation(), BestLocation) > 60.f)
	{
		AIC->MoveToLocation(BestLocation, 30.f);
	}
}

void ADemo0709_EQS::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* World = GetWorld();
	if (WasKeyPressed(EKeys::V))
	{
		D07::ToggleOverviewCamera(this, OverviewCamera);
	}
	QueryTimer -= DeltaSeconds;
	if (QueryTimer <= 0.f || WasKeyPressed(EKeys::R))
	{
		QueryTimer = QueryInterval;
		RunQuery();
	}

	// 생성된 격자(회색 = 탈락) 위에 통과한 점을 점수 색으로
	const int32 Count = FMath::TruncToInt(GridHalfSize * 2.f / SpaceBetween) + 1;
	for (int32 X = 0; X < Count; ++X)
	{
		for (int32 Y = 0; Y < Count; ++Y)
		{
			const FVector P = QueryCenter + FVector(-GridHalfSize + X * SpaceBetween, -GridHalfSize + Y * SpaceBetween, 0.f);
			DrawDebugPoint(World, FVector(P.X, P.Y, 8.f), 6.f, FColor(110, 110, 110), false, -1.f);
		}
	}
	float MinScore = 1.f, MaxScore = 0.f;
	for (float S : PassedScores) { MinScore = FMath::Min(MinScore, S); MaxScore = FMath::Max(MaxScore, S); }
	for (int32 Index = 0; Index < PassedLocations.Num(); ++Index)
	{
		const float T = MaxScore > MinScore ? (PassedScores[Index] - MinScore) / (MaxScore - MinScore) : 1.f;
		const FColor Color = FLinearColor::LerpUsingHSV(FLinearColor(1.f, 0.15f, 0.1f), FLinearColor(0.1f, 1.f, 0.2f), T).ToFColor(true);
		DrawDebugSphere(World, PassedLocations[Index] + FVector(0.f, 0.f, 15.f), 16.f, 8, Color, false, -1.f, 0, 3.f);
	}
	if (bHasBest)
	{
		DrawDebugSphere(World, BestLocation + FVector(0.f, 0.f, 30.f), 45.f, 12, FColor(30, 255, 60), false, -1.f, 0, 5.f);
	}
	if (const APawn* Player = GetPlayerPawn())
	{
		DrawDebugSphere(World, Player->GetActorLocation() + FVector(0.f, 0.f, 60.f), 30.f, 10, FColor(60, 120, 255), false, -1.f, 0, 3.f);
	}
	AAIController* AIC = Enemy ? Cast<AAIController>(Enemy->GetController()) : nullptr;
	D07::DrawCurrentPath(AIC, FColor(255, 230, 40), 5.f);

	SetStatus(FString::Printf(TEXT("쿼리 %d회 실행 · 격자 %d점 중 통과(숨은 곳) %d점\n최고점 위치 %s · 적 이동 %s"),
		QueryCount, Count * Count, PassedLocations.Num(),
		bHasBest ? *FString::Printf(TEXT("(%.0f, %.0f)"), BestLocation.X, BestLocation.Y) : TEXT("없음"),
		AIC && AIC->GetMoveStatus() == EPathFollowingStatus::Moving ? TEXT("중") : TEXT("멈춤 (엄폐 중)")));
}
