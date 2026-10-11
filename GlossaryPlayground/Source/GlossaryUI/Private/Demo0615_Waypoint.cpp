#include "Demo0615_Waypoint.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- 레이어

void UDemo0615_WaypointLayer::BuildTree()
{
	Canvas = MakeRootCanvas();
	for (FDemo0615Marker& M : Markers)
	{
		M.Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		M.Icon = MakeText(TEXT("◆"), 30.f, M.Color, true);
		M.Icon->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		M.Box->AddChildToVerticalBox(M.Icon)->SetHorizontalAlignment(HAlign_Center);
		M.Label = MakeText(TEXT(""), 15.f, FLinearColor::White, true);
		M.Box->AddChildToVerticalBox(M.Label)->SetHorizontalAlignment(HAlign_Center);
		UCanvasPanelSlot* CSlot = Canvas->AddChildToCanvas(M.Box);
		CSlot->SetAutoSize(true);
		CSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	}
}

void UDemo0615_WaypointLayer::AddTarget(const FString& Name, const FVector& Location, const FLinearColor& Color)
{
	FDemo0615Marker& M = Markers.AddDefaulted_GetRef();
	M.Name = Name;
	M.WorldLocation = Location;
	M.Color = Color;
}

void UDemo0615_WaypointLayer::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	APlayerController* PC = GetOwningPlayer();
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}
	int32 SizeX = 0, SizeY = 0;
	PC->GetViewportSize(SizeX, SizeY);
	const float Scale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), 0.01f);
	// 이후 계산은 Slate 단위 (픽셀 / DPI 배율)
	const FVector2D ViewSize(SizeX / Scale, SizeY / Scale);
	const FVector2D Center = ViewSize * 0.5f;
	const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
	const FRotator CamRot = PC->PlayerCameraManager->GetCameraRotation();
	const APawn* Pawn = PC->GetPawn();
	const FVector From = Pawn ? Pawn->GetActorLocation() : CamLoc;

	for (FDemo0615Marker& M : Markers)
	{
		if (!M.Box)
		{
			continue;
		}
		M.DistanceM = FVector::Dist(From, M.WorldLocation) / 100.f;

		// 1) 핵심: 월드 → 화면 픽셀. 카메라 뒤면 false
		FVector2D Pixel;
		const bool bProjected = PC->ProjectWorldLocationToScreen(M.WorldLocation, Pixel, true);
		const FVector2D Pos = Pixel / Scale;
		M.bOnScreen = bProjected && Pos.X >= EdgeMargin && Pos.X <= ViewSize.X - EdgeMargin && Pos.Y >= EdgeMargin && Pos.Y <= ViewSize.Y - EdgeMargin;

		FVector2D Final = Pos;
		float ArrowAngle = 0.f;
		if (!M.bOnScreen)
		{
			// 2) 화면 밖: 화면 중심에서 목표 쪽 방향을 구한다
			FVector2D Dir;
			const FVector Local = CamRot.UnrotateVector(M.WorldLocation - CamLoc); // X 앞, Y 오른쪽, Z 위
			if (bProjected && Local.X > 0.f)
			{
				Dir = Pos - Center;                 // 앞에 있지만 화면 밖: 투영 좌표 방향
			}
			else
			{
				Dir = FVector2D(Local.Y, -Local.Z); // 뒤에 있음: 옆·위아래 성분만 (화면 y 는 아래가 +)
				if (Dir.IsNearlyZero())
				{
					Dir = FVector2D(0.f, 1.f);      // 바로 뒤면 아래쪽에
				}
			}
			Dir.Normalize();
			// 3) 화면 가장자리(여백만큼 안쪽 사각형)에 닿을 때까지 늘린다
			const FVector2D Half = Center - FVector2D(EdgeMargin, EdgeMargin);
			const float T = FMath::Min(FMath::Abs(Dir.X) > KINDA_SMALL_NUMBER ? Half.X / FMath::Abs(Dir.X) : BIG_NUMBER,
				FMath::Abs(Dir.Y) > KINDA_SMALL_NUMBER ? Half.Y / FMath::Abs(Dir.Y) : BIG_NUMBER);
			Final = Center + Dir * T;
			ArrowAngle = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X)) + 90.f; // ▲ 는 위쪽이 0°
		}

		const bool bVisible = M.bOnScreen || bShowOffscreenArrows;
		M.Box->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		M.Icon->SetText(FText::FromString(M.bOnScreen ? TEXT("◆") : TEXT("▲")));
		M.Icon->SetRenderTransformAngle(M.bOnScreen ? 0.f : ArrowAngle);
		M.Label->SetText(FText::FromString(FString::Printf(TEXT("%s %.0fm"), *M.Name, M.DistanceM)));
		if (UCanvasPanelSlot* CSlot = Cast<UCanvasPanelSlot>(M.Box->Slot))
		{
			CSlot->SetPosition(Final);
		}
	}
}

