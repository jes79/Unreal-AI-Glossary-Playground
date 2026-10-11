#include "Demo0616_CommonUI.h"

#include "Blueprint/WidgetTree.h"
#include "CommonInputSettings.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "GlossaryUIKit.h"
#include "ICommonInputModule.h"
#include "Input/UIActionBindingHandle.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- 메뉴 베이스

TSharedRef<SWidget> UDemo0616_MenuBase::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UBorder* Panel = GlossaryUIKit::MakeBorder(WidgetTree, FLinearColor(0.04f, 0.05f, 0.09f, 0.95f), FMargin(28.f, 22.f));
		WidgetTree->RootWidget = Panel;
		UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Panel->SetContent(Col);
		Col->AddChildToVerticalBox(GlossaryUIKit::MakeText(WidgetTree, Title, 30.f, FLinearColor(1.f, 0.85f, 0.35f), true))
			->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));
		BuildButtons(Col);
		Col->AddChildToVerticalBox(GlossaryUIKit::MakeText(WidgetTree,
			bIsRootScreen ? TEXT("Esc / 패드 B : 루트 화면이라 무시") : TEXT("Esc / 패드 B : 뒤로 (이 화면 비활성화 → 스택에서 제거)"),
			13.f, FLinearColor(0.6f, 0.75f, 1.f)))->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	}
	return Super::RebuildWidget();
}

void UDemo0616_MenuBase::NativeConstruct()
{
	// 프로젝트 설정(Common Input Settings)에 기본 Back 액션이 있으면 Common UI 의 진짜 Back 바인딩을 쓴다.
	// 없으면(이 프로젝트 기본값) 아래 NativeOnKeyDown 에서 Esc / 패드 B 를 직접 HandleBackAction 으로 보낸다.
	bIsBackHandler = !ICommonInputModule::GetSettings().GetDefaultBackAction().IsNull();
	Super::NativeConstruct();
}

UButton* UDemo0616_MenuBase::AddButton(UVerticalBox* Column, const FString& Label)
{
	UButton* Button = GlossaryUIKit::MakeButton(WidgetTree, Label, 20.f, FLinearColor(0.12f, 0.2f, 0.4f, 1.f));
	Column->AddChildToVerticalBox(GlossaryUIKit::WrapSize(WidgetTree, Button, 380.f, 56.f))->SetPadding(FMargin(0.f, 5.f));
	Buttons.Add(Button);
	return Button;
}

UWidget* UDemo0616_MenuBase::NativeGetDesiredFocusTarget() const
{
	// 활성화될 때 포커스를 줄 위젯 → 게임패드로 바로 위/아래 이동 가능
	return Buttons.Num() > 0 ? Buttons[0].Get() : nullptr;
}

