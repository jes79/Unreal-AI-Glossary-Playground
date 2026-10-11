#include "Demo0611_Tooltip.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "GlossaryUIKit.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- 툴팁 위젯

FString UDemo0611_TooltipWidget::RarityName(int32 Rarity)
{
	static const TCHAR* Names[] = { TEXT("일반"), TEXT("희귀"), TEXT("영웅"), TEXT("전설") };
	return Names[FMath::Clamp(Rarity, 0, 3)];
}

FLinearColor UDemo0611_TooltipWidget::RarityColor(int32 Rarity)
{
	static const FLinearColor Colors[] = { FLinearColor(0.85f, 0.85f, 0.85f), FLinearColor(0.3f, 0.6f, 1.f),
		FLinearColor(0.75f, 0.35f, 1.f), FLinearColor(1.f, 0.65f, 0.1f) };
	return Colors[FMath::Clamp(Rarity, 0, 3)];
}

void UDemo0611_TooltipWidget::BuildTree()
{
	// 등급 색 테두리 → 어두운 본문
	UBorder* Outline = MakeBorder(RarityColor(Info.Rarity), FMargin(2.f));
	WidgetTree->RootWidget = Outline;
	UBorder* Body = MakeBorder(FLinearColor(0.03f, 0.03f, 0.05f, 0.97f), FMargin(14.f, 10.f));
	Outline->SetContent(Body);
	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Body->SetContent(Col);

	Col->AddChildToVerticalBox(MakeText(Info.Name, 20.f, RarityColor(Info.Rarity), true));
	Col->AddChildToVerticalBox(MakeText(FString::Printf(TEXT("[%s]"), *RarityName(Info.Rarity)), 13.f, RarityColor(Info.Rarity)))
		->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	UTextBlock* Desc = MakeText(Info.Description, 14.f, FLinearColor(0.85f, 0.85f, 0.85f));
	Desc->SetAutoWrapText(true);
	Desc->SetWrapTextAt(300.f);
	Col->AddChildToVerticalBox(Desc);
	Col->AddChildToVerticalBox(MakeText(Info.Stats, 14.f, FLinearColor(0.5f, 1.f, 0.6f)))->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
}

// ---------------------------------------------------------------- 슬롯

void UDemo0611_Slot::BuildTree()
{
	Frame = MakeBorder(FLinearColor(0.1f, 0.11f, 0.14f, 0.95f), FMargin(8.f));
	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Frame->SetContent(Col);
	UBorder* Icon = MakeBorder(Info.IconColor, FMargin(0.f));
	Col->AddChildToVerticalBox(GlossaryUIKit::WrapSize(WidgetTree, Icon, 0.f, 60.f));
	UTextBlock* Name = MakeText(Info.Name, 13.f, UDemo0611_TooltipWidget::RarityColor(Info.Rarity), true);
	Name->SetJustification(ETextJustify::Center);
	Col->AddChildToVerticalBox(Name)->SetHorizontalAlignment(HAlign_Center);
	WidgetTree->RootWidget = GlossaryUIKit::WrapSize(WidgetTree, Frame, 120.f, 110.f);
	CreateTooltip(); // Info 가 채워진 뒤(트리를 만들 때) 툴팁을 만든다
}

void UDemo0611_Slot::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UDemo0611_Slot::CreateTooltip()
{
	// 핵심: 툴팁 위젯을 만들어 SetToolTip 으로 지정. 마우스를 올리면 Slate 가 커서 옆에 띄운다
	// (WBP 라면 디테일 패널 Behavior > Tool Tip Widget 을 Bind 해 같은 일을 한다)
	UDemo0611_TooltipWidget* Tip = CreateWidget<UDemo0611_TooltipWidget>(GetOwningPlayer(), UDemo0611_TooltipWidget::StaticClass());
	Tip->Info = Info;
	SetToolTip(Tip);
}

void UDemo0611_Slot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	bHovered = true;
	if (Frame) { Frame->SetBrushColor(FLinearColor(0.2f, 0.25f, 0.35f, 0.95f)); }
}

void UDemo0611_Slot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	bHovered = false;
	if (Frame) { Frame->SetBrushColor(FLinearColor(0.1f, 0.11f, 0.14f, 0.95f)); }
}

// ---------------------------------------------------------------- 패널

void UDemo0611_Panel::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UBorder* Panel = MakeBorder(FLinearColor(0.02f, 0.03f, 0.05f, 0.9f), FMargin(16.f));
	AddToCanvas(Canvas, Panel, FAnchors(0.5f, 1.f), FVector2D(200.f, -60.f), FVector2D::ZeroVector, FVector2D(0.5f, 1.f));
	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Col);
	Col->AddChildToVerticalBox(MakeText(TEXT("슬롯에 마우스를 올려 보세요"), 18.f, FLinearColor(1.f, 0.85f, 0.35f), true))
		->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	Grid->SetSlotPadding(FMargin(4.f));
	Col->AddChildToVerticalBox(Grid);
	for (int32 i = 0; i < ItemInfos.Num(); ++i)
	{
		UDemo0611_Slot* SlotWidget = CreateWidget<UDemo0611_Slot>(this, UDemo0611_Slot::StaticClass());
		SlotWidget->Info = ItemInfos[i];
		Grid->AddChildToUniformGrid(SlotWidget, i / 4, i % 4);
		Slots.Add(SlotWidget);
	}
}

