#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "Demo0611_Tooltip.generated.h"

class UBorder;
class UTextBlock;

/** 툴팁에 보여 줄 아이템 정보 */
USTRUCT()
struct FDemo0611ItemInfo
{
	GENERATED_BODY()

	UPROPERTY() FString Name;
	UPROPERTY() int32 Rarity = 0;          // 0 일반 · 1 희귀 · 2 영웅 · 3 전설
	UPROPERTY() FString Description;
	UPROPERTY() FString Stats;
	UPROPERTY() FLinearColor IconColor = FLinearColor::White;
};

/** 커스텀 툴팁 위젯: 이름 · 등급(색) · 설명 · 능력치 */
UCLASS()
class GLOSSARYUI_API UDemo0611_TooltipWidget : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	FDemo0611ItemInfo Info;

	static FString RarityName(int32 Rarity);
	static FLinearColor RarityColor(int32 Rarity);

protected:
	virtual void BuildTree() override;
};

/** 툴팁을 가진 슬롯 */
UCLASS()
class GLOSSARYUI_API UDemo0611_Slot : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	FDemo0611ItemInfo Info;
	bool bHovered = false;

protected:
	virtual void BuildTree() override;
	virtual void NativeOnInitialized() override;
	void CreateTooltip();
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	UPROPERTY(Transient) TObjectPtr<UBorder> Frame;
};

UCLASS()
class GLOSSARYUI_API UDemo0611_Panel : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	TArray<FDemo0611ItemInfo> ItemInfos;
	UPROPERTY(Transient) TArray<TObjectPtr<UDemo0611_Slot>> Slots;

protected:
	virtual void BuildTree() override;
};

/**
 * 0611 툴팁
 * 슬롯에 마우스를 올리면 이름·등급·설명이 있는 커스텀 툴팁 위젯을 띄운다 (UWidget::SetToolTip).
 */
UCLASS()
class GLOSSARYUI_API ADemo0611_Tooltip : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0611_Tooltip();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(Transient) TObjectPtr<UDemo0611_Panel> Panel;

private:
	void MoveMouseToSlot(int32 Index);
	int32 DemoSlot = -1;
};
