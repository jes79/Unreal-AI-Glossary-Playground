#include "Demo1114_DebugDraw.h"

#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GlossaryKit.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "VisualLogger/VisualLogger.h"

using namespace GlossaryKit;

// ================================================================ AD1114_Guard

AD1114_Guard::AD1114_Guard()
{
	PrimaryActorTick.bCanEverTick = true;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	RootComponent = Body;
	InitShapeComponent(Body, EShape::Cylinder, FVector(80.f, 80.f, 180.f));
	Body->SetCollisionProfileName(TEXT("NoCollision"));

	Eye = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Eye"));
	Eye->SetupAttachment(Body);
	InitShapeComponent(Eye, EShape::Cube, FVector(40.f, 70.f, 20.f));
	Eye->SetAbsolute(false, false, true);
	Eye->SetRelativeLocation(FVector(45.f, 0.f, 25.f));
	Eye->SetCollisionProfileName(TEXT("NoCollision"));
}

void AD1114_Guard::BeginPlay()
{
	Super::BeginPlay();
	SetColor(Body, Colors::Enemy);
	SetColor(Eye, Colors::Item, 0.4f, 0.f, 3.f);
}

void AD1114_Guard::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* World = GetWorld();
	if (Waypoints.Num() == 0)
	{
		return;
	}

	// --- 순찰 이동 (웨이포인트를 차례로)
	const FVector Goal = Waypoints[TargetIndex];
	FVector ToGoal = Goal - GetActorLocation();
	ToGoal.Z = 0.f;
	if (ToGoal.Size() < 20.f)
	{
		TargetIndex = (TargetIndex + 1) % Waypoints.Num();
	}
	else
	{
		const FVector Step = ToGoal.GetSafeNormal() * PatrolSpeed * DeltaSeconds;
		SetActorLocation(GetActorLocation() + Step);
		// 걷는 방향 + 좌우로 고개를 돌려 원뿔이 흔들리게
		LookAround += DeltaSeconds;
		SetActorRotation(FRotator(0.f, ToGoal.Rotation().Yaw + FMath::Sin(LookAround * 1.5f) * 35.f, 0.f));
	}

	// --- 시야 판정: 거리 + 각도 + 가리는 것 없음(라인 트레이스)
	const FVector EyePos = GetActorLocation() + FVector(0.f, 0.f, 60.f);
	const FVector Forward = GetActorForwardVector();
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	bool bInCone = false, bBlocked = false;
	FVector PlayerPos = FVector::ZeroVector;
	FHitResult Hit;
	if (Player)
	{
		PlayerPos = Player->GetActorLocation();
		const FVector ToPlayer = PlayerPos - EyePos;
		const float CosAngle = FVector::DotProduct(ToPlayer.GetSafeNormal(), Forward);
		bInCone = ToPlayer.Size() <= SightRange && CosAngle >= FMath::Cos(FMath::DegreesToRadians(SightHalfAngle));
		if (bInCone)
		{
			FCollisionQueryParams Params(SCENE_QUERY_STAT(GuardSight), false, this);
			Params.AddIgnoredActor(Player);
			bBlocked = World->LineTraceSingleByChannel(Hit, EyePos, PlayerPos, ECC_Visibility, Params);
		}
	}
	bSeesPlayer = bInCone && !bBlocked;
	const FColor ConeColor = bSeesPlayer ? FColor::Red : (bInCone ? FColor::Orange : FColor::Green);

	// --- DrawDebug: 이 프레임에만 보이는 선(bPersistentLines=false, LifeTime=-1)
	if (bDrawDebug)
	{
		const float HalfRad = FMath::DegreesToRadians(SightHalfAngle);
		DrawDebugCone(World, EyePos, Forward, SightRange, HalfRad, FMath::DegreesToRadians(8.f), 24, ConeColor, false, -1.f, 0, 2.f);
		DrawDebugString(World, FVector(0.f, 0.f, 150.f), bSeesPlayer ? TEXT("ALERT: player seen") : TEXT("PATROL"), this, ConeColor, 0.f, true, 1.4f);
		if (Player && bInCone)
		{
			// 시선: 막히면 막힌 지점까지 주황 + 막힌 점 표시, 보이면 빨강
			DrawDebugLine(World, EyePos, bBlocked ? Hit.ImpactPoint : PlayerPos, bBlocked ? FColor::Orange : FColor::Red, false, -1.f, 0, 3.f);
			if (bBlocked)
			{
				DrawDebugPoint(World, Hit.ImpactPoint, 14.f, FColor::Orange);
			}
		}
	}
	if (bDrawDebug && bDrawPath)
	{
		for (int32 i = 0; i < Waypoints.Num(); ++i)
		{
			const FVector A = Waypoints[i] + FVector(0.f, 0.f, 5.f);
			const FVector B = Waypoints[(i + 1) % Waypoints.Num()] + FVector(0.f, 0.f, 5.f);
			DrawDebugLine(World, A, B, FColor::Cyan, false, -1.f, 0, 4.f);
			DrawDebugSphere(World, A, 25.f, 10, i == TargetIndex ? FColor::Yellow : FColor::Cyan, false, -1.f, 0, 2.f);
		}
		DrawDebugDirectionalArrow(World, GetActorLocation(), Goal, 60.f, FColor::Yellow, false, -1.f, 0, 3.f);
	}

	// --- Visual Logger: 녹화 중일 때만 기록된다(UE_VLOG 매크로가 IsRecording 을 먼저 검사)
	if (FVisualLogger::IsRecording())
	{
		UE_VLOG(this, LogGlossaryPatterns, Log, TEXT("State=%s WP=%d"), bSeesPlayer ? TEXT("Alert") : TEXT("Patrol"), TargetIndex);
		UE_VLOG_CONE(this, LogGlossaryPatterns, Log, EyePos, Forward, SightRange, SightHalfAngle, ConeColor, TEXT("Sight"));
		UE_VLOG_SEGMENT(this, LogGlossaryPatterns, Log, GetActorLocation(), Goal, FColor::Yellow, TEXT("To WP %d"), TargetIndex);
		if (bSeesPlayer)
		{
			UE_VLOG_LOCATION(this, LogGlossaryPatterns, Warning, PlayerPos, 40.f, FColor::Red, TEXT("Player seen"));
		}
		VLogEntries += bSeesPlayer ? 4 : 3;
	}
}

