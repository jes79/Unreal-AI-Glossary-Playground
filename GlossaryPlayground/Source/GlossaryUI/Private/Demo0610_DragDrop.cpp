#include "Demo0610_DragDrop.h"

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
#include "GlossaryKit.h"
#include "GlossaryUIKit.h"
#include "Input/Reply.h"

using namespace GlossaryKit;

static const FLinearColor SlotIdle(0.1f, 0.11f, 0.14f, 0.95f);
static const FLinearColor SlotHover(0.25f, 0.45f, 0.25f, 0.95f);

// ---------------------------------------------------------------- 슬롯

void UDemo0610_Slot::BuildTree()
{
	Frame = MakeBorder(SlotIdle, FMargin(6.f));
	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Frame->SetContent(Col);
	Icon = MakeBorder(FLinearColor::Transparent, FMargin(0.f));
	UVerticalBoxSlot* IconSlot = Col->AddChildToVerticalBox(GlossaryUIKit::WrapSize(WidgetTree, Icon, 0.f, 58.f));
	IconSlot->SetHorizontalAlignment(HAlign_Fill);
	NameText = MakeText(TEXT(""), 13.f, FLinearColor::White, true);
	NameText->SetJustification(ETextJustify::Center);
	Col->AddChildToVerticalBox(NameText)->SetHorizontalAlignment(HAlign_Center);
	WidgetTree->RootWidget = GlossaryUIKit::WrapSize(WidgetTree, Frame, 120.f, 110.f);
	ShowItem(Current);
}

void UDemo0610_Slot::ShowItem(const FDemo0610Item& Item)
{
	Current = Item;
	if (!Icon)
	{
		return;
	}
	Icon->SetBrushColor(Item.IsEmpty() ? FLinearColor(0.f, 0.f, 0.f, 0.25f) : Item.Color);
	NameText->SetText(FText::FromString(Item.IsEmpty() ? TEXT("(빈 칸)") : Item.Name));
	NameText->SetColorAndOpacity(FSlateColor(Item.IsEmpty() ? FLinearColor(0.5f, 0.5f, 0.5f) : FLinearColor::White));
}

void UDemo0610_Slot::SetHighlight(bool bOn)
{
	if (Frame)
	{
		Frame->SetBrushColor(bOn ? SlotHover : SlotIdle);
	}
}

FReply UDemo0610_Slot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !Current.IsEmpty())
	{
		// UWidgetBlueprintLibrary::DetectDragIfPressed 와 같은 일: 이 위젯에서 끌기가 시작되는지 지켜봐 달라고 Slate 에 요청
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UDemo0610_Slot::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	UDemo0610_DragOp* Op = NewObject<UDemo0610_DragOp>(this);
	Op->SourceIndex = Index;
	// 마우스를 따라다니는 그림: 같은 슬롯 위젯을 하나 더 만들어 반투명하게
	UDemo0610_Slot* Visual = CreateWidget<UDemo0610_Slot>(GetOwningPlayer(), UDemo0610_Slot::StaticClass());
	Visual->bIsDragVisual = true;
	Visual->ShowItem(Current);
	Visual->SetRenderOpacity(0.75f);
	Op->DefaultDragVisual = Visual;
	Op->Pivot = EDragPivot::CenterCenter;
	OutOperation = Op;
	if (Inventory.IsValid())
	{
		Inventory->DraggingFrom = Index;
		Inventory->LastAction = FString::Printf(TEXT("OnDragDetected: %d번 칸 '%s' 들기"), Index, *Current.Name);
	}
}

void UDemo0610_Slot::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	SetHighlight(true);
}

void UDemo0610_Slot::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	SetHighlight(false);
}

bool UDemo0610_Slot::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	SetHighlight(false);
	const UDemo0610_DragOp* Op = Cast<UDemo0610_DragOp>(InOperation);
	if (!Op || !Inventory.IsValid())
	{
		return false;
	}
	// 핵심: 출발 칸과 이 칸을 교환 (빈 칸이면 '옮기기' 가 된다)
	Inventory->SwapItems(Op->SourceIndex, Index);
	Inventory->DraggingFrom = INDEX_NONE;
	return true; // 처리했다고 알려야 DragCancelled 가 안 불린다
}

void UDemo0610_Slot::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (Inventory.IsValid())
	{
		Inventory->DraggingFrom = INDEX_NONE;
		Inventory->LastAction = TEXT("OnDragCancelled: 슬롯 밖에 놓아 취소 (원래 자리 유지)");
	}
}

// ---------------------------------------------------------------- 인벤토리

