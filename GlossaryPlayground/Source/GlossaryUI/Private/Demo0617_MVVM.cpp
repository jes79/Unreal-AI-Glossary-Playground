#include "Demo0617_MVVM.h"

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
using FFieldId = UE::FieldNotification::FFieldId;

// ---------------------------------------------------------------- 뷰모델

void UDemo0617_HealthViewModel::SetHealth(float NewHealth)
{
	NewHealth = FMath::Clamp(NewHealth, 0.f, MaxHealth);
	// 값이 같으면 false 를 돌려주고 알림도 보내지 않는다
	if (UE_MVVM_SET_PROPERTY_VALUE(Health, NewHealth))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthPercent); // 파생 값도 바뀌었음을 알림
	}
}

void UDemo0617_HealthViewModel::SetMaxHealth(float NewMax)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(MaxHealth, NewMax))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthPercent);
		SetHealth(Health); // 최대치가 줄면 현재 체력도 잘라낸다
	}
}

void UDemo0617_HealthViewModel::BroadcastFieldValueChanged(FFieldId InFieldId)
{
	++BroadcastCount;
	Super::BroadcastFieldValueChanged(InFieldId);
}

// ---------------------------------------------------------------- View 1: 체력바

void UDemo0617_HealthBarView::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UBorder* Panel = MakeBorder(FLinearColor(0.f, 0.f, 0.f, 0.6f), FMargin(16.f, 12.f));
	AddToCanvas(Canvas, Panel, FAnchors(0.5f, 1.f), FVector2D(0.f, -60.f), FVector2D::ZeroVector, FVector2D(0.5f, 1.f));
	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Col);
	Col->AddChildToVerticalBox(MakeText(TEXT("View 1 · 체력바  (Health · MaxHealth · GetHealthPercent 구독)"), 14.f, FLinearColor(0.6f, 0.75f, 1.f)));
	Label = MakeText(TEXT(""), 22.f, FLinearColor::White, true);
	Col->AddChildToVerticalBox(Label)->SetPadding(FMargin(0.f, 4.f));
	Bar = GlossaryUIKit::MakeBar(WidgetTree, FLinearColor(0.15f, 0.8f, 0.3f), FLinearColor(0.08f, 0.08f, 0.08f, 1.f));
	Col->AddChildToVerticalBox(GlossaryUIKit::WrapSize(WidgetTree, Bar, 700.f, 32.f));
	Counter = MakeText(TEXT(""), 13.f, FLinearColor(1.f, 0.85f, 0.35f));
	Col->AddChildToVerticalBox(Counter)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
}

void UDemo0617_HealthBarView::SetViewModel(UDemo0617_HealthViewModel* InViewModel)
{
	ViewModel = InViewModel;
	// 핵심: 필드별로 '값이 바뀌면 불러 줘' 를 등록. WBP 의 View Bindings 가 내부에서 하는 일과 같다
	const auto Delegate = INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UDemo0617_HealthBarView::OnFieldChanged);
	ViewModel->AddFieldValueChangedDelegate(UDemo0617_HealthViewModel::FFieldNotificationClassDescriptor::Health, Delegate);
	ViewModel->AddFieldValueChangedDelegate(UDemo0617_HealthViewModel::FFieldNotificationClassDescriptor::MaxHealth, Delegate);
	ViewModel->AddFieldValueChangedDelegate(UDemo0617_HealthViewModel::FFieldNotificationClassDescriptor::GetHealthPercent, Delegate);
	Refresh();
}

void UDemo0617_HealthBarView::NativeDestruct()
{
	if (ViewModel)
	{
		ViewModel->RemoveAllFieldValueChangedDelegates(this);
	}
	Super::NativeDestruct();
}

void UDemo0617_HealthBarView::OnFieldChanged(UObject* ViewModelObject, FFieldId FieldId)
{
	++UpdateCount;
	Refresh();
	Counter->SetText(FText::FromString(FString::Printf(TEXT("알림 %d회 · 마지막 필드: %s"), UpdateCount, *FieldId.GetName().ToString())));
}

void UDemo0617_HealthBarView::Refresh()
{
	if (!ViewModel || !Bar)
	{
		return;
	}
	const float Pct = ViewModel->GetHealthPercent();
	Bar->SetPercent(Pct);
	Bar->SetFillColorAndOpacity(FLinearColor::LerpUsingHSV(FLinearColor(0.9f, 0.1f, 0.05f), FLinearColor(0.15f, 0.8f, 0.3f), Pct));
	Label->SetText(FText::FromString(FString::Printf(TEXT("HP %.0f / %.0f"), ViewModel->GetHealth(), ViewModel->GetMaxHealth())));
}

// ---------------------------------------------------------------- View 2: 배지

void UDemo0617_StatusBadgeView::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UBorder* Panel = MakeBorder(FLinearColor(0.f, 0.f, 0.f, 0.6f), FMargin(14.f, 10.f));
	AddToCanvas(Canvas, Panel, FAnchors(1.f, 0.f), FVector2D(-24.f, 24.f), FVector2D::ZeroVector, FVector2D(1.f, 0.f));
	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Col);
	Col->AddChildToVerticalBox(MakeText(TEXT("View 2 · 상태 배지 (GetHealthPercent 만 구독)"), 13.f, FLinearColor(0.6f, 0.75f, 1.f)));
	Badge = MakeText(TEXT(""), 26.f, FLinearColor::White, true);
	Col->AddChildToVerticalBox(Badge);
}

