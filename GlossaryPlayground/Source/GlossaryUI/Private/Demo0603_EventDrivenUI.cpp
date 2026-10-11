#include "Demo0603_EventDrivenUI.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GlossaryKit.h"
#include "GlossaryUIKit.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- HUD

void UDemo0603_CompareHud::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	// 화면 아래 가운데 (정보 패널과 겹치지 않게)
	AddToCanvas(Canvas, Row, FAnchors(0.5f, 1.f), FVector2D(0.f, -40.f), FVector2D::ZeroVector, FVector2D(0.5f, 1.f));

	auto MakeColumn = [this, Row](const FString& Title, const FString& Sub, const FLinearColor& Tint,
		TObjectPtr<UProgressBar>& OutBar, TObjectPtr<UTextBlock>& OutText, TObjectPtr<UTextBlock>& OutCount)
	{
		UBorder* Panel = MakeBorder(Tint, FMargin(16.f, 12.f));
		UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Panel->SetContent(Col);
		Col->AddChildToVerticalBox(MakeText(Title, 20.f, FLinearColor::White, true));
		Col->AddChildToVerticalBox(MakeText(Sub, 13.f, FLinearColor(0.8f, 0.85f, 0.9f)));
		OutText = MakeText(TEXT(""), 18.f, FLinearColor::White, true);
		Col->AddChildToVerticalBox(OutText)->SetPadding(FMargin(0.f, 10.f, 0.f, 2.f));
		OutBar = GlossaryUIKit::MakeBar(WidgetTree, FLinearColor(0.15f, 0.8f, 0.3f), FLinearColor(0.05f, 0.05f, 0.05f, 0.9f));
		Col->AddChildToVerticalBox(GlossaryUIKit::WrapSize(WidgetTree, OutBar, 420.f, 24.f));
		OutCount = MakeText(TEXT(""), 22.f, FLinearColor(1.f, 0.85f, 0.3f), true);
		Col->AddChildToVerticalBox(OutCount)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
		Row->AddChildToHorizontalBox(Panel)->SetPadding(FMargin(12.f, 0.f));
	};

	MakeColumn(TEXT("Property Binding (Bind 함수)"), TEXT("TextDelegate / PercentDelegate — 그릴 때마다 호출"),
		FLinearColor(0.35f, 0.08f, 0.08f, 0.85f), BoundBar, BoundText, LeftCount);
	MakeColumn(TEXT("이벤트 갱신 (Delegate)"), TEXT("OnHealthChanged 때만 SetPercent / SetText"),
		FLinearColor(0.05f, 0.25f, 0.12f, 0.85f), EventBar, EventText, RightCount);

	// 핵심: UMG 'Bind' 와 똑같은 일. 델리게이트를 묶으면 SynchronizeProperties 가 '매 프레임 평가되는 속성' 으로 만든다
	BoundText->TextDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UDemo0603_CompareHud, GetBoundHealthText));
	BoundBar->PercentDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UDemo0603_CompareHud, GetBoundHealthPercent));
}

void UDemo0603_CompareHud::Init(ADemo0603_EventDrivenUI* InSource)
{
	Source = InSource;
	// Delegate 방식: 구독만 해 두면 값이 바뀔 때 알려 준다
	HealthChangedHandle = InSource->OnHealthChanged.AddUObject(this, &UDemo0603_CompareHud::HandleHealthChanged);
	HandleHealthChanged(InSource->GetHealth(), InSource->GetMaxHealth()); // 초기값 한 번
}

void UDemo0603_CompareHud::NativeDestruct()
{
	if (Source.IsValid())
	{
		Source->OnHealthChanged.Remove(HealthChangedHandle);
	}
	Super::NativeDestruct();
}

FText UDemo0603_CompareHud::GetBoundHealthText()
{
	++BindingCalls;
	const float Health = Source.IsValid() ? Source->GetHealth() : 0.f;
	return FText::FromString(FString::Printf(TEXT("HP %.0f"), Health));
}

