#include "Demo0601_WidgetBlueprint.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GlossaryKit.h"
#include "GlossaryUIKit.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- HUD 베이스 (로직만, 레이아웃 없음)

void UDemo0601_HudBase::SetHealth(float Current, float Max)
{
	// BindWidget 덕분에 FindWidget 같은 검색 없이 바로 쓴다
	HealthBar->SetPercent(Max > 0.f ? Current / Max : 0.f);
	HealthText->SetText(FText::FromString(FString::Printf(TEXT("HP %.0f / %.0f"), Current, Max)));
}

void UDemo0601_HudBase::SetScore(int32 Score)
{
	ScoreText->SetText(FText::FromString(FString::Printf(TEXT("SCORE %06d"), Score)));
}

// ---------------------------------------------------------------- 코드로 만든 레이아웃 (WBP 디자이너 대신)

TSharedRef<SWidget> UDemo0601_CodeBuiltHud::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UDemo0601_CodeBuiltHud::BuildTree()
{
	using namespace GlossaryUIKit;
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = Canvas;

	// 왼쪽 아래: 체력바 (Anchor 왼쪽 아래, 정보 패널과 겹치지 않음)
	UBorder* HealthPanel = MakeBorder(WidgetTree, FLinearColor(0.f, 0.f, 0.f, 0.55f), FMargin(12.f, 8.f));
	UVerticalBox* HealthColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	HealthPanel->SetContent(HealthColumn);
	HealthText = MakeText(WidgetTree, TEXT(""), 18.f, FLinearColor::White, true);
	HealthColumn->AddChildToVerticalBox(HealthText);
	HealthBar = MakeBar(WidgetTree, FLinearColor(0.85f, 0.1f, 0.1f), FLinearColor(0.1f, 0.1f, 0.1f, 0.9f));
	HealthColumn->AddChildToVerticalBox(WrapSize(WidgetTree, HealthBar, 360.f, 26.f))->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	UCanvasPanelSlot* HealthSlot = Canvas->AddChildToCanvas(HealthPanel);
	HealthSlot->SetAnchors(FAnchors(0.f, 1.f));
	HealthSlot->SetAlignment(FVector2D(0.f, 1.f));
	HealthSlot->SetPosition(FVector2D(24.f, -24.f));
	HealthSlot->SetAutoSize(true);

	// 오른쪽 위: 점수
	UBorder* ScorePanel = MakeBorder(WidgetTree, FLinearColor(0.f, 0.f, 0.f, 0.55f), FMargin(16.f, 8.f));
	ScoreText = MakeText(WidgetTree, TEXT(""), 30.f, FLinearColor(1.f, 0.85f, 0.2f), true);
	ScorePanel->SetContent(ScoreText);
	UCanvasPanelSlot* ScoreSlot = Canvas->AddChildToCanvas(ScorePanel);
	ScoreSlot->SetAnchors(FAnchors(1.f, 0.f));
	ScoreSlot->SetAlignment(FVector2D(1.f, 0.f));
	ScoreSlot->SetPosition(FVector2D(-24.f, 24.f));
	ScoreSlot->SetAutoSize(true);

	// 가운데 아래: 이 위젯이 무엇인지 (BindWidgetOptional)
	HintText = MakeText(WidgetTree, TEXT("UDemo0601_CodeBuiltHud  (부모: UDemo0601_HudBase · BindWidget 3개)"), 13.f, FLinearColor(0.7f, 0.8f, 1.f));
	UCanvasPanelSlot* HintSlot = Canvas->AddChildToCanvas(HintText);
	HintSlot->SetAnchors(FAnchors(0.5f, 1.f));
	HintSlot->SetAlignment(FVector2D(0.5f, 1.f));
	HintSlot->SetPosition(FVector2D(0.f, -12.f));
	HintSlot->SetAutoSize(true);
}

// ---------------------------------------------------------------- 데모 액터

ADemo0601_WidgetBlueprint::ADemo0601_WidgetBlueprint()
{
	DemoId = TEXT("0601");
	Controls = TEXT("1 : 점수 +100   2 : 피해 -15   H : HUD 끄기/켜기 (RemoveFromParent ↔ AddToViewport)\n")
		TEXT("관찰: 왼쪽 아래 체력바와 오른쪽 위 점수가 C++ 포인터(BindWidget)로 바로 갱신된다");
	Showcase = TEXT("체력바·점수 HUD 를 CreateWidget + AddToViewport, BindWidget 베이스 클래스 패턴");
	Implementation = EGlossaryImplementation::Alternative;
	AlternativeNote = TEXT("WBP 는 코드로 못 만들어 같은 트리를 C++ 로 조립. 실제로는 WBP_HUD 의 Parent Class 를 UDemo0601_HudBase 로 두고 "
		"HealthBar·HealthText·ScoreText 이름의 위젯을 그리면 UPROPERTY(meta=(BindWidget)) 로 자동 연결된다");
	HudClass = UDemo0601_CodeBuiltHud::StaticClass();
}

void ADemo0601_WidgetBlueprint::BeginPlay()
{
	Super::BeginPlay();
	SpawnLabel(GetWorld(), TEXT("HUD = 화면에 붙은 UI (월드와 무관하게 항상 같은 자리)"), FVector(300.f, 0.f, 250.f), Colors::Item, 20.f);
	SpawnShape(GetWorld(), EShape::Cube, FVector(400.f, -250.f, 50.f), FVector(100.f), Colors::Enemy);
	SpawnShape(GetWorld(), EShape::Sphere, FVector(400.f, 250.f, 50.f), FVector(100.f), Colors::Item);

	// 핵심 두 줄: CreateWidget 으로 인스턴스를 만들고 AddToViewport 로 화면에 붙인다
	Hud = CreateWidget<UDemo0601_HudBase>(GetPC(), HudClass);
	Hud->AddToViewport(0);

	Health = MaxHealth;
	RefreshHud();
}

void ADemo0601_WidgetBlueprint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Hud)
	{
		Hud->RemoveFromParent();
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0601_WidgetBlueprint::RefreshHud()
{
	if (Hud)
	{
		Hud->SetHealth(Health, MaxHealth);
		Hud->SetScore(Score);
	}
}

void ADemo0601_WidgetBlueprint::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UE_LOG(LogTemp, Warning, TEXT("DBG0601 t=%.2f dt=%.3f one=%d two=%d h=%d"), GetWorld()->GetTimeSeconds(), DeltaSeconds, WasKeyPressed(EKeys::One), WasKeyPressed(EKeys::Two), WasKeyPressed(EKeys::H));
	if (WasKeyPressed(EKeys::One))
	{
		Score += 100;
		RefreshHud();
	}
	if (WasKeyPressed(EKeys::Two))
	{
		Health = Health <= 0.f ? MaxHealth : FMath::Max(0.f, Health - 15.f);
		RefreshHud();
	}
	if (WasKeyPressed(EKeys::H) && Hud)
	{
		if (Hud->IsInViewport())
		{
			Hud->RemoveFromParent();
		}
		else
		{
			Hud->AddToViewport(0);
		}
	}
	SetStatus(FString::Printf(TEXT("CreateWidget<%s> · IsInViewport %s\nHP %.0f/%.0f · Score %d"),
		Hud ? *Hud->GetClass()->GetName() : TEXT("-"), Hud && Hud->IsInViewport() ? TEXT("true") : TEXT("false"),
		Health, MaxHealth, Score));
}