TOptional<FUIInputConfig> UDemo0616_MenuBase::GetDesiredInputConfig() const
{
	// 메뉴 모드: 게임 입력은 막고, 마우스는 잡지 않는다 (커서로도 조작 가능)
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

bool UDemo0616_MenuBase::NativeOnHandleBackAction()
{
	if (bIsRootScreen)
	{
		return true; // 처리했다고만 하고 아무것도 안 함 (메인 메뉴는 닫히지 않음)
	}
	return Super::NativeOnHandleBackAction(); // 기본 동작 = DeactivateWidget → 스택이 이 화면을 빼고 아래 화면을 다시 활성화
}

FReply UDemo0616_MenuBase::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (!bIsBackHandler && (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::BackSpace))
	{
		HandleBackAction();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// ---------------------------------------------------------------- 메인

UDemo0616_MainMenu::UDemo0616_MainMenu()
{
	Title = TEXT("메인 메뉴");
	bIsRootScreen = true;
}

void UDemo0616_MainMenu::BuildButtons(UVerticalBox* Column)
{
	AddButton(Column, TEXT("게임 시작"))->OnClicked.AddDynamic(this, &UDemo0616_MainMenu::StartGame);
	AddButton(Column, TEXT("옵션  ›"))->OnClicked.AddDynamic(this, &UDemo0616_MainMenu::OpenOptions);
}

void UDemo0616_MainMenu::StartGame()
{
	// 이 데모에서는 동작 없음 (스택 이동만 보여 준다)
}

void UDemo0616_MainMenu::OpenOptions()
{
	if (UCommonActivatableWidgetStack* S = Stack.Get())
	{
		// 핵심: 스택에 클래스로 push. 이전 화면은 비활성화(입력·포커스 해제)되고 새 화면이 활성화된다
		S->AddWidget<UDemo0616_OptionsMenu>(UDemo0616_OptionsMenu::StaticClass(),
			[S](UDemo0616_OptionsMenu& W) { W.Stack = S; });
	}
}

// ---------------------------------------------------------------- 옵션

UDemo0616_OptionsMenu::UDemo0616_OptionsMenu()
{
	Title = TEXT("옵션");
}

void UDemo0616_OptionsMenu::BuildButtons(UVerticalBox* Column)
{
	AddButton(Column, TEXT("그래픽  ›"))->OnClicked.AddDynamic(this, &UDemo0616_OptionsMenu::OpenGraphics);
	AddButton(Column, TEXT("‹ 뒤로"))->OnClicked.AddDynamic(this, &UDemo0616_OptionsMenu::GoBack);
}

void UDemo0616_OptionsMenu::OpenGraphics()
{
	if (UCommonActivatableWidgetStack* S = Stack.Get())
	{
		S->AddWidget<UDemo0616_GraphicsMenu>(UDemo0616_GraphicsMenu::StaticClass(),
			[S](UDemo0616_GraphicsMenu& W) { W.Stack = S; });
	}
}

void UDemo0616_OptionsMenu::GoBack()
{
	HandleBackAction();
}

// ---------------------------------------------------------------- 그래픽

UDemo0616_GraphicsMenu::UDemo0616_GraphicsMenu()
{
	Title = TEXT("그래픽");
}

void UDemo0616_GraphicsMenu::BuildButtons(UVerticalBox* Column)
{
	UButton* QualityButton = AddButton(Column, TEXT(""));
	QualityButton->OnClicked.AddDynamic(this, &UDemo0616_GraphicsMenu::CycleQuality);
	QualityText = Cast<UTextBlock>(QualityButton->GetChildAt(0));
	AddButton(Column, TEXT("‹ 뒤로"))->OnClicked.AddDynamic(this, &UDemo0616_GraphicsMenu::GoBack);
	Quality = (Quality + 3) % 4; // CycleQuality 가 한 칸 올리므로 미리 한 칸 내려 둔다
	CycleQuality();
}

void UDemo0616_GraphicsMenu::CycleQuality()
{
	static const TCHAR* Names[] = { TEXT("낮음"), TEXT("중간"), TEXT("높음"), TEXT("에픽") };
	Quality = (Quality + 1) % 4;
	if (QualityText)
	{
		QualityText->SetText(FText::FromString(FString::Printf(TEXT("품질: %s  (눌러서 변경)"), Names[Quality])));
	}
}

void UDemo0616_GraphicsMenu::GoBack()
{
	HandleBackAction();
}

// ---------------------------------------------------------------- 레이아웃

void UDemo0616_RootLayout::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	AddToCanvas(Canvas, Col, FAnchors(0.5f, 0.5f), FVector2D(260.f, 40.f), FVector2D(560.f, 520.f), FVector2D(0.5f, 0.5f));
	Breadcrumb = MakeText(TEXT(""), 18.f, FLinearColor(0.75f, 0.9f, 1.f), true);
	Col->AddChildToVerticalBox(Breadcrumb)->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	// 핵심 위젯: Activatable Widget Stack. 맨 위 화면 하나만 보이고 입력을 받는다
	MenuStack = WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>(UCommonActivatableWidgetStack::StaticClass());
	UVerticalBoxSlot* StackSlot = Col->AddChildToVerticalBox(MenuStack);
	StackSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
}

// ---------------------------------------------------------------- 데모

ADemo0616_CommonUI::ADemo0616_CommonUI()
{
	DemoId = TEXT("0616");
	Controls = TEXT("마우스 클릭 / 패드 A : 버튼   Esc · 패드 B : 뒤로 (스택 pop)\n")
		TEXT("Enter : 맨 위 화면의 첫 버튼   G : 그래픽까지 한 번에   Backspace : 뒤로\n")
		TEXT("관찰: 메인 → 옵션 → 그래픽으로 쌓이고, 뒤로 가면 아래 화면이 다시 활성화되어 포커스를 돌려받는다");
	Showcase = TEXT("UCommonActivatableWidgetStack 으로 메인→옵션→그래픽 메뉴 스택, Esc/패드 B 로 pop 과 포커스 복원");
	AlternativeNote = TEXT("버튼은 스타일 에셋이 필요한 UCommonButtonBase 대신 UButton 사용. 프로젝트 설정에 Common UI Input Data(Back 액션)가 없어서 "
		"Esc/패드 B 를 위젯에서 HandleBackAction 으로 직접 보낸다 (설정되면 Common UI 의 Back 바인딩이 자동으로 쓰인다)");
	Implementation = EGlossaryImplementation::Alternative;
	PlayerPawnClass = nullptr;
	bShowMouseCursor = true;
}

void ADemo0616_CommonUI::BeginPlay()
{
	Super::BeginPlay();
	SpawnShape(GetWorld(), EShape::Cube, FVector(-100.f, -250.f, 60.f), FVector(120.f), Colors::Cyan);
	Layout = CreateWidget<UDemo0616_RootLayout>(GetPC(), UDemo0616_RootLayout::StaticClass());
	Layout->AddToViewport(0);
	if (UCommonActivatableWidgetStack* S = Layout->MenuStack)
	{
		S->AddWidget<UDemo0616_MainMenu>(UDemo0616_MainMenu::StaticClass(), [S](UDemo0616_MainMenu& W) { W.Stack = S; });
	}
}

void ADemo0616_CommonUI::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Layout)
	{
		Layout->RemoveFromParent();
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0616_CommonUI::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UCommonActivatableWidgetStack* S = Layout ? Layout->MenuStack.Get() : nullptr;
	if (!S)
	{
		return;
	}
	UDemo0616_MenuBase* Top = Cast<UDemo0616_MenuBase>(S->GetActiveWidget());
	// 아래 키들은 PlayerController 에 들어온 입력 (시연·자동 캡처용). 실제 조작은 위젯 포커스·Common UI 가 처리한다
	if (Top && WasKeyPressed(EKeys::Enter)) { Top->ActivateFirst(); }
	if (Top && (WasKeyPressed(EKeys::BackSpace) || WasKeyPressed(EKeys::Escape))) { Top->RequestBack(); }
	if (Top && WasKeyPressed(EKeys::G))
	{
		while (UDemo0616_MenuBase* Cur = Cast<UDemo0616_MenuBase>(S->GetActiveWidget()))
		{
			if (Cur->IsA<UDemo0616_GraphicsMenu>()) { break; }
			Cur->ActivateFirst();
			if (S->GetActiveWidget() == Cur) { break; }
		}
	}

	FString Path;
	for (UCommonActivatableWidget* W : S->GetWidgetList())
	{
		if (const UDemo0616_MenuBase* M = Cast<UDemo0616_MenuBase>(W))
		{
			Path += Path.IsEmpty() ? M->Title : TEXT("  ›  ") + M->Title;
		}
	}
	Layout->Breadcrumb->SetText(FText::FromString(FString::Printf(TEXT("스택: %s"), *Path)));
	Top = Cast<UDemo0616_MenuBase>(S->GetActiveWidget());
	SetStatus(FString::Printf(TEXT("스택 깊이 %d · 활성 화면: %s (IsActivated %s)\nBack 처리: %s"),
		S->GetNumWidgets(), Top ? *Top->Title : TEXT("-"), Top && Top->IsActivated() ? TEXT("true") : TEXT("false"),
		ICommonInputModule::GetSettings().GetDefaultBackAction().IsNull() ? TEXT("위젯 NativeOnKeyDown → HandleBackAction") : TEXT("Common UI Back 액션 바인딩")));
}