// ---------------------------------------------------------------- 데모

ADemo0611_Tooltip::ADemo0611_Tooltip()
{
	DemoId = TEXT("0611");
	Controls = TEXT("슬롯에 마우스를 올리면 커스텀 툴팁 (이름·등급 색·설명·능력치)\n")
		TEXT("1~8 : 마우스 커서를 해당 슬롯으로 옮겨 보기\n")
		TEXT("관찰: 툴팁 테두리와 이름 색이 등급(일반·희귀·영웅·전설)에 따라 달라진다");
	Showcase = TEXT("인벤토리 슬롯 호버 시 SetToolTip 으로 지정한 커스텀 툴팁(이름·등급·설명·능력치)");
	PlayerPawnClass = nullptr;
	bShowMouseCursor = true;
}

void ADemo0611_Tooltip::BeginPlay()
{
	Super::BeginPlay();
	GlossaryUIKit::SetupFixedCamera(this, FVector(-650.f, 0.f, 260.f), FRotator(-12.f, 0.f, 0.f)); // 고정 카메라
	Panel = CreateWidget<UDemo0611_Panel>(GetPC(), UDemo0611_Panel::StaticClass());
	auto Add = [this](const TCHAR* Name, int32 Rarity, const TCHAR* Desc, const TCHAR* Stats, const FLinearColor& Color)
	{
		FDemo0611ItemInfo& Info = Panel->ItemInfos.AddDefaulted_GetRef();
		Info.Name = Name; Info.Rarity = Rarity; Info.Description = Desc; Info.Stats = Stats; Info.IconColor = Color;
	};
	Add(TEXT("녹슨 단검"), 0, TEXT("오래 방치된 단검. 그래도 아직은 벨 수 있다."), TEXT("공격력 +4"), FLinearColor(0.5f, 0.4f, 0.35f));
	Add(TEXT("기사의 방패"), 1, TEXT("왕국 기사단에 지급되는 둥근 방패."), TEXT("방어력 +12 · 이동 속도 -3%"), FLinearColor(0.3f, 0.45f, 0.9f));
	Add(TEXT("치유 물약"), 0, TEXT("마시면 체력을 즉시 회복한다."), TEXT("체력 +50 회복"), FLinearColor(0.9f, 0.15f, 0.2f));
	Add(TEXT("바람의 활"), 2, TEXT("시위를 당기면 바람이 화살을 밀어 준다."), TEXT("공격력 +28 · 사거리 +20%"), FLinearColor(0.4f, 0.8f, 0.7f));
	Add(TEXT("용의 심장"), 3, TEXT("고대 용에게서 꺼낸 보석. 아직도 따뜻하다."), TEXT("모든 능력치 +10 · 화염 저항 50%"), FLinearColor(1.f, 0.35f, 0.1f));
	Add(TEXT("은반지"), 1, TEXT("작은 룬이 새겨진 반지."), TEXT("마나 +20"), FLinearColor(0.8f, 0.8f, 0.85f));
	Add(TEXT("순간이동 두루마리"), 2, TEXT("읽으면 마지막으로 들른 마을로 돌아간다."), TEXT("시전 시간 3초"), FLinearColor(0.85f, 0.8f, 0.6f));
	Add(TEXT("빈 병"), 0, TEXT("물약을 담을 수 있다."), TEXT("-"), FLinearColor(0.6f, 0.75f, 0.8f));
	Panel->AddToViewport(0);
	SpawnShape(GetWorld(), EShape::Cube, FVector(-100.f, -250.f, 60.f), FVector(120.f), Colors::Item);
}

void ADemo0611_Tooltip::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Panel)
	{
		Panel->RemoveFromParent();
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0611_Tooltip::MoveMouseToSlot(int32 Index)
{
	if (!Panel || !Panel->Slots.IsValidIndex(Index))
	{
		return;
	}
	// 슬롯의 화면(절대) 좌표 → 뷰포트 픽셀 좌표로 바꿔 OS 커서를 옮긴다. Slate 가 커서 이동을 감지해 호버·툴팁을 처리한다
	const FGeometry& Geo = Panel->Slots[Index]->GetCachedGeometry();
	const FVector2D AbsCenter = Geo.LocalToAbsolute(Geo.GetLocalSize() * 0.5f);
	FVector2D PixelPos, ViewportPos;
	USlateBlueprintLibrary::AbsoluteToViewport(this, AbsCenter, PixelPos, ViewportPos);
	GetPC()->SetMouseLocation(FMath::RoundToInt(PixelPos.X), FMath::RoundToInt(PixelPos.Y));
	DemoSlot = Index;
}

void ADemo0611_Tooltip::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Panel)
	{
		return;
	}
	const FKey NumKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight };
	for (int32 i = 0; i < 8; ++i)
	{
		if (WasKeyPressed(NumKeys[i]))
		{
			MoveMouseToSlot(i);
		}
	}
	FString Hovered = TEXT("없음");
	for (const UDemo0611_Slot* S : Panel->Slots)
	{
		if (S->bHovered)
		{
			Hovered = FString::Printf(TEXT("%s [%s]"), *S->Info.Name, *UDemo0611_TooltipWidget::RarityName(S->Info.Rarity));
		}
	}
	SetStatus(FString::Printf(TEXT("호버 중인 슬롯: %s\n툴팁 = UDemo0611_TooltipWidget (슬롯마다 SetToolTip)"), *Hovered));
}
