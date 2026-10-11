#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Components/ListView.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "Demo0609_ListView.generated.h"

class UTextBlock;
class UBorder;

/** 목록의 '아이템' = 데이터 (1000개 전부 만든다) */
UCLASS()
class GLOSSARYUI_API UDemo0609_Item : public UObject
{
	GENERATED_BODY()

public:
	int32 Index = 0;
	FString Name;
	int32 Rarity = 0;   // 0 일반 · 1 희귀 · 2 영웅 · 3 전설
	int32 Price = 0;
};

/**
 * 목록의 '엔트리' = 화면에 보이는 줄 위젯 (보이는 만큼만 만들어 재사용한다).
 * IUserObjectListEntry 를 구현하면 ListView 가 스크롤할 때 NativeOnListItemObjectSet 으로 새 아이템을 넣어 준다.
 */
UCLASS()
class GLOSSARYUI_API UDemo0609_Entry : public UGlossaryWidgetBase, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	/** 지금까지 만들어진 엔트리 위젯 수 (아이템 수가 아니라!) */
	static int32 NumCreated;
	/** 아이템이 엔트리에 다시 꽂힌 횟수 (재사용 횟수) */
	static int32 NumItemSets;

protected:
	virtual void BuildTree() override;
	virtual void NativeOnInitialized() override;
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	UPROPERTY(Transient) TObjectPtr<UBorder> Background;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> IndexText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PriceText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SerialText;

	int32 Serial = 0;
};

/**
 * EntryWidgetClass 는 UListViewBase 의 protected UPROPERTY 라서 WBP 디테일 패널에서 고르는 값이다.
 * C++ 로만 만들 때는 이렇게 서브클래스 생성자에서 지정한다.
 */
UCLASS()
class GLOSSARYUI_API UDemo0609_ItemListView : public UListView
{
	GENERATED_BODY()

public:
	UDemo0609_ItemListView(const FObjectInitializer& ObjectInitializer);
};

/** 오른쪽 목록 패널 */
UCLASS()
class GLOSSARYUI_API UDemo0609_Panel : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient) TObjectPtr<UDemo0609_ItemListView> List;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Header;

protected:
	virtual void BuildTree() override;
};

/**
 * 0609 ListView (목록 가상화)
 * 아이템 1000개를 ListView 에 넣고, 엔트리 위젯은 보이는 만큼만 만들어 재사용한다(IUserObjectListEntry).
 */
UCLASS()
class GLOSSARYUI_API ADemo0609_ListView : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0609_ListView();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "ListView", meta = (ClampMin = 1)) int32 ItemCount = 1000;
	/** 자동 스크롤 속도 (아이템/초). 0 이면 멈춤 */
	UPROPERTY(EditAnywhere, Category = "ListView") float AutoScrollSpeed = 25.f;

	UPROPERTY(Transient) TObjectPtr<UDemo0609_Panel> Panel;
	UPROPERTY(Transient) TArray<TObjectPtr<UDemo0609_Item>> Items;

private:
	bool bAutoScroll = true;
	float ScrollPos = 0.f;
};
