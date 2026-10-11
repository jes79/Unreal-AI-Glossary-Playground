#include "Demo0609_ListView.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GlossaryKit.h"
#include "GlossaryUIKit.h"

using namespace GlossaryKit;

int32 UDemo0609_Entry::NumCreated = 0;
int32 UDemo0609_Entry::NumItemSets = 0;

static const FLinearColor RarityColors[] = {
	FLinearColor(0.85f, 0.85f, 0.85f), FLinearColor(0.3f, 0.6f, 1.f), FLinearColor(0.75f, 0.35f, 1.f), FLinearColor(1.f, 0.65f, 0.1f) };

// ---------------------------------------------------------------- 엔트리

void UDemo0609_Entry::BuildTree()
{
	Background = MakeBorder(FLinearColor(0.08f, 0.09f, 0.12f, 0.95f), FMargin(10.f, 4.f));
	WidgetTree->RootWidget = Background;
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Background->SetContent(Row);

	IndexText = MakeText(TEXT(""), 14.f, FLinearColor(0.6f, 0.6f, 0.65f));
	Row->AddChildToHorizontalBox(GlossaryUIKit::WrapSize(WidgetTree, IndexText, 60.f, 0.f))->SetVerticalAlignment(VAlign_Center);
	NameText = MakeText(TEXT(""), 16.f, FLinearColor::White, true);
	UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(NameText);
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);
	PriceText = MakeText(TEXT(""), 14.f, FLinearColor(1.f, 0.85f, 0.3f));
	Row->AddChildToHorizontalBox(PriceText)->SetVerticalAlignment(VAlign_Center);
	// 이 엔트리 위젯 자체의 번호 (재사용되는 것을 눈으로 보기 위해)
	SerialText = MakeText(TEXT(""), 12.f, FLinearColor(0.4f, 0.9f, 0.6f));
	Row->AddChildToHorizontalBox(GlossaryUIKit::WrapSize(WidgetTree, SerialText, 90.f, 0.f))->SetVerticalAlignment(VAlign_Center);
	SerialText->SetJustification(ETextJustify::Right);
}

void UDemo0609_Entry::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Serial = ++NumCreated; // 위젯 인스턴스가 처음 만들어질 때 한 번
}

void UDemo0609_Entry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	++NumItemSets;
	// 핵심: 같은 엔트리 위젯이 스크롤될 때마다 다른 아이템을 받아 내용만 바꾼다
	const UDemo0609_Item* Item = Cast<UDemo0609_Item>(ListItemObject);
	if (!Item || !NameText)
	{
		return;
	}
	IndexText->SetText(FText::FromString(FString::Printf(TEXT("#%04d"), Item->Index)));
	NameText->SetText(FText::FromString(Item->Name));
	NameText->SetColorAndOpacity(FSlateColor(RarityColors[Item->Rarity]));
	PriceText->SetText(FText::FromString(FString::Printf(TEXT("%d G"), Item->Price)));
	SerialText->SetText(FText::FromString(FString::Printf(TEXT("엔트리 %d"), Serial)));
	Background->SetBrushColor(Item->Index % 2 ? FLinearColor(0.08f, 0.09f, 0.12f, 0.95f) : FLinearColor(0.11f, 0.12f, 0.16f, 0.95f));
}

// ---------------------------------------------------------------- ListView 서브클래스

UDemo0609_ItemListView::UDemo0609_ItemListView(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// WBP 디테일 패널의 'Entry Widget Class' 와 같은 값
	EntryWidgetClass = UDemo0609_Entry::StaticClass();
}

// ---------------------------------------------------------------- 패널

void UDemo0609_Panel::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UBorder* Frame = MakeBorder(FLinearColor(0.02f, 0.03f, 0.05f, 0.9f), FMargin(12.f));
	AddToCanvas(Canvas, Frame, FAnchors(1.f, 0.5f), FVector2D(-40.f, 0.f), FVector2D(620.f, 820.f), FVector2D(1.f, 0.5f));
	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Frame->SetContent(Col);
	Header = MakeText(TEXT(""), 16.f, FLinearColor(1.f, 0.85f, 0.35f), true);
	Col->AddChildToVerticalBox(Header)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	List = WidgetTree->ConstructWidget<UDemo0609_ItemListView>(UDemo0609_ItemListView::StaticClass());
	UVerticalBoxSlot* ListSlot = Col->AddChildToVerticalBox(List);
	ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
}

