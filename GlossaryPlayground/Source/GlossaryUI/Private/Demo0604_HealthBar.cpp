#include "Demo0604_HealthBar.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryKit.h"
#include "GlossaryUIKit.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- 위젯

void UDemo0604_DelayedHealthBar::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UBorder* Panel = MakeBorder(FLinearColor(0.f, 0.f, 0.f, 0.6f), FMargin(16.f, 12.f));
	AddToCanvas(Canvas, Panel, FAnchors(0.5f, 1.f), FVector2D(0.f, -60.f), FVector2D::ZeroVector, FVector2D(0.5f, 1.f));

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Col);
	Caption = MakeText(TEXT(""), 18.f, FLinearColor::White, true);
	Col->AddChildToVerticalBox(Caption)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));

	// 겹치기: 아래에 흰 고스트 바(배경 어두움), 위에 빨간 바(배경 투명)
	UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	GhostBar = GlossaryUIKit::MakeBar(WidgetTree, FLinearColor(1.f, 1.f, 1.f, 0.95f), FLinearColor(0.08f, 0.08f, 0.08f, 1.f));
	FrontBar = GlossaryUIKit::MakeBar(WidgetTree, FLinearColor(0.9f, 0.08f, 0.06f), FLinearColor(0.f, 0.f, 0.f, 0.f));
	for (UWidget* W : { static_cast<UWidget*>(GhostBar), static_cast<UWidget*>(FrontBar) })
	{
		UOverlaySlot* OSlot = Stack->AddChildToOverlay(W);
		OSlot->SetHorizontalAlignment(HAlign_Fill);
		OSlot->SetVerticalAlignment(VAlign_Fill);
	}
	Col->AddChildToVerticalBox(GlossaryUIKit::WrapSize(WidgetTree, Stack, 720.f, 40.f));
}

void UDemo0604_DelayedHealthBar::ApplyHealth(float NewPercent)
{
	if (NewPercent < FrontPercent)
	{
		DelayRemaining = GhostDelay; // 피격마다 대기 시간을 다시 건다 (연타 중에는 흰 바가 멈춰 있다)
	}
	FrontPercent = NewPercent;
	if (GhostPercent < FrontPercent || !bDelayedGhost)
	{
		GhostPercent = FrontPercent; // 회복이나 지연 끔: 흰 바도 바로 맞춘다
	}
	FrontBar->SetPercent(FrontPercent);
	GhostBar->SetPercent(GhostPercent);
}

void UDemo0604_DelayedHealthBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (DelayRemaining > 0.f)
	{
		DelayRemaining -= InDeltaTime;
		return;
	}
	if (GhostPercent > FrontPercent)
	{
		// 핵심: 0.5초 기다린 뒤 FInterpTo 로 빨간 바 위치까지 부드럽게 따라 내려간다
		GhostPercent = FMath::FInterpTo(GhostPercent, FrontPercent, InDeltaTime, GhostInterpSpeed);
		if (GhostPercent - FrontPercent < 0.001f)
		{
			GhostPercent = FrontPercent;
		}
		GhostBar->SetPercent(GhostPercent);
	}
}

// ---------------------------------------------------------------- 데모

ADemo0604_HealthBar::ADemo0604_HealthBar()
{
	DemoId = TEXT("0604");
	Controls = TEXT("Space : 피격 -15 (1.5초마다 자동 피격도 있음)   H : 회복\n")
		TEXT("C : 지연 고스트 바 ON/OFF\n")
		TEXT("관찰: 빨간 바는 즉시 줄고, 흰 바는 0.5초 멈췄다가 FInterpTo 로 따라 내려와 '얼마나 깎였는지' 가 보인다");
	Showcase = TEXT("빨간 바 즉시 감소 + 흰 고스트 바 0.5초 뒤 FInterpTo 추적, 지연 ON/OFF 비교");
	PlayerPawnClass = nullptr;
}

void ADemo0604_HealthBar::BeginPlay()
{
	Super::BeginPlay();
	GlossaryUIKit::SetupFixedCamera(this, FVector(-650.f, 0.f, 260.f), FRotator(-12.f, 0.f, 0.f)); // 고정 카메라
	Dummy = SpawnShape(GetWorld(), EShape::Cylinder, FVector(-100.f, 0.f, 90.f), FVector(100.f, 100.f, 180.f), Colors::Enemy);
	SpawnLabel(GetWorld(), TEXT("훈련용 표적"), FVector(-100.f, 0.f, 230.f), FLinearColor::White, 18.f);

	Health = MaxHealth;
	Bar = CreateWidget<UDemo0604_DelayedHealthBar>(GetPC(), UDemo0604_DelayedHealthBar::StaticClass());
	Bar->GhostDelay = GhostDelay;
	Bar->GhostInterpSpeed = GhostInterpSpeed;
	Bar->AddToViewport(0);
}

void ADemo0604_HealthBar::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Bar)
	{
		Bar->RemoveFromParent();
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0604_HealthBar::Hit()
{
	Health = Health <= HitDamage ? MaxHealth : Health - HitDamage; // 바닥나면 다시 가득
	Bar->ApplyHealth(Health / MaxHealth);
	FlashTime = 0.12f;
}

void ADemo0604_HealthBar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Bar)
	{
		return;
	}
	HitTimer += DeltaSeconds;
	if (HitTimer >= AutoHitInterval || WasKeyPressed(EKeys::SpaceBar))
	{
		HitTimer = 0.f;
		Hit();
	}
	if (WasKeyPressed(EKeys::H))
	{
		Health = MaxHealth;
		Bar->ApplyHealth(1.f);
	}
	if (WasKeyPressed(EKeys::C))
	{
		Bar->bDelayedGhost = !Bar->bDelayedGhost;
		Bar->ApplyHealth(Health / MaxHealth);
	}

	// 맞은 순간 표적이 하얗게 번쩍
	FlashTime -= DeltaSeconds;
	if (Dummy)
	{
		SetColor(Dummy->GetStaticMeshComponent(), FlashTime > 0.f ? FLinearColor::White : Colors::Enemy, 0.6f, 0.f, FlashTime > 0.f ? 2.f : 0.f);
	}

	const float Lag = (Bar->GetGhostPercent() - Bar->GetFrontPercent()) * MaxHealth;
	if (Bar->Caption)
	{
		Bar->Caption->SetText(FText::FromString(FString::Printf(TEXT("HP %.0f / %.0f   %s"), Health, MaxHealth,
			Bar->bDelayedGhost ? *FString::Printf(TEXT("(흰 바가 %.1f 만큼 뒤따라오는 중)"), Lag) : TEXT("(지연 끔: 즉시 감소만)"))));
	}
	SetStatus(FString::Printf(TEXT("지연 고스트 %s · GhostDelay %.1fs · InterpSpeed %.1f\n빨간 바 %.2f · 흰 바 %.2f"),
		Bar->bDelayedGhost ? TEXT("ON") : TEXT("OFF"), GhostDelay, GhostInterpSpeed, Bar->GetFrontPercent(), Bar->GetGhostPercent()));
}
