#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "Demo0812_Inventory.generated.h"

class UBorder;
class UTextBlock;
class UD08_SidePanel;

/** 슬롯 한 칸: 아이템 ID + 개수 (ID 가 None 이면 빈 칸) */
USTRUCT(BlueprintType)
struct FD0812_Slot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName ItemId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Count = 0;

	bool IsEmpty() const { return ItemId.IsNone() || Count <= 0; }
};

/** 아이템 정의 (실제 프로젝트라면 DataTable/DataAsset) */
struct FD0812_ItemDef
{
	FString Name;
	int32 MaxStack = 1;
	FLinearColor Color;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FD0812_OnInventoryChanged);

/**
 * 인벤토리 컴포넌트: 어느 액터에나 붙이면 가방이 생긴다.
 * 슬롯 20칸, 같은 아이템은 MaxStack 까지 겹치고 넘치면 다음 칸으로. 바뀔 때마다 OnInventoryChanged 방송.
 */
UCLASS(ClassGroup = (Glossary), meta = (BlueprintSpawnableComponent))
class GLOSSARYSYSTEMS_API UD0812_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UD0812_InventoryComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = 1))
	int32 SlotCount = 20;

	/** UI 는 이 델리게이트만 구독한다 (Tick 으로 매번 검사하지 않음) */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FD0812_OnInventoryChanged OnInventoryChanged;

	/** 추가. 가방이 차서 못 넣은 개수를 돌려준다 */
	UFUNCTION(BlueprintCallable, Category = "Inventory") int32 AddItem(FName ItemId, int32 Count);
	/** 제거. 실제로 뺀 개수를 돌려준다 (뒤 칸부터) */
	UFUNCTION(BlueprintCallable, Category = "Inventory") int32 RemoveItem(FName ItemId, int32 Count);
	/** 슬롯의 아이템 1개 사용 (소모품만). 성공하면 true */
	UFUNCTION(BlueprintCallable, Category = "Inventory") bool UseItem(int32 SlotIndex);
	UFUNCTION(BlueprintCallable, Category = "Inventory") int32 CountOf(FName ItemId) const;
	int32 FindFirstSlot(FName ItemId) const;

	const TArray<FD0812_Slot>& GetSlots() const { return Slots; }
	static const FD0812_ItemDef* GetDef(FName ItemId);

	/** 마지막으로 사용한 아이템 효과 (데모 표시용) */
	FString LastUseResult;

protected:
	virtual void InitializeComponent() override;

	UPROPERTY(VisibleAnywhere, Category = "Inventory") TArray<FD0812_Slot> Slots;
};

/** 5×4 슬롯 격자 UI */
UCLASS()
class GLOSSARYSYSTEMS_API UD0812_InventoryWidget : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	void Refresh(const UD0812_InventoryComponent* Inventory);

protected:
	virtual void BuildTree() override;

	UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> Cells;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> CellTexts;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Header;
};

/**
 * 0812 인벤토리 (Actor Component)
 * 플레이어에 InventoryComponent 를 붙이고, 바닥 아이템 줍기·키로 추가/사용/제거 → OnInventoryChanged 로 UI 갱신.
 */
UCLASS()
class GLOSSARYSYSTEMS_API ADemo0812_Inventory : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0812_Inventory();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION() void HandleInventoryChanged();

private:
	void SpawnPickup(FName ItemId, int32 Count, const FVector& Location);

	UPROPERTY(Transient) TObjectPtr<UD0812_InventoryComponent> Inventory;
	UPROPERTY(Transient) TObjectPtr<UD0812_InventoryWidget> Grid;
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Pickups;
	TMap<TObjectPtr<AActor>, TPair<FName, int32>> PickupContents;
	int32 ChangeCount = 0;
	FString LastAction;
};
