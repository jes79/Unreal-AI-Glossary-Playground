#include "Demo1017_WorldPartition.h"

#include "Camera/CameraActor.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo1017_WorldPartition::ADemo1017_WorldPartition()
{
	DemoId = TEXT("1017");
	Controls = TEXT("WASD 이동   M : 지도(위에서) ↔ 캐릭터 카메라   T : 자동 달리기   J : 동쪽으로 96m 순간이동\n")
		TEXT("관찰: 노란 원(Loading Range 128m) 안에 걸친 셀만 바닥 타일·건물이 생기고, 원 밖으로 나간 셀은 사라진다.\n")
		TEXT("캐릭터가 움직이면 셀이 앞쪽에서 로드되고 뒤쪽에서 언로드된다.");
	Showcase = TEXT("768m 월드를 32m 셀 24×24 로 나누고 Loading Range 12800 안의 셀만 로드/언로드, 위에서 본 지도로 표시");
	Environment.Sky = EGlossarySky::Day;
	Environment.FloorSize = 80000.f;
	Environment.bFog = false;
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 100.f));
	Implementation = EGlossaryImplementation::Alternative;
	AlternativeNote = TEXT("World Partition 맵은 액터를 파일 하나씩(OFPA) 저장하고 에디터가 셀을 나눠 쿡/스트리밍한다. ")
		TEXT("이 프로젝트의 레벨 생성기는 일반 맵을 만들어서, 같은 규칙(셀 크기·Loading Range·스트리밍 소스)을 C++ 로 직접 구현해 보여 준다. ")
		TEXT("실제로는 Tools > Convert Level 또는 World Settings > Enable Streaming, Runtime Grid 의 Cell Size·Loading Range 12800 을 설정한다.");
}

void ADemo1017_WorldPartition::BeginPlay()
{
	Super::BeginPlay();
	MapCamera = RenderKit::SpawnViewCamera(this, FVector(0.f, 0.f, 32000.f), FRotator(-89.9f, 0.f, 0.f), 70.f);
	if (APawn* Pawn = GetPlayerPawn())
	{
		UpdateStreaming(Pawn->GetActorLocation());
	}
}

FBox2D ADemo1017_WorldPartition::CellBounds(const FIntPoint& Cell) const
{
	const float Half = GridCells * CellSize * 0.5f;
	const FVector2D Min(Cell.X * CellSize - Half, Cell.Y * CellSize - Half);
	return FBox2D(Min, Min + FVector2D(CellSize, CellSize));
}

void ADemo1017_WorldPartition::LoadCell(const FIntPoint& Cell)
{
	// 셀 내용: 바닥 타일 + 건물 2~4채. 같은 셀은 항상 같은 모양(시드 = 셀 좌표)
	FCell& Data = Loaded.Add(Cell);
	const FBox2D B = CellBounds(Cell);
	const FVector Center(B.GetCenter(), 0.f);
	FRandomStream Rand(Cell.X * 1000 + Cell.Y);
	const FLinearColor Tile = FLinearColor::MakeFromHSV8(uint8(Rand.RandRange(60, 110)), 120, uint8(Rand.RandRange(120, 180)));
	Data.Actors.Add(SpawnShape(GetWorld(), EShape::Cube, Center + FVector(0.f, 0.f, 5.f), FVector(CellSize - 100.f, CellSize - 100.f, 10.f), Tile));
	const int32 Count = Rand.RandRange(2, 4);
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector Pos = Center + FVector(Rand.FRandRange(-1000.f, 1000.f), Rand.FRandRange(-1000.f, 1000.f), 0.f);
		const FVector Size(Rand.FRandRange(400.f, 900.f), Rand.FRandRange(400.f, 900.f), Rand.FRandRange(300.f, 2000.f));
		Data.Actors.Add(SpawnBlock(GetWorld(), Pos, Size, FLinearColor(0.75f, 0.72f, 0.66f)));
	}
	++TotalLoads;
}

void ADemo1017_WorldPartition::UnloadCell(const FIntPoint& Cell)
{
	if (FCell* Data = Loaded.Find(Cell))
	{
		for (const TWeakObjectPtr<AActor>& A : Data->Actors)
		{
			if (A.IsValid())
			{
				A->Destroy();
			}
		}
	}
	Loaded.Remove(Cell);
	++TotalUnloads;
}

