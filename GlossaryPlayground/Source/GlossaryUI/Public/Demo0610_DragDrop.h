#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "Demo0610_DragDrop.generated.h"

class UBorder;
class UTextBlock;
class UDemo0610_Inventory;

/** 인벤토리 칸 하나의 내용 (이름이 비면 빈 칸) */
USTRUCT()
struct FDemo0610Item
{
	GENERATED_BODY()

	UPROPERTY() FString Name;
	UPROPERTY() FLinearColor Color = FLinearColor::White;
	bool IsEmpty() const { return Name.IsEmpty(); }
};

/** 드래그 중인 정보: 어느 칸에서 출발했는지 */
UCLASS()
class GLOSSARYUI_API UDemo0610_DragOp : public UDragDropOperation
{
	GENERATED_BODY()

public:
	int32 SourceIndex = INDEX_NONE;
};

/** 슬롯 위젯: 누르고 끌면 OnDragDetected, 놓으면 OnDrop */
UCLASS()
class GLOSSARYUI_API UDemo0610_Slot : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	int32 Index = 0;
	TWeakObjectPtr<UDemo0610_Inventory> Inventory;
	bool bIsDragVisual = false;

	void ShowItem(const FDemo0610Item& Item);
	void SetHighlight(bool bOn);

protected:
	virtual void BuildTree() override;
	/** 1) 왼쪽 버튼을 누르면 '끌기 감지' 를 요청 (DetectDragIfPressed 와 같음) */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	/** 2) 일정 거리 이상 끌면 호출 → DragDropOperation 을 만든다 */
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	/** 3) 다른 슬롯 위에서 놓으면 호출 → 교환 */
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(Transient) TObjectPtr<UBorder> Frame;
	UPROPERTY(Transient) TObjectPtr<UBorder> Icon;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText;
	FDemo0610Item Current;
};

/** 4×4 인벤토리 */
UCLASS()
class GLOSSARYUI_API UDemo0610_Inventory : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	UPROPERTY() TArray<FDemo0610Item> Items;
	UPROPERTY(Transient) TArray<TObjectPtr<UDemo0610_Slot>> Slots;

	void SwapItems(int32 A, int32 B);
	void Refresh();

	int32 SwapCount = 0;
	FString LastAction = TEXT("-");
	int32 DraggingFrom = INDEX_NONE;

protected:
	virtual void BuildTree() override;
};

/**
 * 0610 드래그 앤 드롭
 * 인벤토리 슬롯에 OnDragDetected / OnDrop 을 구현해서 아이템을 옮기고 교환한다.
 */
UCLASS()
class GLOSSARYUI_API ADemo0610_DragDrop : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0610_DragDrop();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(Transient) TObjectPtr<UDemo0610_Inventory> Inventory;
};
