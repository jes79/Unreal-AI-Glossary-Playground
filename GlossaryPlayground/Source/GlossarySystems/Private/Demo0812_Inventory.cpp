#include "Demo0812_Inventory.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "D08_Common.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- 아이템 정의

const FD0812_ItemDef* UD0812_InventoryComponent::GetDef(FName ItemId)
{
	static const TMap<FName, FD0812_ItemDef> Defs = {
		{ TEXT("Potion"), { TEXT("물약"), 10, FLinearColor(0.9f, 0.1f, 0.15f) } },
		{ TEXT("Arrow"),  { TEXT("화살"), 99, FLinearColor(0.7f, 0.6f, 0.4f) } },
		{ TEXT("Sword"),  { TEXT("검"),   1,  FLinearColor(0.7f, 0.75f, 0.85f) } },
		{ TEXT("Gem"),    { TEXT("보석"), 20, FLinearColor(0.6f, 0.2f, 0.95f) } },
	};
	return Defs.Find(ItemId);
}

// ---------------------------------------------------------------- 컴포넌트

UD0812_InventoryComponent::UD0812_InventoryComponent()
{
	bWantsInitializeComponent = true;
}

void UD0812_InventoryComponent::InitializeComponent()
{
	Super::InitializeComponent();
	Slots.SetNum(SlotCount);
}

int32 UD0812_InventoryComponent::AddItem(FName ItemId, int32 Count)
{
	const FD0812_ItemDef* Def = GetDef(ItemId);
	if (!Def || Count <= 0)
	{
		return Count;
	}
	if (Slots.Num() != SlotCount)
	{
		Slots.SetNum(SlotCount);
	}
	int32 Left = Count;
	// 1) 같은 아이템이 있는 칸부터 MaxStack 까지 채운다 (스택)
	for (FD0812_Slot& Slot : Slots)
	{
		if (Left > 0 && Slot.ItemId == ItemId && Slot.Count < Def->MaxStack)
		{
			const int32 Add = FMath::Min(Left, Def->MaxStack - Slot.Count);
			Slot.Count += Add;
			Left -= Add;
		}
	}
	// 2) 남으면 빈 칸에 새 스택
	for (FD0812_Slot& Slot : Slots)
	{
		if (Left > 0 && Slot.IsEmpty())
		{
			const int32 Add = FMath::Min(Left, Def->MaxStack);
			Slot.ItemId = ItemId;
			Slot.Count = Add;
			Left -= Add;
		}
	}
	if (Left != Count)
	{
		OnInventoryChanged.Broadcast();
	}
	return Left; // 가방이 차서 못 넣은 개수
}

int32 UD0812_InventoryComponent::RemoveItem(FName ItemId, int32 Count)
{
	int32 Removed = 0;
	for (int32 i = Slots.Num() - 1; i >= 0 && Removed < Count; --i)
	{
		FD0812_Slot& Slot = Slots[i];
		if (Slot.ItemId == ItemId)
		{
			const int32 Take = FMath::Min(Slot.Count, Count - Removed);
			Slot.Count -= Take;
			Removed += Take;
			if (Slot.Count <= 0)
			{
				Slot = FD0812_Slot();
			}
		}
	}
	if (Removed > 0)
	{
		OnInventoryChanged.Broadcast();
	}
	return Removed;
}

bool UD0812_InventoryComponent::UseItem(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty())
	{
		LastUseResult = TEXT("빈 칸");
		return false;
	}
	FD0812_Slot& Slot = Slots[SlotIndex];
	if (Slot.ItemId != TEXT("Potion"))
	{
		LastUseResult = FString::Printf(TEXT("%s 은(는) 사용할 수 없는 아이템"), *GetDef(Slot.ItemId)->Name);
		return false;
	}
	// 소모품: 1개 줄이고 효과 적용
	Slot.Count -= 1;
	if (Slot.Count <= 0)
	{
		Slot = FD0812_Slot();
	}
	LastUseResult = TEXT("물약 사용 → HP +30");
	OnInventoryChanged.Broadcast();
	return true;
}

int32 UD0812_InventoryComponent::CountOf(FName ItemId) const
{
	int32 Sum = 0;
	for (const FD0812_Slot& Slot : Slots)
	{
		if (Slot.ItemId == ItemId)
		{
			Sum += Slot.Count;
		}
	}
	return Sum;
}

int32 UD0812_InventoryComponent::FindFirstSlot(FName ItemId) const
{
	return Slots.IndexOfByPredicate([ItemId](const FD0812_Slot& S) { return S.ItemId == ItemId; });
}

// ---------------------------------------------------------------- UI