void UDemo0617_StatusBadgeView::SetViewModel(UDemo0617_HealthViewModel* InViewModel)
{
	ViewModel = InViewModel;
	ViewModel->AddFieldValueChangedDelegate(UDemo0617_HealthViewModel::FFieldNotificationClassDescriptor::GetHealthPercent,
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UDemo0617_StatusBadgeView::OnPercentChanged));
	OnPercentChanged(ViewModel, UDemo0617_HealthViewModel::FFieldNotificationClassDescriptor::GetHealthPercent);
	UpdateCount = 0;
}

void UDemo0617_StatusBadgeView::NativeDestruct()
{
	if (ViewModel)
	{
		ViewModel->RemoveAllFieldValueChangedDelegates(this);
	}
	Super::NativeDestruct();
}

void UDemo0617_StatusBadgeView::OnPercentChanged(UObject* ViewModelObject, FFieldId FieldId)
{
	++UpdateCount;
	if (!Badge)
	{
		return;
	}
	const float Pct = ViewModel->GetHealthPercent();
	const TCHAR* State = Pct > 0.6f ? TEXT("건강") : (Pct > 0.3f ? TEXT("부상") : TEXT("위험!"));
	Badge->SetText(FText::FromString(FString::Printf(TEXT("%s  %.0f%%"), State, Pct * 100.f)));
	Badge->SetColorAndOpacity(FSlateColor(Pct > 0.6f ? FLinearColor(0.3f, 1.f, 0.4f) : (Pct > 0.3f ? FLinearColor(1.f, 0.8f, 0.2f) : FLinearColor(1.f, 0.2f, 0.15f))));
}

// ---------------------------------------------------------------- 데모

ADemo0617_MVVM::ADemo0617_MVVM()
{
	DemoId = TEXT("0617");
	Controls = TEXT("Space : 피해 -12   H : 회복 +20   M : MaxHealth 100 ↔ 150   R : 같은 값 다시 넣기 (알림 없음)\n")
		TEXT("관찰: 게임 코드는 뷰모델 값만 바꾸고, 체력바와 배지 두 View 가 FieldNotify 알림으로 스스로 갱신된다");
	Showcase = TEXT("Health·MaxHealth FieldNotify 뷰모델 하나를 체력바·상태 배지 두 View 가 구독, 알림 횟수 표시");
	Implementation = EGlossaryImplementation::Alternative;
	AlternativeNote = TEXT("뷰모델(UMVVMViewModelBase + FieldNotify)은 실제 MVVM 플러그인 그대로. WBP 의 Viewmodels 패널·View Bindings 는 에디터 전용이라 "
		"View 쪽은 AddFieldValueChangedDelegate 로 같은 알림을 직접 구독했다");
	PlayerPawnClass = nullptr;
}

void ADemo0617_MVVM::BeginPlay()
{
	Super::BeginPlay();
	GlossaryUIKit::SetupFixedCamera(this, FVector(-650.f, 0.f, 260.f), FRotator(-12.f, 0.f, 0.f)); // 고정 카메라
	SpawnShape(GetWorld(), EShape::Cylinder, FVector(-100.f, 0.f, 90.f), FVector(100.f, 100.f, 180.f), Colors::Player);
	// Model 쪽(여기서는 데모 액터)이 뷰모델을 만들어 View 들에게 나눠 준다
	HealthViewModel = NewObject<UDemo0617_HealthViewModel>(this);
	BarView = CreateWidget<UDemo0617_HealthBarView>(GetPC(), UDemo0617_HealthBarView::StaticClass());
	BarView->AddToViewport(0);
	BarView->SetViewModel(HealthViewModel);
	BadgeView = CreateWidget<UDemo0617_StatusBadgeView>(GetPC(), UDemo0617_StatusBadgeView::StaticClass());
	BadgeView->AddToViewport(0);
	BadgeView->SetViewModel(HealthViewModel);
	HealthViewModel->BroadcastCount = 0;
}

void ADemo0617_MVVM::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BarView) { BarView->RemoveFromParent(); }
	if (BadgeView) { BadgeView->RemoveFromParent(); }
	Super::EndPlay(EndPlayReason);
}

void ADemo0617_MVVM::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UDemo0617_HealthViewModel* VM = HealthViewModel;
	if (!VM)
	{
		return;
	}
	Timer += DeltaSeconds;
	if (Timer >= AutoDamageInterval)
	{
		Timer = 0.f;
		VM->SetHealth(VM->GetHealth() <= 12.f ? VM->GetMaxHealth() : VM->GetHealth() - 12.f);
		LastAction = TEXT("자동 피해 -12");
	}
	if (WasKeyPressed(EKeys::SpaceBar)) { VM->SetHealth(VM->GetHealth() - 12.f); LastAction = TEXT("피해 -12"); }
	if (WasKeyPressed(EKeys::H))        { VM->SetHealth(VM->GetHealth() + 20.f); LastAction = TEXT("회복 +20"); }
	if (WasKeyPressed(EKeys::M))        { VM->SetMaxHealth(VM->GetMaxHealth() > 100.f ? 100.f : 150.f); LastAction = TEXT("MaxHealth 변경"); }
	if (WasKeyPressed(EKeys::R))
	{
		const int32 Before = VM->BroadcastCount;
		VM->SetHealth(VM->GetHealth()); // 같은 값 → UE_MVVM_SET_PROPERTY_VALUE 가 false, 알림 없음
		LastAction = FString::Printf(TEXT("같은 값 넣기 → 알림 %d개"), VM->BroadcastCount - Before);
	}
	SetStatus(FString::Printf(TEXT("ViewModel Health %.0f / MaxHealth %.0f · 보낸 알림 %d개\nView1 갱신 %d회 · View2 갱신 %d회 · 마지막: %s"),
		VM->GetHealth(), VM->GetMaxHealth(), VM->BroadcastCount,
		BarView ? BarView->UpdateCount : 0, BadgeView ? BadgeView->UpdateCount : 0, *LastAction));
}
