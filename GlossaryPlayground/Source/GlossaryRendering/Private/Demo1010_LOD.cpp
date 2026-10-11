#include "Demo1010_LOD.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "RenderingAssetPaths.h"
#include "SceneManagement.h"
#include "StaticMeshResources.h"

using namespace GlossaryKit;

namespace
{
	// LOD 별 표시 색: LOD0 흰색 → LOD1 초록 → LOD2 노랑 → LOD3 빨강
	const FLinearColor LODColors[] = {FLinearColor(0.85f, 0.85f, 0.85f), FLinearColor(0.1f, 0.8f, 0.2f), FLinearColor(1.f, 0.8f, 0.1f), FLinearColor(0.9f, 0.1f, 0.1f)};
}

ADemo1010_LOD::ADemo1010_LOD()
{
	DemoId = TEXT("1010");
	Controls = TEXT("W/S : 카메라 앞뒤 (Shift 빠르게)   0 : 자동 LOD   1~4 : LOD0~3 강제   F : 와이어프레임\n")
		TEXT("관찰: 멀수록 화면에서 작아져 Screen Size 기준(1.0/0.5/0.25/0.1)을 넘으면 LOD 가 바뀐다.\n")
		TEXT("색 = 현재 LOD (흰 0 · 초록 1 · 노랑 2 · 빨강 3), 와이어프레임에서 삼각형이 줄어드는 게 보인다.");
	Showcase = TEXT("자동 생성 LOD 3단계(50/25/10%) 바위 5개, Screen Size 에 따라 LOD 색·삼각형 수 표시, LOD 강제·와이어프레임");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Day;
}

void ADemo1010_LOD::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	Mesh = LoadAsset<UStaticMesh>(RenderingAssets::LODRock);
	if (!Mesh)
	{
		SetStatus(TEXT("SM_LODRock 이 없습니다. tools/glossary-build.sh -filter=10 으로 에셋을 만드세요."));
		return;
	}

	// 카메라에서 2.5m · 6m · 12m · 25m · 50m 거리에 바위 5개
	const float Distances[] = {250.f, 600.f, 1200.f, 2500.f, 5000.f};
	for (int32 i = 0; i < 5; ++i)
	{
		UStaticMeshComponent* Rock = NewObject<UStaticMeshComponent>(this);
		Rock->SetStaticMesh(Mesh);
		Rock->SetupAttachment(RootComponent);
		const float Y = (i - 2) * (120.f + Distances[i] * 0.12f);
		Rock->SetRelativeLocation(FVector(Distances[i], Y, 90.f));
		Rock->RegisterComponent();
		Rocks.Add(Rock);
		RockMIDs.Add(SetColor(Rock, LODColors[0], 0.85f));
		Labels.Add(SpawnLabel(World, TEXT("LOD"), FVector(Distances[i], Y, 260.f), FLinearColor::White, 16.f));
	}

	Camera = RenderKit::SpawnViewCamera(this, FVector(0.f, 0.f, 160.f), FRotator(-6.f, 0.f, 0.f), 70.f);
}

int32 ADemo1010_LOD::ComputeLOD(const UStaticMeshComponent* Rock, float& OutScreenSize) const
{
	// 엔진의 ComputeStaticMeshLOD 와 같은 방식: 경계 구의 화면 크기 → Screen Size 표에서 LOD 선택
	FMinimalViewInfo View;
	Camera->GetCameraComponent()->GetCameraView(0.f, View);
	if (const APlayerController* PC = GetPC())
	{
		int32 SX = 16, SY = 9;
		PC->GetViewportSize(SX, SY);
		View.AspectRatio = SY > 0 ? float(SX) / SY : 1.777f;
	}
	const FMatrix Proj = View.CalculateProjectionMatrix();
	const FBoxSphereBounds Bounds = Rock->Bounds;
	OutScreenSize = ComputeBoundsScreenSize(FVector4(Bounds.Origin, 1.f), Bounds.SphereRadius, FVector4(View.Location, 1.f), Proj);

	if (ForcedLOD > 0)
	{
		return ForcedLOD - 1;
	}
	const FStaticMeshRenderData* RD = Mesh->GetRenderData();
	for (int32 LOD = RD->LODResources.Num() - 1; LOD > 0; --LOD)
	{
		if (RD->ScreenSize[LOD].GetValue() > OutScreenSize)
		{
			return LOD;
		}
	}
	return 0;
}

void ADemo1010_LOD::EndPlay(const EEndPlayReason::Type Reason)
{
	if (bWireframe && GetPC())
	{
		GetPC()->ConsoleCommand(TEXT("viewmode lit"));
	}
	Super::EndPlay(Reason);
}

void ADemo1010_LOD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Mesh)
	{
		return;
	}
	const FKey NumKeys[] = {EKeys::Zero, EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four};
	for (int32 i = 0; i < 5; ++i)
	{
		if (WasKeyPressed(NumKeys[i]))
		{
			ForcedLOD = i;
			// 핵심: Forced LOD Model (0 = 자동, n = LOD n-1 고정)
			for (UStaticMeshComponent* Rock : Rocks)
			{
				Rock->SetForcedLodModel(ForcedLOD);
			}
		}
	}
	if (WasKeyPressed(EKeys::F) && GetPC())
	{
		bWireframe = !bWireframe;
		GetPC()->ConsoleCommand(bWireframe ? TEXT("viewmode wireframe") : TEXT("viewmode lit"));
	}
	RenderKit::TickFlyCamera(this, Camera, DeltaSeconds, 400.f);

	const FStaticMeshRenderData* RD = Mesh->GetRenderData();
	for (int32 i = 0; i < Rocks.Num(); ++i)
	{
		float ScreenSize = 0.f;
		const int32 LOD = ComputeLOD(Rocks[i], ScreenSize);
		RockMIDs[i]->SetVectorParameterValue(TEXT("Color"), LODColors[FMath::Clamp(LOD, 0, 3)]);
		Labels[i]->SetText(FString::Printf(TEXT("LOD%d · %d tris\nScreen %.2f"), LOD, RD->LODResources[LOD].GetNumTriangles(), ScreenSize));
	}

	FString Table;
	for (int32 LOD = 0; LOD < RD->LODResources.Num(); ++LOD)
	{
		Table += FString::Printf(TEXT("LOD%d %d tris (Screen ≥ %.2f)  "), LOD, RD->LODResources[LOD].GetNumTriangles(), RD->ScreenSize[LOD].GetValue());
	}
	Stats.Update(DeltaSeconds);
	SetStatus(FString::Printf(TEXT("%s\nLOD 모드: %s\n%s"), *Table,
		ForcedLOD == 0 ? TEXT("자동 (Screen Size)") : *FString::Printf(TEXT("LOD%d 강제"), ForcedLOD - 1), *Stats.ToString()));
}
