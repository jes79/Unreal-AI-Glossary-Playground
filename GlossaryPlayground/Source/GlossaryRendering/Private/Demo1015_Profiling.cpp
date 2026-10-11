#include "Demo1015_Profiling.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "ProfilingDebugging/TraceAuxiliary.h"

using namespace GlossaryKit;

ADemo1015_Profiling::ADemo1015_Profiling()
{
	DemoId = TEXT("1015");
	Controls = TEXT("병목 만들기  G : Game(12ms 계산)   R : Draw(액터 4000개·머티리얼 4000개)   P : GPU(해상도 200%)\n")
		TEXT("1 : stat unit   2 : stat unitgraph   3 : stat gpu   4 : stat fps   T : Insights 트레이스 시작/중지\n")
		TEXT("관찰: 켠 병목에 맞춰 stat unit 의 Game / Draw / GPU 중 하나만 커진다 — 가장 큰 숫자가 병목.\n")
		TEXT("Insights: T 로 .utrace 저장 → UnrealInsights.exe 로 열기 → Timing 창에서 SimulateExpensiveGameLogic 이 가장 길다.");
	Showcase = TEXT("Game·Draw·GPU 병목 스위치 + stat unit/unitgraph/gpu, Trace.File 로 Unreal Insights 트레이스 저장");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Day;
}

void ADemo1015_Profiling::BeginPlay()
{
	Super::BeginPlay();
	SpawnLabel(GetWorld(), TEXT("G · R · P 로 병목을 만들고 stat unit 으로 찾기"), FVector(800.f, 0.f, 400.f), Colors::Item, 20.f);
	SpawnShape(GetWorld(), EShape::Sphere, FVector(800.f, 0.f, 120.f), FVector(200.f), Colors::Player);
	RenderKit::SpawnLookAtCamera(this, FVector(-600.f, 0.f, 600.f), FVector(1200.f, 0.f, 0.f), 80.f);
	Exec(TEXT("stat unit"));
}

void ADemo1015_Profiling::Exec(const TCHAR* Command)
{
	// 콘솔(~)에 직접 입력하는 것과 같다
	if (APlayerController* PC = GetPC())
	{
		PC->ConsoleCommand(Command);
	}
}

void ADemo1015_Profiling::SimulateExpensiveGameLogic()
{
	// 이 스코프 이름이 Insights 의 CPU 트랙에 그대로 표시된다
	TRACE_CPUPROFILER_EVENT_SCOPE(SimulateExpensiveGameLogic);
	const double End = FPlatformTime::Seconds() + GameWorkMs / 1000.0;
	double X = 0.0;
	while (FPlatformTime::Seconds() < End)
	{
		for (int32 i = 0; i < 1000; ++i)
		{
			X += FMath::Sqrt(double(i) + X * 0.0001);
		}
	}
	Sink = X;
}

void ADemo1015_Profiling::SetDrawHeavy(bool bOn)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Demo1015_SetDrawHeavy);
	bDrawHeavy = bOn;
	for (AStaticMeshActor* A : DrawHeavyActors)
	{
		A->Destroy();
	}
	DrawHeavyActors.Reset();
	if (bOn)
	{
		// 액터마다 다른 동적 머티리얼 → 묶어 그리기가 안 돼 렌더(Draw) 스레드가 바빠진다
		for (int32 i = 0; i < DrawHeavyActorCount; ++i)
		{
			const int32 X = i % 80, Y = i / 80;
			AStaticMeshActor* A = SpawnShape(GetWorld(), (i % 2) ? EShape::Cube : EShape::Cylinder, FVector(300.f + X * 50.f, (Y - 25) * 50.f, 20.f), FVector(30.f),
				FLinearColor::MakeFromHSV8(uint8(i * 7), 180, 220));
			DrawHeavyActors.Add(A);
		}
	}
}

void ADemo1015_Profiling::EndPlay(const EEndPlayReason::Type Reason)
{
	if (bTracing)
	{
		Exec(TEXT("Trace.Stop"));
	}
	Exec(TEXT("stat none"));
	CVars.RestoreAll();
	Super::EndPlay(Reason);
}

void ADemo1015_Profiling::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::G)) { bGameHeavy = !bGameHeavy; }
	if (WasKeyPressed(EKeys::R)) { SetDrawHeavy(!bDrawHeavy); }
	if (WasKeyPressed(EKeys::P))
	{
		bGPUHeavy = !bGPUHeavy;
		CVars.Set(TEXT("r.ScreenPercentage"), bGPUHeavy ? GPUHeavyScreenPercentage : 100);
	}
	if (WasKeyPressed(EKeys::One))   { Exec(TEXT("stat unit")); }
	if (WasKeyPressed(EKeys::Two))   { Exec(TEXT("stat unitgraph")); }
	if (WasKeyPressed(EKeys::Three)) { Exec(TEXT("stat gpu")); }
	if (WasKeyPressed(EKeys::Four))  { Exec(TEXT("stat fps")); }
	if (WasKeyPressed(EKeys::T))
	{
		// Unreal Insights: Trace.File 로 Saved/Profiling/*.utrace 에 cpu·gpu·frame 채널을 기록
		bTracing = !bTracing;
		Exec(bTracing ? TEXT("Trace.File default,gpu") : TEXT("Trace.Stop"));
	}

	if (bGameHeavy)
	{
		SimulateExpensiveGameLogic();
	}

	Stats.Update(DeltaSeconds);
	const FString Dest = FTraceAuxiliary::GetTraceDestinationString();
	SetStatus(FString::Printf(TEXT("%s\n병목 스위치: Game %s · Draw %s (액터 %d) · GPU %s\n지금 병목: %s   ·   트레이스: %s"),
		*Stats.ToString(),
		bGameHeavy ? TEXT("ON") : TEXT("off"), bDrawHeavy ? TEXT("ON") : TEXT("off"), DrawHeavyActors.Num(), bGPUHeavy ? TEXT("ON") : TEXT("off"),
		*Stats.Bottleneck(), bTracing ? *FString::Printf(TEXT("기록 중 → %s"), *Dest) : TEXT("꺼짐 (T)")));
}