#if ENABLE_VISUAL_LOG
void AD1114_Guard::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	// Visual Logger 창 오른쪽 'Status' 표에 이 값들이 프레임마다 나온다
	FVisualLogStatusCategory Category(TEXT("Guard"));
	Category.Add(TEXT("SeesPlayer"), bSeesPlayer ? TEXT("true") : TEXT("false"));
	Category.Add(TEXT("TargetWaypoint"), FString::FromInt(TargetIndex));
	Category.Add(TEXT("SightRange"), FString::SanitizeFloat(SightRange));
	Snapshot->Status.Add(Category);
	// 순찰 경로도 스냅샷에 넣어 타임라인 어디서든 보이게
	TArray<FVector> Loop = Waypoints;
	if (Loop.Num() > 0)
	{
		const FVector First = Loop[0]; // Loop.Add(Loop[0]) 는 재할당 중 자기 원소 참조 → 어서트
		Loop.Add(First);
	}
	Snapshot->AddPath(Loop, LogGlossaryPatterns.GetCategoryName(), ELogVerbosity::Log, FColor::Cyan, TEXT("Patrol"));
}
#endif

// ================================================================ ADemo1114

ADemo1114_DebugDraw::ADemo1114_DebugDraw()
{
	DemoId = TEXT("1114");
	Controls = TEXT("1 : DrawDebug 전체 켜기/끄기   2 : 순찰 경로 켜기/끄기   V : Visual Logger 녹화 시작/정지\n")
		TEXT("WASD 로 움직여 경비병 시야(원뿔)에 들어가 보기 — 초록=안 보임, 주황=원뿔 안이지만 벽에 가림, 빨강=발견\n")
		TEXT("녹화 파일(.bvlog)은 Saved/Logs → 에디터 Tools > Debug > Visual Logger 에서 Load 후 타임라인을 되감아 본다");
	Showcase = TEXT("경비병 시야를 DrawDebugCone, 순찰 경로를 DrawDebugLine 으로 그리고 Visual Logger 로 녹화");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-700.f, 0.f, 100.f));
}

