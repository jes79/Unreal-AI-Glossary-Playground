#include "Demo0602_AnchorDPI.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/UserInterfaceSettings.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "GlossaryUIKit.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- HUD

void UDemo0602_AnchorHud::AddAnchored(UCanvasPanel* Canvas, const FString& Label, const FAnchors& Anchors, const FVector2D& Alignment,
	const FVector2D& Position, const FVector2D& Size, const FLinearColor& Color)
{
	UBorder* Box = MakeBorder(Color, FMargin(8.f));
	UTextBlock* Text = MakeText(Label, 15.f, FLinearColor::White, true);
	Text->SetAutoWrapText(true);
	Box->SetContent(Text);
	AddToCanvas(Canvas, Box, Anchors, Position, Size, Alignment);

	// 앵커 지점(이 위젯이 붙어 있는 화면 기준점)을 노란 점으로 표시
	if (!Anchors.IsStretchedHorizontal())
	{
		UBorder* Dot = MakeBorder(FLinearColor(1.f, 0.9f, 0.1f), FMargin(0.f));
		AddToCanvas(Canvas, Dot, FAnchors(Anchors.Minimum.X, Anchors.Minimum.Y), FVector2D::ZeroVector, FVector2D(14.f, 14.f), FVector2D(0.5f, 0.5f));
	}
}

void UDemo0602_AnchorHud::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	const FLinearColor Good(0.05f, 0.35f, 0.15f, 0.85f);

	// 요청: 체력바는 왼쪽 위 Anchor — 정보 패널(왼쪽 위 620×440) 아래로 Position 만 내린다. Anchor 는 여전히 (0,0)
	AddAnchored(Canvas, TEXT("체력바  Anchor(0,0) 왼쪽 위"), FAnchors(0.f, 0.f), FVector2D(0.f, 0.f), FVector2D(24.f, 470.f), FVector2D(360.f, 56.f),
		FLinearColor(0.6f, 0.08f, 0.08f, 0.9f));
	// 요청: 미니맵은 오른쪽 위 Anchor. Alignment(1,0) 이라 상자의 오른쪽 위 모서리가 앵커에 붙는다
	AddAnchored(Canvas, TEXT("미니맵\nAnchor(1,0) 오른쪽 위\nAlignment(1,0)"), FAnchors(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(-24.f, 24.f), FVector2D(260.f, 260.f), Good);
	AddAnchored(Canvas, TEXT("스킬 슬롯  Anchor(0,1)"), FAnchors(0.f, 1.f), FVector2D(0.f, 1.f), FVector2D(24.f, -24.f), FVector2D(300.f, 70.f), Good);
	AddAnchored(Canvas, TEXT("탄약  Anchor(1,1)"), FAnchors(1.f, 1.f), FVector2D(1.f, 1.f), FVector2D(-24.f, -24.f), FVector2D(220.f, 70.f), Good);
	AddAnchored(Canvas, TEXT("+ 조준점 Anchor(0.5,0.5)"), FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector, FVector2D(230.f, 40.f),
		FLinearColor(0.1f, 0.1f, 0.1f, 0.6f));
	// 가로로 늘어나는 Anchor(0,1)-(1,1): Position/Size 가 왼쪽·오른쪽 여백으로 쓰인다
	AddAnchored(Canvas, TEXT("경험치 바  Anchor 가로 늘이기 (0,1)~(1,1)  좌우 여백 360"), FAnchors(0.f, 1.f, 1.f, 1.f), FVector2D(0.f, 1.f),
		FVector2D(360.f, -110.f), FVector2D(360.f, 40.f), FLinearColor(0.15f, 0.2f, 0.5f, 0.85f));

	// 나쁜 예: Anchor 를 신경 쓰지 않고 1920×1080 화면 기준 절대 좌표로 둔 상자
	UBorder* Bad = MakeBorder(FLinearColor(0.5f, 0.1f, 0.4f, 0.9f), FMargin(8.f));
	Bad->SetContent(MakeText(TEXT("나쁜 예: Anchor(0,0) + 절대 좌표 (1480, 620)\n해상도가 작아지고 DPI 를 끄면 화면 밖으로"), 14.f));
	AddToCanvas(Canvas, Bad, FAnchors(0.f, 0.f), FVector2D(1480.f, 620.f), FVector2D(400.f, 70.f));

	ScaleText = MakeText(TEXT(""), 16.f, FLinearColor(1.f, 0.9f, 0.4f), true);
	UBorder* ScalePanel = MakeBorder(FLinearColor(0.f, 0.f, 0.f, 0.6f), FMargin(10.f, 6.f));
	ScalePanel->SetContent(ScaleText);
	AddToCanvas(Canvas, ScalePanel, FAnchors(0.5f, 0.f), FVector2D(0.f, 24.f), FVector2D::ZeroVector, FVector2D(0.5f, 0.f));
}

// ---------------------------------------------------------------- 데모