void ADemo1017_WorldPartition::UpdateStreaming(const FVector& Source)
{
	// 스트리밍 소스(플레이어)에서 Loading Range 원과 겹치는 셀 = 로드 대상
	const FVector2D S(Source.X, Source.Y);
	TSet<FIntPoint> Wanted;
	for (int32 X = 0; X < GridCells; ++X)
	{
		for (int32 Y = 0; Y < GridCells; ++Y)
		{
			const FBox2D B = CellBounds(FIntPoint(X, Y));
			const FVector2D Closest(FMath::Clamp(S.X, B.Min.X, B.Max.X), FMath::Clamp(S.Y, B.Min.Y, B.Max.Y));
			if (FVector2D::DistSquared(Closest, S) <= FMath::Square(LoadingRange))
			{
				Wanted.Add(FIntPoint(X, Y));
			}
		}
	}
	TArray<FIntPoint> ToUnload;
	for (const TPair<FIntPoint, FCell>& Pair : Loaded)
	{
		if (!Wanted.Contains(Pair.Key))
		{
			ToUnload.Add(Pair.Key);
		}
	}
	for (const FIntPoint& C : ToUnload) { UnloadCell(C); }
	for (const FIntPoint& C : Wanted)
	{
		if (!Loaded.Contains(C)) { LoadCell(C); }
	}
}

void ADemo1017_WorldPartition::DrawGrid(const FVector& Source) const
{
	UWorld* World = GetWorld();
	const float Half = GridCells * CellSize * 0.5f;
	for (int32 i = 0; i <= GridCells; ++i)
	{
		const float P = i * CellSize - Half;
		DrawDebugLine(World, FVector(P, -Half, 30.f), FVector(P, Half, 30.f), FColor(255, 255, 255, 120), false, -1.f, 0, 25.f);
		DrawDebugLine(World, FVector(-Half, P, 30.f), FVector(Half, P, 30.f), FColor(255, 255, 255, 120), false, -1.f, 0, 25.f);
	}
	// Loading Range 원 (노란색)
	DrawDebugCircle(World, FVector(Source.X, Source.Y, 60.f), LoadingRange, 96, FColor::Yellow, false, -1.f, 0, 120.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
	// 스트리밍 소스 표시 (파란 점)
	DrawDebugSolidBox(World, FBox(Source - FVector(500.f, 500.f, 0.f), Source + FVector(500.f, 500.f, 300.f)), FColor(30, 90, 255));
}

void ADemo1017_WorldPartition::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ACharacter* Character = GetPlayerCharacter();
	APlayerController* PC = GetPC();
	if (!Character || !PC)
	{
		return;
	}
	if (WasKeyPressed(EKeys::M))
	{
		bMapView = !bMapView;
		PC->SetViewTarget(bMapView ? static_cast<AActor*>(MapCamera.Get()) : static_cast<AActor*>(Character));
	}
	if (WasKeyPressed(EKeys::T)) { bAutoRun = !bAutoRun; }
	if (WasKeyPressed(EKeys::J)) { Character->TeleportTo(Character->GetActorLocation() + FVector(9600.f, 0.f, 0.f), Character->GetActorRotation()); }
	if (bAutoRun)
	{
		AutoRunAngle += DeltaSeconds * 0.15f;
		Character->AddMovementInput(FVector(FMath::Cos(AutoRunAngle), FMath::Sin(AutoRunAngle), 0.f), 1.f);
	}

	const FVector Source = Character->GetActorLocation();
	UpdateStreaming(Source);
	DrawGrid(Source);
	if (MapCamera)
	{
		MapCamera->SetActorLocation(FVector(Source.X, Source.Y, 32000.f));
	}

	int32 Actors = 0;
	for (const TPair<FIntPoint, FCell>& Pair : Loaded) { Actors += Pair.Value.Actors.Num(); }
	const float Half = GridCells * CellSize * 0.5f;
	const FIntPoint MyCell(FMath::FloorToInt((Source.X + Half) / CellSize), FMath::FloorToInt((Source.Y + Half) / CellSize));
	Stats.Update(DeltaSeconds);
	SetStatus(FString::Printf(TEXT("Cell Size %.0f · Loading Range %.0f · 셀 %d×%d\n로드된 셀 %d / %d · 액터 %d · 누적 로드 %d · 언로드 %d · 내 셀 (%d,%d)\n%s"),
		CellSize, LoadingRange, GridCells, GridCells, Loaded.Num(), GridCells * GridCells, Actors, TotalLoads, TotalUnloads,
		MyCell.X, MyCell.Y, *Stats.ToString(false)));
}