// ---------------------------------------------------------------- 데모

ADemo0609_ListView::ADemo0609_ListView()
{
	DemoId = TEXT("0609");
	Controls = TEXT("마우스 휠 : 스크롤   S : 자동 스크롤 ON/OFF   1 : 맨 위   2 : #999 로 이동\n")
		TEXT("관찰: 아이템은 1000개지만 엔트리 위젯은 화면에 보이는 수 + 몇 개만 만들어지고, 오른쪽 '엔트리 N' 번호가 재사용된다");
	Showcase = TEXT("아이템 1000개 ListView, 생성된 엔트리 위젯 수와 재사용(NativeOnListItemObjectSet) 횟수 표시");
	PlayerPawnClass = nullptr;
	bShowMouseCursor = true;
}

void ADemo0609_ListView::BeginPlay()
{
	Super::BeginPlay();
	UDemo0609_Entry::NumCreated = 0;
	UDemo0609_Entry::NumItemSets = 0;

	static const TCHAR* Kinds[] = { TEXT("검"), TEXT("방패"), TEXT("물약"), TEXT("반지"), TEXT("활"), TEXT("투구"), TEXT("두루마리"), TEXT("보석") };
	static const TCHAR* Prefix[] = { TEXT("낡은"), TEXT("단단한"), TEXT("빛나는"), TEXT("전설의") };
	for (int32 i = 0; i < ItemCount; ++i)
	{
		UDemo0609_Item* Item = NewObject<UDemo0609_Item>(this);
		Item->Index = i;
		Item->Rarity = (i * 7 + i / 13) % 10 < 6 ? 0 : (i % 10 < 8 ? 1 : (i % 23 == 0 ? 3 : 2));
		Item->Name = FString::Printf(TEXT("%s %s"), Prefix[Item->Rarity], Kinds[i % UE_ARRAY_COUNT(Kinds)]);
		Item->Price = 10 + (i * 37) % 990;
		Items.Add(Item);
	}

	Panel = CreateWidget<UDemo0609_Panel>(GetPC(), UDemo0609_Panel::StaticClass());
	Panel->AddToViewport(0);
	Panel->List->SetListItems(Items); // 1000 개를 한 번에 넣어도 엔트리는 보이는 만큼만
	SpawnShape(GetWorld(), EShape::Cube, FVector(-100.f, -200.f, 60.f), FVector(120.f), Colors::Item);
}

void ADemo0609_ListView::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Panel)
	{
		Panel->RemoveFromParent();
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0609_ListView::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Panel || !Panel->List)
	{
		return;
	}
	UDemo0609_ItemListView* List = Panel->List;
	if (WasKeyPressed(EKeys::S)) { bAutoScroll = !bAutoScroll; ScrollPos = List->GetScrollOffset(); }
	if (WasKeyPressed(EKeys::One)) { bAutoScroll = false; List->ScrollIndexIntoView(0); }
	if (WasKeyPressed(EKeys::Two)) { bAutoScroll = false; List->ScrollIndexIntoView(Items.Num() - 1); }
	if (bAutoScroll && AutoScrollSpeed > 0.f)
	{
		ScrollPos += AutoScrollSpeed * DeltaSeconds;
		if (ScrollPos > Items.Num())
		{
			ScrollPos = 0.f;
		}
		List->SetScrollOffset(ScrollPos); // ListView 의 스크롤 오프셋 단위는 '아이템 수'
	}
	const int32 Displayed = List->GetDisplayedEntryWidgets().Num();
	const FString Line = FString::Printf(TEXT("아이템 %d개 · 엔트리 위젯 생성 %d개 · 지금 표시 %d개 · 재사용(ItemObjectSet) %d회"),
		List->GetNumItems(), UDemo0609_Entry::NumCreated, Displayed, UDemo0609_Entry::NumItemSets);
	Panel->Header->SetText(FText::FromString(Line));
	SetStatus(FString::Printf(TEXT("%s\n자동 스크롤 %s · 스크롤 위치 %.1f"), *Line, bAutoScroll ? TEXT("ON") : TEXT("OFF"), List->GetScrollOffset()));
}
