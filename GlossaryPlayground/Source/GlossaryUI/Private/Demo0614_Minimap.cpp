#include "Demo0614_Minimap.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "GlossaryUIKit.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- 위젯

void UDemo0614_MinimapWidget::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	// 요청: 오른쪽 위
	AddToCanvas(Canvas, Col, FAnchors(1.f, 0.f), FVector2D(-24.f, 24.f), FVector2D::ZeroVector, FVector2D(1.f, 0.f));

	UBorder* Frame = MakeBorder(FLinearColor(0.9f, 0.85f, 0.6f, 1.f), FMargin(4.f));
	UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Frame->SetContent(Stack);
	MapImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	UOverlaySlot* ImgSlot = Stack->AddChildToOverlay(GlossaryUIKit::WrapSize(WidgetTree, MapImage, MapSize, MapSize));
	ImgSlot->SetHorizontalAlignment(HAlign_Fill);
	ImgSlot->SetVerticalAlignment(VAlign_Fill);
	// 플레이어 = 가운데 화살표 (지도는 플레이어를 따라 움직이므로 항상 가운데)
	Arrow = MakeText(TEXT("▲"), 26.f, FLinearColor(0.2f, 0.6f, 1.f), true);
	Arrow->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	UOverlaySlot* ArrowSlot = Stack->AddChildToOverlay(Arrow);
	ArrowSlot->SetHorizontalAlignment(HAlign_Center);
	ArrowSlot->SetVerticalAlignment(VAlign_Center);
	Col->AddChildToVerticalBox(Frame);

	Caption = MakeText(TEXT(""), 13.f, FLinearColor(0.9f, 0.9f, 0.9f));
	Col->AddChildToVerticalBox(Caption)->SetHorizontalAlignment(HAlign_Right);
}

void UDemo0614_MinimapWidget::SetRenderTarget(UTextureRenderTarget2D* Target)
{
	// 핵심: Render Target 을 Image 브러시의 리소스로 바로 쓴다 (머티리얼 없이도 된다)
	MapImage->SetBrushResourceObject(Target);
	MapImage->SetDesiredSizeOverride(FVector2D(MapSize, MapSize));
}

void UDemo0614_MinimapWidget::SetPlayerYaw(float Yaw)
{
	if (Arrow)
	{
		// 월드 +X(앞) = 지도 위쪽. Yaw 가 늘면 시계 방향으로 돈다
		Arrow->SetRenderTransformAngle(Yaw);
	}
}

void UDemo0614_MinimapWidget::SetCaption(const FString& Text)
{
	if (Caption)
	{
		Caption->SetText(FText::FromString(Text));
	}
}

// ---------------------------------------------------------------- 데모

ADemo0614_Minimap::ADemo0614_Minimap()
{
	DemoId = TEXT("0614");
	Controls = TEXT("WASD 이동 · 마우스 시점   Z / X : 미니맵 확대 / 축소 (Ortho Width)   R : 회전 미니맵 ON/OFF\n")
		TEXT("관찰: 오른쪽 위 지도는 머리 위 직교 카메라가 실시간으로 찍은 Render Target. 움직이면 지도가 따라오고 화살표가 방향을 가리킨다");
	Showcase = TEXT("플레이어 위 SceneCaptureComponent2D(Orthographic) → Render Target → 오른쪽 위 Image 미니맵, 확대·회전");

	MinimapCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MinimapCapture"));
	MinimapCapture->SetupAttachment(Root);
	MinimapCapture->SetUsingAbsoluteLocation(true);
	MinimapCapture->SetUsingAbsoluteRotation(true);
	MinimapCapture->ProjectionType = ECameraProjectionMode::Orthographic;  // 원근 없는 지도
	MinimapCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	MinimapCapture->bCaptureEveryFrame = true;
	MinimapCapture->bCaptureOnMovement = false;
	// 지도에 필요 없는 것은 끈다 (안개·대기·구름)
	MinimapCapture->ShowFlags.SetFog(false);
	MinimapCapture->ShowFlags.SetAtmosphere(false);
	MinimapCapture->ShowFlags.SetCloud(false);
}