float UDemo0603_CompareHud::GetBoundHealthPercent()
{
	++BindingCalls;
	return Source.IsValid() ? Source->GetHealth() / Source->GetMaxHealth() : 0.f;
}

void UDemo0603_CompareHud::HandleHealthChanged(float Health, float MaxHealth)
{
	++DelegateUpdates;
	EventBar->SetPercent(MaxHealth > 0.f ? Health / MaxHealth : 0.f);
	EventText->SetText(FText::FromString(FString::Printf(TEXT("HP %.0f"), Health)));
}

// ---------------------------------------------------------------- 데모

ADemo0603_EventDrivenUI::ADemo0603_EventDrivenUI()
{
	DemoId = TEXT("0603");
	Controls = TEXT("Space : 피해 -10   H : 회복 (그 외에는 2초마다 자동 피해)\n")
		TEXT("관찰: 두 체력바는 똑같이 보이지만 왼쪽 Bind 함수는 그릴 때마다(매 프레임) 불리고, 오른쪽 Delegate 는 체력이 바뀐 횟수만큼만 불린다");
	Showcase = TEXT("같은 체력바를 Property Binding(매 프레임) vs Delegate(변경 시) 로 그리고 호출 횟수 비교");
	PlayerPawnClass = nullptr;
}

void ADemo0603_EventDrivenUI::BeginPlay()
{
	Super::BeginPlay();
	GlossaryUIKit::SetupFixedCamera(this, FVector(-650.f, 0.f, 260.f), FRotator(-12.f, 0.f, 0.f)); // 고정 카메라
	SpawnShape(GetWorld(), EShape::Cube, FVector(-100.f, 0.f, 60.f), FVector(120.f), Colors::Enemy);
	Health = MaxHealth;
	Hud = CreateWidget<UDemo0603_CompareHud>(GetPC(), UDemo0603_CompareHud::StaticClass());
	Hud->AddToViewport(0);
	Hud->Init(this);
}

void ADemo0603_EventDrivenUI::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Hud)
	{
		Hud->RemoveFromParent();
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0603_EventDrivenUI::SetHealth(float NewHealth)
{
	NewHealth = FMath::Clamp(NewHealth, 0.f, MaxHealth);
	if (FMath::IsNearlyEqual(NewHealth, Health))
	{
		return; // 바뀌지 않았으면 방송하지 않는다
	}
	Health = NewHealth;
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void ADemo0603_EventDrivenUI::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;

	DamageTimer += DeltaSeconds;
	if (DamageTimer >= AutoDamageInterval)
	{
		DamageTimer = 0.f;
		SetHealth(Health <= 10.f ? MaxHealth : Health - 10.f);
	}
	if (WasKeyPressed(EKeys::SpaceBar)) { SetHealth(Health - 10.f); }
	if (WasKeyPressed(EKeys::H))        { SetHealth(MaxHealth); }

	if (Hud && Hud->LeftCount)
	{
		const float Secs = FMath::Max(Elapsed, 0.01f);
		Hud->LeftCount->SetText(FText::FromString(FString::Printf(TEXT("Bind 호출 %lld회 (초당 %.0f)"), Hud->BindingCalls, Hud->BindingCalls / Secs)));
		Hud->RightCount->SetText(FText::FromString(FString::Printf(TEXT("Delegate 갱신 %lld회"), Hud->DelegateUpdates)));
		SetStatus(FString::Printf(TEXT("%.0f초 동안 · Bind 함수 %lld회 vs Delegate 갱신 %lld회\n같은 결과를 그리는 데 %.0f배 더 많은 호출"),
			Elapsed, Hud->BindingCalls, Hud->DelegateUpdates,
			Hud->DelegateUpdates > 0 ? double(Hud->BindingCalls) / double(Hud->DelegateUpdates) : 0.0));
	}
}
