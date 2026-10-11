#include "Demo1011_HLOD.h"

#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GlossaryKit.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "RenderingAssetPaths.h"

using namespace GlossaryKit;

namespace
{
	const FVector TownCenter(0.f, 0.f, 0.f);
}

ADemo1011_HLOD::ADemo1011_HLOD()
{
	DemoId = TEXT("1011");
	Controls = TEXT("W/S : 카메라 앞뒤 (Shift 빠르게)   1 : 거리로 자동   2 : 개별 건물 고정   3 : HLOD 프록시 고정   C : 프록시 색 표시\n")
		TEXT("관찰: 80m 보다 멀어지면 건물 25채(컴포넌트 75개)가 합친 메시 1개로 바뀐다. 멀리서는 거의 차이가 안 보이지만\n")
		TEXT("가까이서 프록시를 고정하면 지붕·굴뚝이 없는 단순한 상자 덩어리인 게 보인다.");
	Showcase = TEXT("건물 25채(75 컴포넌트) ↔ 빌더가 합친 HLOD 프록시 메시 1개, 80m 전환 거리·강제 전환·프록시 색 표시");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Day;
	Environment.FloorSize = 40000.f;
	Implementation = EGlossaryImplementation::Alternative;
	AlternativeNote = TEXT("HLOD Layer·HLOD 빌드는 World Partition 맵 + 에디터 'Build HLODs' 가 필요해 코드 레벨 생성기로는 만들지 않았다. ")
		TEXT("대신 빌더가 건물들을 미리 한 메시로 합친 프록시(SM_HLODProxy_Town)를 만들고, 데모가 거리로 개별 건물 ↔ 프록시를 전환한다. ")
		TEXT("실제로는 World Settings 에서 HLOD Layer(Instancing/Merged/Simplified)를 지정하고 Build > Build HLODs.");
}

void ADemo1011_HLOD::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 개별 건물: 몸체 + 지붕 + 굴뚝 (컴포넌트 3개씩)
	for (const RenderingAssets::FTownBuilding& B : RenderingAssets::GetTownBuildings())
	{
		const FVector Base = TownCenter + B.Base;
		Individual.Add(AddShape(this, EShape::Cube, Base + FVector(0.f, 0.f, B.Size.Z * 0.5f), B.Size, B.Color));
		Individual.Add(AddShape(this, EShape::Cone, Base + FVector(0.f, 0.f, B.Size.Z + 80.f), FVector(B.Size.X * 1.1f, B.Size.Y * 1.1f, 160.f), FLinearColor(0.45f, 0.12f, 0.08f)));
		Individual.Add(AddShape(this, EShape::Cube, Base + FVector(B.Size.X * 0.25f, B.Size.Y * 0.2f, B.Size.Z + 120.f), FVector(40.f, 40.f, 160.f), Colors::Dark));
	}

	// HLOD 프록시: 빌더가 25채를 한 메시·한 머티리얼로 합쳐 둔 것
	Proxy = NewObject<UStaticMeshComponent>(this);
	Proxy->SetStaticMesh(LoadAsset<UStaticMesh>(RenderingAssets::TownProxy));
	Proxy->SetupAttachment(RootComponent);
	Proxy->SetRelativeLocation(TownCenter);
	Proxy->RegisterComponent();
	Proxy->SetVisibility(false);
	ProxyMID = SetColor(Proxy, Colors::Purple, 0.7f);

	SpawnLabel(World, TEXT("마을 (건물 25채)"), TownCenter + FVector(0.f, 0.f, 1600.f), Colors::Item, 20.f);
	Camera = RenderKit::SpawnLookAtCamera(this, FVector(-6200.f, -2600.f, 1800.f), FVector(0.f, -900.f, 300.f), 50.f);   // 전환 거리(80m) 바로 안쪽
}

void ADemo1011_HLOD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One))   { ForceMode = 0; }
	if (WasKeyPressed(EKeys::Two))   { ForceMode = 1; }
	if (WasKeyPressed(EKeys::Three)) { ForceMode = 2; }
	if (WasKeyPressed(EKeys::C))     { bTintProxy = !bTintProxy; }
	RenderKit::TickFlyCamera(this, Camera, DeltaSeconds, 1500.f);
	if (!Camera || !Proxy->GetStaticMesh())
	{
		SetStatus(TEXT("SM_HLODProxy_Town 이 없습니다. tools/glossary-build.sh -filter=10 으로 에셋을 만드세요."));
		return;
	}

	const float Distance = FVector::Dist(Camera->GetActorLocation(), TownCenter);
	const bool bWantProxy = ForceMode == 2 || (ForceMode == 0 && Distance > HLODTransitionDistance);
	if (bWantProxy != bShowingProxy)
	{
		bShowingProxy = bWantProxy;
		for (UPrimitiveComponent* C : Individual)
		{
			C->SetVisibility(!bShowingProxy);
		}
		Proxy->SetVisibility(bShowingProxy);
	}
	// 프록시 색 표시: 디버그용으로 프록시를 보라색으로 (실제 HLOD 는 원래 텍스처를 구워 넣는다)
	if (ProxyMID)
	{
		ProxyMID->SetVectorParameterValue(TEXT("Color"), bTintProxy ? Colors::Purple : FLinearColor(0.75f, 0.55f, 0.35f));
	}

	Stats.Update(DeltaSeconds);
	SetStatus(FString::Printf(TEXT("카메라 거리 %.0fm · 전환 거리 %.0fm · 모드 %s\n지금 그리는 것: %s\n%s"),
		Distance / 100.f, HLODTransitionDistance / 100.f,
		ForceMode == 0 ? TEXT("자동") : ForceMode == 1 ? TEXT("개별 고정") : TEXT("프록시 고정"),
		bShowingProxy ? TEXT("HLOD 프록시 1개 (머티리얼 1개)") : *FString::Printf(TEXT("개별 건물 컴포넌트 %d개"), Individual.Num()),
		*Stats.ToString()));
}