void ADemo1114_DebugDraw::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Guard = World->SpawnActor<AD1114_Guard>(FVector(200.f, -600.f, 90.f), FRotator::ZeroRotator, Params);
	Guard->Waypoints = { FVector(200.f, -600.f, 90.f), FVector(1000.f, -600.f, 90.f), FVector(1000.f, 600.f, 90.f), FVector(200.f, 600.f, 90.f) };

	// 시선을 가리는 벽 두 개
	SpawnBlock(World, FVector(500.f, -150.f, 0.f), FVector(60.f, 300.f, 220.f), Colors::Wall);
	SpawnBlock(World, FVector(700.f, 350.f, 0.f), FVector(300.f, 60.f, 220.f), Colors::Wall);

	Panel = UD11_SidePanel::Create(GetPC(), TEXT("디버그 그리기 · Visual Logger"), FVector2D(1.f, 0.f), 13.f);
}

void ADemo1114_DebugDraw::ToggleRecording()
{
	FVisualLogger& VLog = FVisualLogger::Get();
	if (!bRecording)
	{
		// 녹화 시작: 메모리 수집 + 파일 기록(.bvlog). 콘솔 'VisLog record' 와 같은 일
		VLog.SetIsRecording(true);
		VLog.SetIsRecordingToFile(true);
		bRecording = true;
		RecordStart = FPlatformTime::Seconds();
		EntriesAtStart = Guard ? Guard->VLogEntries : 0;
		Log.Add(TEXT("Visual Logger 녹화 시작"));
	}
	else
	{
		// 정지하면 파일이 Saved/Logs/VisualLog*.bvlog 로 저장된다
		VLog.SetIsRecordingToFile(false);
		VLog.SetIsRecording(false);
		bRecording = false;
		Log.Add(FString::Printf(TEXT("녹화 정지 → %s 에 .bvlog 저장"), *FPaths::ConvertRelativePathToFull(FPaths::ProjectLogDir())));
	}
}

void ADemo1114_DebugDraw::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bRecording)
	{
		ToggleRecording(); // 레벨을 나갈 때 녹화를 닫아 파일을 남긴다
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo1114_DebugDraw::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Guard)
	{
		return;
	}
	if (WasKeyPressed(EKeys::One)) { Guard->bDrawDebug = !Guard->bDrawDebug; }
	if (WasKeyPressed(EKeys::Two)) { Guard->bDrawPath = !Guard->bDrawPath; }
	if (WasKeyPressed(EKeys::V))   { ToggleRecording(); }

	if (Panel)
	{
		Panel->SetBody(FString::Printf(
			TEXT("DrawDebug %s · 경로 %s\n경비병: %s\n\n")
			TEXT("Visual Logger: %s\n  이번 녹화 기록 %d개%s\n\n")
			TEXT("그리는 코드 (매 프레임, Tick 안)\n")
			TEXT("  DrawDebugCone(World, Eye, Fwd, %.0f, %.0f°…)\n  DrawDebugLine(World, WP[i], WP[i+1], Cyan…)\n")
			TEXT("기록 코드 (녹화 중에만 비용)\n  UE_VLOG_CONE / UE_VLOG_SEGMENT / UE_VLOG\n\n")
			TEXT("DrawDebug 는 개발 빌드 전용 (Shipping 에선 빈 함수)\n\n%s"),
			Guard->bDrawDebug ? TEXT("ON") : TEXT("OFF"), Guard->bDrawPath ? TEXT("ON") : TEXT("OFF"),
			Guard->bSeesPlayer ? TEXT("플레이어 발견!") : TEXT("순찰 중"),
			bRecording ? TEXT("녹화 중 (REC)") : TEXT("정지"), Guard->VLogEntries - EntriesAtStart,
			bRecording ? *FString::Printf(TEXT(" · %.0f초"), FPlatformTime::Seconds() - RecordStart) : TEXT(""),
			Guard->SightRange, Guard->SightHalfAngle * 2.f, *Log.Join()));
	}
	SetStatus(FString::Printf(TEXT("시야 %.0fcm · %.0f° · %s · VisLog %s"), Guard->SightRange, Guard->SightHalfAngle * 2.f,
		Guard->bSeesPlayer ? TEXT("발견") : TEXT("순찰"), bRecording ? TEXT("녹화 중") : TEXT("정지")));
}
