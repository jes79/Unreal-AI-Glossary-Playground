#include "Demo1002_LumenReflections.h"

#include "Camera/CameraActor.h"
#include "Components/PostProcessComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo1002_LumenReflections::ADemo1002_LumenReflections()
{
	DemoId = TEXT("1002");
	Controls = TEXT("1 : Lumen Reflections   2 : Screen Space Reflections   3 : 끄기(None)\n")
		TEXT("WASD·QE 카메라 이동 · ←→↑↓ 회전\n")
		TEXT("관찰: 화면 위쪽 밖의 노란 간판과 잘린 기둥 꼭대기는 Lumen 에서만 바닥에 비친다.\n")
		TEXT("SSR 은 화면에 보이는 것만 반사해 화면 가장자리로 갈수록 반사가 사라진다.");
	Showcase = TEXT("Roughness 0.05 대리석 바닥, 화면 밖 노란 간판·기둥 반사로 Lumen 반사 vs SSR vs None 비교");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Day;
}

void ADemo1002_LumenReflections::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 대리석 바닥: 어두운 회색 + Roughness 0.05 (거울에 가까움)
	AStaticMeshActor* Floor = SpawnShape(World, EShape::Cube, FVector(800.f, 0.f, -4.f), FVector(4000.f, 3000.f, 10.f), FLinearColor(0.08f, 0.08f, 0.09f));
	SetColor(Floor->GetStaticMeshComponent(), FLinearColor(0.06f, 0.06f, 0.07f), FloorRoughness, 0.f);

	// 화면 안: 색 기둥과 구
	const FLinearColor Cols[] = {Colors::Enemy, Colors::Ally, Colors::Player, Colors::Accent, Colors::Purple};
	for (int32 i = 0; i < 5; ++i)
	{
		const float Y = (i - 2) * 350.f;
		SpawnShape(World, EShape::Cylinder, FVector(900.f, Y, 150.f), FVector(80.f, 80.f, 300.f), Cols[i]);
		SpawnShape(World, EShape::Sphere, FVector(500.f, Y + 150.f, 60.f), FVector(120.f), Cols[(i + 2) % 5], FRotator::ZeroRotator);
	}
	// 화면 위로 잘리는 높은 기둥 (꼭대기는 화면 밖 → SSR 반사에서 잘림)
	AStaticMeshActor* Tall = SpawnShape(World, EShape::Cube, FVector(1400.f, 0.f, 1500.f), FVector(200.f, 200.f, 3000.f), Colors::Cyan);
	SetColor(Tall->GetStaticMeshComponent(), Colors::Cyan, 0.5f, 0.f, 2.f);

	// 화면 위쪽 밖에 떠 있는 큰 노란 간판 (화면에 안 보이므로 SSR 은 바닥에 비추지 못한다)
	AStaticMeshActor* Back = SpawnShape(World, EShape::Cube, FVector(1600.f, 0.f, 900.f), FVector(40.f, 2400.f, 400.f), Colors::Item);
	SetColor(Back->GetStaticMeshComponent(), Colors::Item, 0.6f, 0.f, 3.f);

	SpawnLabel(World, TEXT("화면 위(밖)에 노란 간판이 떠 있다"), FVector(200.f, -700.f, 40.f), Colors::Item, 18.f);

	Camera = RenderKit::SpawnLookAtCamera(this, FVector(-500.f, 0.f, 220.f), FVector(600.f, 0.f, -60.f), 75.f);

	PostProcess = RenderKit::AddPostProcess(this);
	ApplyMethod();
}

void ADemo1002_LumenReflections::ApplyMethod()
{
	FPostProcessSettings& PP = PostProcess->Settings;
	PP.bOverride_ReflectionMethod = true;
	PP.ReflectionMethod = ReflectionMethod;
	// SSR 품질을 최대로 해서 '화면 공간의 한계'만 남게 한다
	PP.bOverride_ScreenSpaceReflectionQuality = true;
	PP.ScreenSpaceReflectionQuality = 100.f;
	PP.bOverride_ScreenSpaceReflectionIntensity = true;
	PP.ScreenSpaceReflectionIntensity = 100.f;
}

void ADemo1002_LumenReflections::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One))   { ReflectionMethod = EReflectionMethod::Lumen; ApplyMethod(); }
	if (WasKeyPressed(EKeys::Two))   { ReflectionMethod = EReflectionMethod::ScreenSpace; ApplyMethod(); }
	if (WasKeyPressed(EKeys::Three)) { ReflectionMethod = EReflectionMethod::None; ApplyMethod(); }
	RenderKit::TickFlyCamera(this, Camera, DeltaSeconds, 600.f);

	Stats.Update(DeltaSeconds);
	const TCHAR* Name = ReflectionMethod == EReflectionMethod::Lumen ? TEXT("Lumen Reflections")
		: ReflectionMethod == EReflectionMethod::ScreenSpace ? TEXT("Screen Space Reflections (화면 안만)") : TEXT("None (스카이라이트만)");
	SetStatus(FString::Printf(TEXT("Reflection Method: %s · 바닥 Roughness %.2f\n%s"), Name, FloorRoughness, *Stats.ToString(false)));
}