void UD0812_InventoryWidget::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UBorder* Box = MakeBorder(FLinearColor(0.02f, 0.03f, 0.05f, 0.85f), FMargin(10.f));
	UCanvasPanelSlot* BoxSlot = AddToCanvas(Canvas, Box, FAnchors(1.f, 1.f), FVector2D(-24.f, -24.f), FVector2D::ZeroVector, FVector2D(1.f, 1.f));
	BoxSlot->SetAutoSize(true);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Box->SetContent(Column);
	Header = MakeText(TEXT("인벤토리 0 / 20"), 16.f, FLinearColor(1.f, 0.85f, 0.35f), true);
	Column->AddChildToVerticalBox(Header);

	UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	Grid->SetSlotPadding(FMargin(3.f));
	Grid->SetMinDesiredSlotWidth(92.f);
	Grid->SetMinDesiredSlotHeight(62.f);
	Column->AddChildToVerticalBox(Grid);
	for (int32 i = 0; i < 20; ++i)
	{
		UBorder* Cell = MakeBorder(FLinearColor(0.12f, 0.13f, 0.16f, 1.f), FMargin(4.f));
		UTextBlock* Text = MakeText(TEXT(""), 13.f);
		Text->SetJustification(ETextJustify::Center);
		Cell->SetContent(Text);
		Cell->SetHorizontalAlignment(HAlign_Center);
		Cell->SetVerticalAlignment(VAlign_Center);
		UUniformGridSlot* GridSlot = Grid->AddChildToUniformGrid(Cell, i / 5, i % 5);
		GridSlot->SetHorizontalAlignment(HAlign_Fill);
		GridSlot->SetVerticalAlignment(VAlign_Fill);
		Cells.Add(Cell);
		CellTexts.Add(Text);
	}
}

void UD0812_InventoryWidget::Refresh(const UD0812_InventoryComponent* Inventory)
{
	if (!Inventory || Cells.Num() == 0)
	{
		return;
	}
	const TArray<FD0812_Slot>& Slots = Inventory->GetSlots();
	int32 Used = 0;
	for (int32 i = 0; i < Cells.Num(); ++i)
	{
		const FD0812_Slot* Cur = Slots.IsValidIndex(i) ? &Slots[i] : nullptr;
		if (Cur && !Cur->IsEmpty())
		{
			++Used;
			const FD0812_ItemDef* Def = UD0812_InventoryComponent::GetDef(Cur->ItemId);
			Cells[i]->SetBrushColor(Def->Color * 0.55f + FLinearColor(0.f, 0.f, 0.f, 0.45f));
			CellTexts[i]->SetText(FText::FromString(FString::Printf(TEXT("%s\n×%d"), *Def->Name, Cur->Count)));
		}
		else
		{
			Cells[i]->SetBrushColor(FLinearColor(0.12f, 0.13f, 0.16f, 1.f));
			CellTexts[i]->SetText(FText::FromString(FString::Printf(TEXT("%d"), i + 1)));
		}
	}
	Header->SetText(FText::FromString(FString::Printf(TEXT("인벤토리 %d / %d 칸"), Used, Cells.Num())));
}

// ---------------------------------------------------------------- 데모

ADemo0812_Inventory::ADemo0812_Inventory()
{
	DemoId = TEXT("0812");
	Controls = TEXT("WASD 이동 · 바닥 아이템에 닿으면 줍기\n")
		TEXT("1 물약+3 · 2 화살+45 · 3 검+1 · 4 보석+7 · U 물약 사용 · X 화살 10개 버리기\n")
		TEXT("관찰: 물약 10·화살 99·보석 20 까지 한 칸에 겹치고, 넘치면 다음 칸. 20칸이 차면 남은 수를 돌려준다");
	Showcase = TEXT("플레이어에 붙인 InventoryComponent: 20칸·스택·추가/제거/사용, OnInventoryChanged 로 격자 UI 갱신");
}

void ADemo0812_Inventory::BeginPlay()
{
	Super::BeginPlay();
	// 플레이어 폰에 컴포넌트를 붙인다 (블루프린트에서는 Add Component 로 끝)
	if (APawn* Pawn = GetPlayerPawn())
	{
		Inventory = NewObject<UD0812_InventoryComponent>(Pawn, TEXT("Inventory"));
		Inventory->RegisterComponent();
		Inventory->OnInventoryChanged.AddDynamic(this, &ADemo0812_Inventory::HandleInventoryChanged);
	}
	Grid = CreateWidget<UD0812_InventoryWidget>(GetPC(), UD0812_InventoryWidget::StaticClass());
	Grid->SetVisibility(ESlateVisibility::HitTestInvisible);
	Grid->AddToViewport(50);

	// 바닥 아이템
	const FName Ids[] = { TEXT("Potion"), TEXT("Arrow"), TEXT("Gem"), TEXT("Potion"), TEXT("Sword"), TEXT("Arrow"), TEXT("Gem"), TEXT("Potion") };
	const int32 Counts[] = { 2, 30, 5, 4, 1, 80, 12, 6 };
	for (int32 i = 0; i < 8; ++i)
	{
		const float Angle = 2.f * PI * i / 8.f;
		SpawnPickup(Ids[i], Counts[i], FVector(300.f + FMath::Cos(Angle) * 500.f, FMath::Sin(Angle) * 500.f, 50.f));
	}
	if (Inventory)
	{
		Inventory->AddItem(TEXT("Potion"), 3);
		Inventory->AddItem(TEXT("Arrow"), 120); // 99 + 21 → 두 칸
	}
	HandleInventoryChanged();
}