void ADemo0614_Minimap::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	// 지도에 보일 것들: 적(빨강)·아이템(노랑)·벽
	for (int32 i = 0; i < 6; ++i)
	{
		const float A = i * 60.f;
		SpawnShape(World, EShape::Cylinder, FVector(FMath::Cos(FMath::DegreesToRadians(A)) * 1400.f, FMath::Sin(FMath::DegreesToRadians(A)) * 1400.f, 90.f),
			FVector(120.f, 120.f, 180.f), Colors::Enemy);
	}
	for (int32 i = 0; i < 5; ++i)
	{
		SpawnShape(World, EShape::Sphere, FVector(-400.f + i * 450.f, 700.f - i * 300.f, 50.f), FVector(100.f), Colors::Item);
	}
	SpawnBlock(World, FVector(800.f, -600.f, 0.f), FVector(200.f, 1200.f, 300.f), Colors::Wall);
	SpawnBlock(World, FVector(-1200.f, 400.f, 0.f), FVector(1200.f, 200.f, 300.f), Colors::Wall);
	SpawnBlock(World, FVector(300.f, 300.f, 0.f), FVector(400.f, 400.f, 150.f), Colors::Platform);

	// Render Target 을 실행 중에 만든다 (에셋으로 만들어 두고 지정해도 같다)
	RenderTarget = NewObject<UTextureRenderTarget2D>(this);
	RenderTarget->RenderTargetFormat = RTF_RGBA8;
	RenderTarget->ClearColor = FLinearColor::Black;
	RenderTarget->InitAutoFormat(RenderTargetSize, RenderTargetSize);
	RenderTarget->UpdateResourceImmediate(true);
	MinimapCapture->TextureTarget = RenderTarget;
	MinimapCapture->OrthoWidth = OrthoWidth;

	MinimapWidget = CreateWidget<UDemo0614_MinimapWidget>(GetPC(), UDemo0614_MinimapWidget::StaticClass());
	MinimapWidget->AddToViewport(0);
	MinimapWidget->SetRenderTarget(RenderTarget);
}

void ADemo0614_Minimap::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MinimapWidget)
	{
		MinimapWidget->RemoveFromParent();
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0614_Minimap::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::Z)) { OrthoWidth = FMath::Max(1000.f, OrthoWidth * 0.75f); }
	if (WasKeyPressed(EKeys::X)) { OrthoWidth = FMath::Min(12000.f, OrthoWidth / 0.75f); }
	if (WasKeyPressed(EKeys::R)) { bRotateWithPlayer = !bRotateWithPlayer; }
	MinimapCapture->OrthoWidth = OrthoWidth;

	APawn* Pawn = GetPlayerPawn();
	if (!Pawn || !MinimapWidget)
	{
		return;
	}
	// 플레이어 머리 위에서 수직으로 내려다본다. 화면 위쪽 = 월드 +X (회전 미니맵이면 플레이어 정면)
	const float ViewYaw = GetPC()->GetControlRotation().Yaw;
	const float MapYaw = bRotateWithPlayer ? ViewYaw : 0.f;
	MinimapCapture->SetWorldLocationAndRotation(Pawn->GetActorLocation() + FVector(0.f, 0.f, CaptureHeight), FRotator(-90.f, MapYaw, 0.f));
	MinimapWidget->SetPlayerYaw(bRotateWithPlayer ? 0.f : Pawn->GetActorRotation().Yaw);
	MinimapWidget->SetCaption(FString::Printf(TEXT("Ortho Width %.0fm · %s"), OrthoWidth / 100.f, bRotateWithPlayer ? TEXT("회전 미니맵") : TEXT("북쪽 고정")));
	SetStatus(FString::Printf(TEXT("SceneCaptureComponent2D Orthographic · Render Target %d×%d\nOrtho Width %.0fcm · 높이 %.0fcm · %s"),
		RenderTargetSize, RenderTargetSize, OrthoWidth, CaptureHeight, bRotateWithPlayer ? TEXT("회전 미니맵") : TEXT("북쪽(+X) 고정")));
}