void UDemo0610_Inventory::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UBorder* Panel = MakeBorder(FLinearColor(0.02f, 0.03f, 0.05f, 0.9f), FMargin(16.f));
	AddToCanvas(Canvas, Panel, FAnchors(1.f, 0.5f), FVector2D(-80.f, 40.f), FVector2D::ZeroVector, FVector2D(1.f, 0.5f));
	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Col);
	Col->AddChildToVerticalBox(MakeText(TEXT("인벤토리 4×4 — 끌어서 옮기기·교환"), 20.f, FLinearColor(1.f, 0.85f, 0.35f), true))
		->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	Grid->SetSlotPadding(FMargin(4.f));
	Col->AddChildToVerticalBox(Grid);

	for (int32 i = 0; i < Items.Num(); ++i)
	{
		UDemo0610_Slot* SlotWidget = CreateWidget<UDemo0610_Slot>(this, UDemo0610_Slot::StaticClass());
		SlotWidget->Index = i;
		SlotWidget->Inventory = this;
		SlotWidget->ShowItem(Items[i]);
		Grid->AddChildToUniformGrid(SlotWidget, i / 4, i % 4);
		Slots.Add(SlotWidget);
	}
}

void UDemo0610_Inventory::SwapItems(int32 A, int32 B)
{
	if (!Items.IsValidIndex(A) || !Items.IsValidIndex(B) || A == B)
	{
		LastAction = TEXT("OnDrop: 같은 칸에 놓음 (변화 없음)");
		return;
	}
	const bool bMove = Items[B].IsEmpty();
	LastAction = FString::Printf(TEXT("OnDrop: %d번 '%s' → %d번 %s"), A, *Items[A].Name, B,
		bMove ? TEXT("(빈 칸으로 옮김)") : *FString::Printf(TEXT("'%s' 와 교환"), *Items[B].Name));
	Items.Swap(A, B);
	++SwapCount;
	Refresh();
}

void UDemo0610_Inventory::Refresh()
{
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		Slots[i]->ShowItem(Items[i]);
	}
}

// ---------------------------------------------------------------- 데모

ADemo0610_DragDrop::ADemo0610_DragDrop()
{
	DemoId = TEXT("0610");
	Controls = TEXT("마우스 왼쪽으로 아이템을 끌어 다른 칸에 놓기 (빈 칸 = 옮기기, 찬 칸 = 교환)\n")
		TEXT("S : 0번↔5번 교환 시연 (OnDrop 과 같은 함수)\n")
		TEXT("관찰: 끄는 동안 반투명 아이템이 커서를 따라오고, 놓을 칸이 초록으로 강조된다");
	Showcase = TEXT("4×4 인벤토리에서 DetectDrag → OnDragDetected(DragDropOperation) → OnDrop 으로 옮기기·교환");
	PlayerPawnClass = nullptr;
	bShowMouseCursor = true;
}

void ADemo0610_DragDrop::BeginPlay()
{
	Super::BeginPlay();
	Inventory = CreateWidget<UDemo0610_Inventory>(GetPC(), UDemo0610_Inventory::StaticClass());
	const TPair<const TCHAR*, FLinearColor> Seed[] = {
		{ TEXT("검"), FLinearColor(0.7f, 0.7f, 0.8f) }, { TEXT("방패"), FLinearColor(0.3f, 0.45f, 0.9f) },
		{ TEXT("물약"), FLinearColor(0.9f, 0.15f, 0.2f) }, { TEXT("활"), FLinearColor(0.55f, 0.35f, 0.15f) },
		{ TEXT("보석"), FLinearColor(0.2f, 0.9f, 0.6f) }, { TEXT("반지"), FLinearColor(1.f, 0.8f, 0.1f) },
		{ TEXT("두루마리"), FLinearColor(0.85f, 0.8f, 0.6f) }, { TEXT("열쇠"), FLinearColor(0.8f, 0.6f, 0.2f) } };
	Inventory->Items.SetNum(16);
	const int32 Places[] = { 0, 1, 2, 5, 6, 9, 12, 15 };
	for (int32 i = 0; i < UE_ARRAY_COUNT(Seed); ++i)
	{
		Inventory->Items[Places[i]].Name = Seed[i].Key;
		Inventory->Items[Places[i]].Color = Seed[i].Value;
	}
	Inventory->AddToViewport(0);
	SpawnShape(GetWorld(), EShape::Cube, FVector(-100.f, -250.f, 60.f), FVector(120.f), Colors::Item);
}

void ADemo0610_DragDrop::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Inventory)
	{
		Inventory->RemoveFromParent();
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0610_DragDrop::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Inventory)
	{
		return;
	}
	if (WasKeyPressed(EKeys::S))
	{
		Inventory->SwapItems(0, 5);
	}
	SetStatus(FString::Printf(TEXT("%s\n교환·이동 %d회 · 드래그 중: %s"), *Inventory->LastAction, Inventory->SwapCount,
		Inventory->DraggingFrom == INDEX_NONE ? TEXT("없음") : *FString::Printf(TEXT("%d번 칸"), Inventory->DraggingFrom)));
}
