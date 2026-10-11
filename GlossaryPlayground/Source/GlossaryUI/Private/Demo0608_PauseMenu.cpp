#include "Demo0608_PauseMenu.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "GlossaryCatalog.h"
#include "GlossaryKit.h"
#include "GlossaryUIKit.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "UnrealClient.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- 위젯

void UDemo0608_PauseWidget::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	// 화면 전체를 어둡게 (Anchor 0~1 늘이기, 여백 0)
	UBorder* Dim = MakeBorder(FLinearColor(0.f, 0.f, 0.f, 0.55f), FMargin(0.f));
	AddToCanvas(Canvas, Dim, FAnchors(0.f, 0.f, 1.f, 1.f), FVector2D::ZeroVector, FVector2D(0.f, 0.f))->SetOffsets(FMargin(0.f));

	UBorder* Panel = MakeBorder(FLinearColor(0.05f, 0.06f, 0.1f, 0.95f), FMargin(32.f, 24.f));
	AddToCanvas(Canvas, Panel, FAnchors(0.5f, 0.5f), FVector2D(200.f, 0.f), FVector2D::ZeroVector, FVector2D(0.5f, 0.5f));
	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Col);
	UTextBlock* Title = MakeText(TEXT("일시정지"), 34.f, FLinearColor(1.f, 0.85f, 0.35f), true);
	Title->SetJustification(ETextJustify::Center);
	Col->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));

	ResumeButton = MakeButton(TEXT("계속하기 (ESC)"), 20.f, FLinearColor(0.15f, 0.4f, 0.2f, 1.f));
	Col->AddChildToVerticalBox(GlossaryUIKit::WrapSize(WidgetTree, ResumeButton, 320.f, 56.f))->SetPadding(FMargin(0.f, 6.f));
	UButton* HubButton = MakeButton(TEXT("허브로 나가기"), 20.f);
	Col->AddChildToVerticalBox(GlossaryUIKit::WrapSize(WidgetTree, HubButton, 320.f, 56.f))->SetPadding(FMargin(0.f, 6.f));
	Col->AddChildToVerticalBox(MakeText(TEXT("Input Mode: UI Only · 커서 표시 · 월드 정지"), 13.f, FLinearColor(0.7f, 0.8f, 1.f)))
		->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));

	ResumeButton->OnClicked.AddDynamic(this, &UDemo0608_PauseWidget::HandleResume);
	HubButton->OnClicked.AddDynamic(this, &UDemo0608_PauseWidget::HandleHub);
}

void UDemo0608_PauseWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true); // FInputModeUIOnly 로 포커스를 받으려면 필요
}

FReply UDemo0608_PauseWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::Gamepad_Special_Right)
	{
		HandleResume();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UDemo0608_PauseWidget::HandleResume()
{
	if (Demo.IsValid())
	{
		Demo->SetPaused(false);
	}
}

void UDemo0608_PauseWidget::HandleHub()
{
	UGameplayStatics::SetGamePaused(this, false);
	UGameplayStatics::OpenLevel(this, FName(UGlossaryCatalog::HubLevelPackage()));
}

// ---------------------------------------------------------------- 데모

ADemo0608_PauseMenu::ADemo0608_PauseMenu()
{
	DemoId = TEXT("0608");
	Controls = TEXT("WASD 이동 · 마우스 시점   ESC (또는 P) : 일시정지 / 계속\n")
		TEXT("관찰: 멈추면 회전하던 기둥과 월드 시간이 멈추고 커서가 나타나 버튼만 누를 수 있다. 다시 ESC 면 Game Only 로 복귀");
	Showcase = TEXT("ESC 로 SetGamePaused + Input Mode UI Only + 커서, 다시 누르면 Game Only 로 복귀. 월드 시간 정지 표시");
	// 일시정지 중에도 이 액터는 Tick 해야 ESC 를 읽고 상태를 표시할 수 있다
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

void ADemo0608_PauseMenu::BeginPlay()
{
	Super::BeginPlay();
	// 계속 도는 기둥들: 일시정지하면 멈춘다 (RotatingMovement 는 일시정지 중 Tick 하지 않음)
	for (int32 i = 0; i < 4; ++i)
	{
		AStaticMeshActor* Pillar = SpawnShape(GetWorld(), EShape::Cube, FVector(400.f, -450.f + 300.f * i, 150.f), FVector(60.f, 220.f, 300.f),
			i % 2 ? Colors::Accent : Colors::Cyan);
		URotatingMovementComponent* Rot = NewObject<URotatingMovementComponent>(Pillar);
		Rot->RotationRate = FRotator(0.f, 90.f + 30.f * i, 0.f);
		Rot->RegisterComponent();
	}
	PauseWidget = CreateWidget<UDemo0608_PauseWidget>(GetPC(), UDemo0608_PauseWidget::StaticClass());
	PauseWidget->Demo = this;
}

void ADemo0608_PauseMenu::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PauseWidget)
	{
		PauseWidget->RemoveFromParent();
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0608_PauseMenu::SetPaused(bool bPause)
{
	APlayerController* PC = GetPC();
	if (!PC || bPause == bPausedNow)
	{
		return;
	}
	bPausedNow = bPause;
	if (bPause)
	{
		++PauseCount;
		PauseWidget->AddToViewport(50);
		// 1) 월드 정지  2) 입력을 UI 에만  3) 커서 표시
		UGameplayStatics::SetGamePaused(this, true);
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(PauseWidget->TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);
	}
	else
	{
		PauseWidget->RemoveFromParent();
		UGameplayStatics::SetGamePaused(this, false);
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
}

void ADemo0608_PauseMenu::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// 검증용: -GlossaryShot 실행에서 일시정지 상태면 (게임 시간이 멈춰 공용 캡처가 안 되므로) 실제 시간 1.5초 뒤 직접 캡처 후 종료
	FString ShotPath;
	if (bPausedNow && FParse::Value(FCommandLine::Get(), TEXT("GlossaryShot="), ShotPath))
	{
		PausedRealTime += FApp::GetDeltaTime();
		if (PausedShotStage == 0 && PausedRealTime > 1.5f) { FScreenshotRequest::RequestScreenshot(ShotPath, true, false); PausedShotStage = 1; }
		else if (PausedShotStage == 1 && PausedRealTime > 3.f) { PausedShotStage = 2; GetPC()->ConsoleCommand(TEXT("quit")); }
	}
	// Game Only 일 때 ESC 는 PlayerController 로 온다 (UI Only 일 때는 위젯의 NativeOnKeyDown 이 받는다)
	if (WasKeyPressed(EKeys::Escape) || WasKeyPressed(EKeys::P) || WasKeyPressed(EKeys::Gamepad_Special_Right))
	{
		SetPaused(!bPausedNow);
	}
	APlayerController* PC = GetPC();
	SetStatus(FString::Printf(TEXT("IsGamePaused %s · Input Mode %s · 커서 %s · 일시정지 %d회\n월드 시간 %.1fs%s · 실제 시간 %.1fs"),
		UGameplayStatics::IsGamePaused(this) ? TEXT("true") : TEXT("false"),
		bPausedNow ? TEXT("UI Only") : TEXT("Game Only"),
		PC && PC->bShowMouseCursor ? TEXT("보임") : TEXT("숨김"), PauseCount,
		GetWorld()->GetTimeSeconds(), bPausedNow ? TEXT(" (멈춤)") : TEXT(""), GetWorld()->GetRealTimeSeconds()));
}