ADemo0602_AnchorDPI::ADemo0602_AnchorDPI()
{
	DemoId = TEXT("0602");
	Controls = TEXT("3 : 1280×720   4 : 1600×900   5 : 1920×1080 (창 해상도 변경)\n")
		TEXT("D : DPI Scaling 커브 ON/OFF (1080p 기준 1.0 ↔ 항상 1.0)\n")
		TEXT("관찰: Anchor 위젯은 해상도가 바뀌어도 모서리에 붙어 있고, DPI 를 켜면 크기 비율도 유지된다");
	Showcase = TEXT("네 모서리·가운데·가로 늘이기 Anchor 와 절대 좌표 나쁜 예, 해상도 전환 + DPI 커브 ON/OFF 와 배율 표시");
	PlayerPawnClass = nullptr;
	bShowMouseCursor = true;
}

void ADemo0602_AnchorDPI::BeginPlay()
{
	Super::BeginPlay();
	GlossaryUIKit::SetupFixedCamera(this, FVector(-650.f, 0.f, 260.f), FRotator(-12.f, 0.f, 0.f)); // 고정 카메라
	SpawnShape(GetWorld(), EShape::Cube, FVector(-100.f, 0.f, 60.f), FVector(120.f), Colors::Platform);

	UUserInterfaceSettings* UISettings = GetMutableDefault<UUserInterfaceSettings>();
	SavedCurve = UISettings->UIScaleCurve;
	SavedRule = static_cast<uint8>(UISettings->UIScaleRule);
	ApplyDpiCurve(true);

	Hud = CreateWidget<UDemo0602_AnchorHud>(GetPC(), UDemo0602_AnchorHud::StaticClass());
	Hud->AddToViewport(0);
}

void ADemo0602_AnchorDPI::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 프로젝트 설정(메모리상의 CDO)을 원래대로 돌려놓는다
	UUserInterfaceSettings* UISettings = GetMutableDefault<UUserInterfaceSettings>();
	UISettings->UIScaleCurve = SavedCurve;
	UISettings->UIScaleRule = static_cast<EUIScalingRule>(SavedRule);
	if (Hud)
	{
		Hud->RemoveFromParent();
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0602_AnchorDPI::ApplyDpiCurve(bool bUse1080Curve)
{
	// Project Settings > Engine > User Interface > DPI Scaling 과 같은 값. 여기서는 실행 중에 CDO 를 바꿔 즉시 비교한다.
	// (에디터 빌드는 배율을 캐시하지 않아 바로 반영된다. 패키지 빌드에서는 ini 로 설정하는 것이 정석)
	UUserInterfaceSettings* UISettings = GetMutableDefault<UUserInterfaceSettings>();
	UISettings->UIScaleRule = EUIScalingRule::ShortestSide;
	FRichCurve* Curve = UISettings->UIScaleCurve.GetRichCurve();
	Curve->Reset();
	if (bUse1080Curve)
	{
		// 짧은 변 1080 = 1.0 (720 → 0.667, 2160 → 2.0)
		Curve->AddKey(480.f, 480.f / ReferenceShortSide);
		Curve->AddKey(ReferenceShortSide, 1.f);
		Curve->AddKey(ReferenceShortSide * 2.f, 2.f);
	}
	else
	{
		Curve->AddKey(0.f, 1.f); // DPI 끔: 어떤 해상도에서도 1.0 (픽셀 = Slate 단위)
	}
	bDpiOn = bUse1080Curve;
}

void ADemo0602_AnchorDPI::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APlayerController* PC = GetPC();
	if (WasKeyPressed(EKeys::Three)) { PC->ConsoleCommand(TEXT("r.SetRes 1280x720w")); }
	if (WasKeyPressed(EKeys::Four))  { PC->ConsoleCommand(TEXT("r.SetRes 1600x900w")); }
	if (WasKeyPressed(EKeys::Five))  { PC->ConsoleCommand(TEXT("r.SetRes 1920x1080w")); }
	if (WasKeyPressed(EKeys::D))     { ApplyDpiCurve(!bDpiOn); }

	// GetViewportScale = 지금 적용 중인 DPI 배율. Slate 단위 = 픽셀 / 배율
	const FVector2D Pixels = UWidgetLayoutLibrary::GetViewportSize(this);
	const float Scale = UWidgetLayoutLibrary::GetViewportScale(this);
	const FString Line = FString::Printf(TEXT("뷰포트 %.0f×%.0f px · DPI 배율 %.3f · UI 좌표계 %.0f×%.0f · 커브 %s"),
		Pixels.X, Pixels.Y, Scale, Pixels.X / FMath::Max(Scale, 0.01f), Pixels.Y / FMath::Max(Scale, 0.01f),
		bDpiOn ? TEXT("1080p 기준") : TEXT("끔(1.0)"));
	if (Hud && Hud->ScaleText)
	{
		Hud->ScaleText->SetText(FText::FromString(Line));
	}
	SetStatus(Line);
}