void ADemo0812_Inventory::SpawnPickup(FName ItemId, int32 Count, const FVector& Location)
{
	const FD0812_ItemDef* Def = UD0812_InventoryComponent::GetDef(ItemId);
	AStaticMeshActor* Pickup = SpawnShape(GetWorld(), EShape::Cube, Location, FVector(50.f), Def->Color, FRotator(0.f, 45.f, 0.f));
	Pickup->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetColor(Pickup->GetStaticMeshComponent(), Def->Color, 0.4f, 0.f, 0.8f);
	Pickups.Add(Pickup);
	PickupContents.Add(Pickup, TPair<FName, int32>(ItemId, Count));
	AGlossaryLabel* Label = SpawnLabel(GetWorld(), FString::Printf(TEXT("%s ×%d"), *Def->Name, Count), Location + FVector(0.f, 0.f, 70.f), Def->Color, 16.f);
	Label->AttachToActor(Pickup, FAttachmentTransformRules::KeepWorldTransform);
}

void ADemo0812_Inventory::HandleInventoryChanged()
{
	// 델리게이트로 불린다: 인벤토리가 바뀔 때만 UI 를 다시 그린다
	++ChangeCount;
	if (Grid && Inventory)
	{
		Grid->Refresh(Inventory);
	}
}

void ADemo0812_Inventory::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Inventory)
	{
		return;
	}
	auto Report = [this](const FString& What, int32 Leftover)
	{
		LastAction = Leftover > 0 ? FString::Printf(TEXT("%s → 가방이 가득 차 %d개를 못 넣음"), *What, Leftover) : What;
	};
	if (WasKeyPressed(EKeys::One))   { Report(TEXT("물약 +3"), Inventory->AddItem(TEXT("Potion"), 3)); }
	if (WasKeyPressed(EKeys::Two))   { Report(TEXT("화살 +45"), Inventory->AddItem(TEXT("Arrow"), 45)); }
	if (WasKeyPressed(EKeys::Three)) { Report(TEXT("검 +1"), Inventory->AddItem(TEXT("Sword"), 1)); }
	if (WasKeyPressed(EKeys::Four))  { Report(TEXT("보석 +7"), Inventory->AddItem(TEXT("Gem"), 7)); }
	if (WasKeyPressed(EKeys::U))
	{
		Inventory->UseItem(Inventory->FindFirstSlot(TEXT("Potion")));
		LastAction = Inventory->LastUseResult;
	}
	if (WasKeyPressed(EKeys::X))
	{
		LastAction = FString::Printf(TEXT("화살 %d개 버림"), Inventory->RemoveItem(TEXT("Arrow"), 10));
	}

	// 줍기
	if (APawn* Pawn = GetPlayerPawn())
	{
		for (int32 i = Pickups.Num() - 1; i >= 0; --i)
		{
			AActor* Pickup = Pickups[i];
			Pickup->AddActorWorldRotation(FRotator(0.f, 90.f * DeltaSeconds, 0.f));
			if (FVector::Dist2D(Pickup->GetActorLocation(), Pawn->GetActorLocation()) < 110.f)
			{
				const TPair<FName, int32> Content = PickupContents.FindRef(Pickup);
				const int32 Left = Inventory->AddItem(Content.Key, Content.Value);
				Report(FString::Printf(TEXT("%s ×%d 주움"), *UD0812_InventoryComponent::GetDef(Content.Key)->Name, Content.Value - Left), Left);
				PickupContents.Remove(Pickup);
				Pickups.RemoveAt(i);
				TArray<AActor*> Attached;
				Pickup->GetAttachedActors(Attached);
				for (AActor* A : Attached)
				{
					A->Destroy();
				}
				Pickup->Destroy();
			}
		}
	}

	SetStatus(FString::Printf(TEXT("물약 %d · 화살 %d · 검 %d · 보석 %d · OnInventoryChanged %d회\n%s"),
		Inventory->CountOf(TEXT("Potion")), Inventory->CountOf(TEXT("Arrow")), Inventory->CountOf(TEXT("Sword")),
		Inventory->CountOf(TEXT("Gem")), ChangeCount, *LastAction));
}