// ---------------------------------------------------------------- 데모

ADemo0615_Waypoint::ADemo0615_Waypoint()
{
	DemoId = TEXT("0615");
	Controls = TEXT("WASD 이동 · 마우스 시점   M : 화면 밖 화살표 ON/OFF\n")
		TEXT("관찰: 보이는 목표는 위에 ◆ 마커, 화면 밖(뒤 포함) 목표는 가장자리에 방향 화살표와 거리(m)가 붙는다");
	Showcase = TEXT("ProjectWorldLocationToScreen 목표 마커, 화면 밖이면 가장자리 방향 화살표 + 거리(m)");
}

void ADemo0615_Waypoint::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	struct FTarget { const TCHAR* Name; FVector Loc; FLinearColor Color; };
	const FTarget Targets[] = {
		{ TEXT("보물"),   FVector(1200.f, 300.f, 50.f),    Colors::Item },
		{ TEXT("동굴"),   FVector(500.f, -2400.f, 150.f),  Colors::Purple },
		{ TEXT("마을"),   FVector(-2600.f, 1200.f, 200.f), Colors::Ally },
		{ TEXT("보스"),   FVector(-1800.f, -1800.f, 150.f), Colors::Enemy },
		{ TEXT("탑"),     FVector(2400.f, 2600.f, 400.f),  Colors::Cyan },
	};
	Layer = CreateWidget<UDemo0615_WaypointLayer>(GetPC(), UDemo0615_WaypointLayer::StaticClass());
	Layer->EdgeMargin = EdgeMargin;
	for (const FTarget& T : Targets)
	{
		SpawnShape(World, EShape::Cylinder, FVector(T.Loc.X, T.Loc.Y, T.Loc.Z), FVector(120.f, 120.f, T.Loc.Z * 2.f), T.Color);
		Layer->AddTarget(T.Name, T.Loc + FVector(0.f, 0.f, T.Loc.Z + 60.f), T.Color);
	}
	Layer->SetVisibility(ESlateVisibility::HitTestInvisible);
	Layer->AddToViewport(0);
}

void ADemo0615_Waypoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Layer)
	{
		Layer->RemoveFromParent();
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0615_Waypoint::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Layer)
	{
		return;
	}
	if (WasKeyPressed(EKeys::M))
	{
		Layer->bShowOffscreenArrows = !Layer->bShowOffscreenArrows;
	}
	FString Line;
	int32 OnScreen = 0;
	for (const FDemo0615Marker& M : Layer->Markers)
	{
		OnScreen += M.bOnScreen ? 1 : 0;
		Line += FString::Printf(TEXT("%s %.0fm%s  "), *M.Name, M.DistanceM, M.bOnScreen ? TEXT("") : TEXT("(밖)"));
	}
	SetStatus(FString::Printf(TEXT("화면 안 %d / %d · 화면 밖 화살표 %s\n%s"), OnScreen, Layer->Markers.Num(),
		Layer->bShowOffscreenArrows ? TEXT("ON") : TEXT("OFF"), *Line));
}
