#include "Demo1013_CullDistance.h"

#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CullDistanceVolume.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo1013_CullDistance::ADemo1013_CullDistance()
{
	DemoId = TEXT("1013");
	Controls = TEXT("1 : 컬링 ON/OFF (Cull Distance Volume + 가로등 Max Draw Distance)   W/S·A/D·Q/E·←→↑↓ 카메라\n")
		TEXT("관찰: 작은 소품(색 상자·구)은 카메라에서 30m 밖이면 사라지고, 노란 가로등은 15m 밖에서 사라진다.\n")
		TEXT("큰 건물은 그대로. 카메라를 앞으로 움직이면 소품이 30m 앞에서 '생겨난다'. 상태 줄의 그려진 개수를 비교.");
	Showcase = TEXT("Cull Distance Volume(50cm 이하 → 3000 밖 컬링) + 가로등 Max Draw Distance 1500, 그려진 소품 수 비교");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Day;
	Environment.FloorSize = 20000.f;
}

#if WITH_EDITOR
void ADemo1013_CullDistance::BuildLevel(UWorld* World)
{
	// 레벨 전체를 덮는 Cull Distance Volume. CullDistances 표: (크기, 컬 거리). 크기가 가장 가까운 항목이 쓰인다
	ACullDistanceVolume* V = Cast<ACullDistanceVolume>(RenderKit::SpawnBoxVolume(World, ACullDistanceVolume::StaticClass(), FVector(4000.f, 0.f, 500.f), FVector(20000.f, 20000.f, 3000.f)));
	if (V)
	{
		V->CullDistances.Reset();
		V->CullDistances.Add(FCullDistanceSizePair(SmallPropSize, SmallPropCullDistance));   // 작은 소품: 3000 밖 컬링
		V->CullDistances.Add(FCullDistanceSizePair(1000.f, 0.f));                           // 큰 물체(10m~): 0 = 컬링 안 함
		V->SetActorLabel(TEXT("CullDistanceVolume"));
	}
}
#endif

void ADemo1013_CullDistance::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	for (TActorIterator<ACullDistanceVolume> It(World); It; ++It)
	{
		Volume = *It;
	}

	// 작은 소품 600개: 길이 120m 의 길 양쪽에 흩뿌린다 (크기 30~50cm)
	FRandomStream Rand(1013);
	const FLinearColor Cols[] = {Colors::Enemy, Colors::Ally, Colors::Player, Colors::Accent, Colors::Purple, Colors::Cyan};
	for (int32 i = 0; i < 600; ++i)
	{
		const FVector Loc(Rand.FRandRange(200.f, 12000.f), Rand.FRandRange(-1200.f, 1200.f), 0.f);
		const float S = Rand.FRandRange(30.f, 50.f);
		AStaticMeshActor* A = SpawnShape(World, Rand.RandRange(0, 1) ? EShape::Cube : EShape::Sphere, Loc + FVector(0.f, 0.f, S * 0.5f), FVector(S), Cols[i % 6],
			FRotator::ZeroRotator, false, /*bMovable=*/false);   // Cull Distance Volume 은 Static 만 적용된다
		Props.Add(A->GetStaticMeshComponent());
	}
	// 큰 건물: 길 양쪽 (컬링되지 않음)
	for (int32 i = 0; i < 8; ++i)
	{
		SpawnBlock(World, FVector(1000.f + i * 1500.f, (i % 2 ? 1.f : -1.f) * 1800.f, 0.f), FVector(800.f, 600.f, 900.f + (i % 3) * 300.f), FLinearColor(0.7f, 0.68f, 0.62f));
	}
	// 노란 가로등: 개별 Max Draw Distance
	for (int32 i = 0; i < 12; ++i)
	{
		AStaticMeshActor* Pole = SpawnShape(World, EShape::Cylinder, FVector(500.f + i * 1000.f, -400.f, 200.f), FVector(20.f, 20.f, 400.f), Colors::Item, FRotator::ZeroRotator, false, false);
		Lamps.Add(Pole->GetStaticMeshComponent());
	}

	Camera = RenderKit::SpawnViewCamera(this, FVector(-400.f, 0.f, 350.f), FRotator(-8.f, 0.f, 0.f), 80.f);
	SpawnLabel(World, TEXT("30m"), FVector(2600.f, 0.f, 120.f), Colors::Item, 18.f);
	SetCullingEnabled(true);
}

void ADemo1013_CullDistance::SetCullingEnabled(bool bEnabled)
{
	bCulling = bEnabled;
	if (Volume)
	{
		// 핵심 1: Cull Distance Volume 켜고 끄기 → 월드에 다시 적용
		Volume->bEnabled = bEnabled;
		GetWorld()->UpdateCullDistanceVolumes();
	}
	// 핵심 2: 개별 컴포넌트의 Max Draw Distance (Details > Rendering > Max Draw Distance)
	for (UStaticMeshComponent* Lamp : Lamps)
	{
		Lamp->SetCullDistance(bEnabled ? LampMaxDrawDistance : 0.f);
	}
}

void ADemo1013_CullDistance::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One)) { SetCullingEnabled(!bCulling); }
	RenderKit::TickFlyCamera(this, Camera, DeltaSeconds, 800.f);

	// 최근 0.2초 안에 화면에 그려진 소품 수 (컬링되면 안 그려진다)
	int32 DrawnProps = 0, DrawnLamps = 0;
	for (UStaticMeshComponent* P : Props) { DrawnProps += P->WasRecentlyRendered(0.2f) ? 1 : 0; }
	for (UStaticMeshComponent* L : Lamps) { DrawnLamps += L->WasRecentlyRendered(0.2f) ? 1 : 0; }
	const float PropCull = Props.Num() ? Props[0]->CachedMaxDrawDistance : 0.f;

	Stats.Update(DeltaSeconds);
	SetStatus(FString::Printf(TEXT("컬링 %s · Volume %s · 소품 Cull Distance %.0f · 가로등 Max Draw Distance %.0f\n그려진 소품 %d / %d · 가로등 %d / %d\n%s"),
		bCulling ? TEXT("ON") : TEXT("OFF"), Volume ? TEXT("있음") : TEXT("없음(레벨 다시 빌드)"), PropCull,
		Lamps.Num() ? Lamps[0]->LDMaxDrawDistance : 0.f, DrawnProps, Props.Num(), DrawnLamps, Lamps.Num(), *Stats.ToString()));
}
