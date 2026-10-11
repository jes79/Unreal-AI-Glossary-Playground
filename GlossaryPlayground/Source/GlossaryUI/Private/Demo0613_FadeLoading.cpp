#include "Demo0613_FadeLoading.h"

#include "Blueprint/WidgetTree.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "GlossaryUIKit.h"
#include "Kismet/GameplayStatics.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- 로딩 위젯

void UDemo0613_LoadingWidget::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UBorder* Back = MakeBorder(FLinearColor::Black, FMargin(0.f));
	AddToCanvas(Canvas, Back, FAnchors(0.f, 0.f, 1.f, 1.f), FVector2D::ZeroVector)->SetOffsets(FMargin(0.f));

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	AddToCanvas(Canvas, Col, FAnchors(1.f, 1.f), FVector2D(-80.f, -80.f), FVector2D::ZeroVector, FVector2D(1.f, 1.f));
	Spinner = MakeText(TEXT("◐"), 40.f, FLinearColor(1.f, 0.85f, 0.35f), true);
	Spinner->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Col->AddChildToVerticalBox(Spinner)->SetHorizontalAlignment(HAlign_Right);
	Col->AddChildToVerticalBox(MakeText(TEXT("LOADING"), 34.f, FLinearColor::White, true))->SetHorizontalAlignment(HAlign_Right);
	Message = MakeText(TEXT(""), 16.f, FLinearColor(0.7f, 0.75f, 0.8f));
	Col->AddChildToVerticalBox(Message)->SetHorizontalAlignment(HAlign_Right);
	Bar = GlossaryUIKit::MakeBar(WidgetTree, FLinearColor(1.f, 0.85f, 0.35f), FLinearColor(0.15f, 0.15f, 0.15f, 1.f));
	Col->AddChildToVerticalBox(GlossaryUIKit::WrapSize(WidgetTree, Bar, 480.f, 8.f))->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));

	UTextBlock* Tip = MakeText(TEXT("팁: OpenLevel 은 동기 로딩이라 실제 로딩 동안은 위젯이 멈춥니다.\n긴 로딩에는 MoviePlayer 로딩 화면이나 비동기 스트리밍을 씁니다."),
		15.f, FLinearColor(0.55f, 0.6f, 0.65f));
	AddToCanvas(Canvas, Tip, FAnchors(0.f, 1.f), FVector2D(80.f, -80.f), FVector2D::ZeroVector, FVector2D(0.f, 1.f));
}

void UDemo0613_LoadingWidget::SetProgress(float Percent, const FString& InMessage)
{
	if (Bar)
	{
		Bar->SetPercent(Percent);
		Message->SetText(FText::FromString(InMessage));
	}
}

void UDemo0613_LoadingWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	SpinTime += InDeltaTime;
	if (Spinner)
	{
		Spinner->SetRenderTransformAngle(SpinTime * 360.f);
	}
}

// ---------------------------------------------------------------- 데모

ADemo0613_FadeLoading::ADemo0613_FadeLoading()
{
	DemoId = TEXT("0613");
	Controls = TEXT("L : 페이드 아웃 → 로딩 화면 → 같은 레벨 다시 열기 (OpenLevel)\n")
		TEXT("관찰: 0.5초 동안 검게 가려지고 로딩 위젯이 뜬 뒤 레벨이 새로 열리며 페이드 인. 상태 줄의 '다시 열린 횟수' 가 오른다");
	Showcase = TEXT("StartCameraFade 0.5초 페이드 아웃 → 로딩 위젯 → OpenLevel 로 같은 레벨 재시작 → 페이드 인");
}

void ADemo0613_FadeLoading::BeginPlay()
{
	Super::BeginPlay();
	// 레벨을 열 때 넘긴 옵션(?ReloadCount=N)을 읽어 몇 번째로 열렸는지 표시
	if (AGameModeBase* GM = GetWorld()->GetAuthGameMode())
	{
		ReloadCount = UGameplayStatics::GetIntOption(GM->OptionsString, TEXT("ReloadCount"), 0);
	}
	const FLinearColor Palette[] = { Colors::Cyan, Colors::Accent, Colors::Purple, Colors::Ally };
	SpawnShape(GetWorld(), EShape::Sphere, FVector(300.f, 0.f, 100.f), FVector(200.f), Palette[ReloadCount % 4]);
	SpawnLabel(GetWorld(), FString::Printf(TEXT("레벨이 열린 횟수: %d"), ReloadCount + 1), FVector(300.f, 0.f, 280.f), FLinearColor::White, 22.f);

	if (ReloadCount > 0)
	{
		// 새 레벨: 검은 화면에서 시작해 FadeDuration 동안 밝아진다
		if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			Cam->StartCameraFade(1.f, 0.f, FadeDuration, FLinearColor::Black, false, false);
		}
	}
}

void ADemo0613_FadeLoading::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Loading)
	{
		Loading->RemoveFromParent();
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0613_FadeLoading::StartTransition()
{
	APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (Stage != EStage::Idle || !Cam)
	{
		return;
	}
	// 핵심 1: 0 → 1(검정) 으로 0.5초 페이드. bHoldWhenFinished=true 라 끝나도 검게 유지
	Cam->StartCameraFade(0.f, 1.f, FadeDuration, FLinearColor::Black, false, true);
	Stage = EStage::FadingOut;
	StageTime = 0.f;
}

void ADemo0613_FadeLoading::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::L))
	{
		StartTransition();
	}
	StageTime += DeltaSeconds;
	switch (Stage)
	{
	case EStage::FadingOut:
		if (StageTime >= FadeDuration)
		{
			// 핵심 2: 화면이 완전히 검어진 뒤 로딩 위젯을 띄운다 (카메라 페이드보다 위에 그려짐)
			Loading = CreateWidget<UDemo0613_LoadingWidget>(GetPC(), UDemo0613_LoadingWidget::StaticClass());
			Loading->AddToViewport(200);
			Stage = EStage::Loading;
			StageTime = 0.f;
		}
		break;
	case EStage::Loading:
		Loading->SetProgress(FMath::Clamp(StageTime / LoadingMinTime, 0.f, 1.f), TEXT("레벨 준비 중 (진행률은 연출용)"));
		if (StageTime >= LoadingMinTime)
		{
			Stage = EStage::Opening;
			StageTime = 0.f;
		}
		break;
	case EStage::Opening:
		// 한 프레임 쉬어 로딩 위젯이 100% 로 그려진 뒤 OpenLevel. 같은 레벨 패키지 경로 + 옵션
		if (StageTime > 0.f)
		{
			const FString LevelPath = GetWorld()->GetOutermost()->GetName();
			UGameplayStatics::OpenLevel(this, FName(*LevelPath), true, FString::Printf(TEXT("ReloadCount=%d"), ReloadCount + 1));
			Stage = EStage::Idle; // 이 월드는 곧 사라진다
		}
		break;
	default:
		break;
	}

	static const TCHAR* StageNames[] = { TEXT("대기"), TEXT("페이드 아웃"), TEXT("로딩 위젯"), TEXT("OpenLevel") };
	SetStatus(FString::Printf(TEXT("단계: %s (%.2fs) · FadeDuration %.1fs\n이 레벨이 열린 횟수 %d (OpenLevel 옵션 ReloadCount=%d)"),
		StageNames[static_cast<int32>(Stage)], StageTime, FadeDuration, ReloadCount + 1, ReloadCount));
}
