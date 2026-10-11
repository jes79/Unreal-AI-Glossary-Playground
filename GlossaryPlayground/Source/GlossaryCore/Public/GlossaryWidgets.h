#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GlossaryWidgets.generated.h"

class UBorder;
class UButton;
class UTextBlock;
class UVerticalBox;
class UWidget;
class UPanelWidget;
class UCanvasPanel;
class UCanvasPanelSlot;

/**
 * Widget Blueprint 없이 C++ 만으로 UMG 위젯을 만드는 베이스.
 * 서브클래스는 BuildTree() 에서 WidgetTree->ConstructWidget 으로 트리를 만들고 WidgetTree->RootWidget 을 지정한다.
 * (실제 프로젝트에서는 보통 WBP 로 레이아웃을 그리고 C++ 에서는 BindWidget 으로 연결한다 — 0601 참고)
 */
UCLASS(Abstract)
class GLOSSARYCORE_API UGlossaryWidgetBase : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void BuildTree() {}

public:
	// ---- 트리 만들기 헬퍼 (BuildTree 안에서 사용) ----
	UTextBlock* MakeText(const FString& Text, float Size = 14.f, const FLinearColor& Color = FLinearColor::White, bool bBold = false);
	UBorder* MakeBorder(const FLinearColor& Background, const FMargin& InPadding = FMargin(12.f));
	/** 텍스트가 든 버튼. 클릭은 반환된 버튼의 OnClicked 에 AddDynamic 으로 연결 */
	UButton* MakeButton(const FString& Label, float FontSize = 16.f, const FLinearColor& Background = FLinearColor(0.12f, 0.14f, 0.18f, 0.95f));
	/** Canvas 루트를 만들고 RootWidget 으로 지정 */
	class UCanvasPanel* MakeRootCanvas();
	/** Canvas 에 위젯을 추가하고 앵커·위치·크기·정렬을 한 번에 지정. Size 가 0 이면 Auto Size */
	class UCanvasPanelSlot* AddToCanvas(UCanvasPanel* Canvas, UWidget* Child, const FAnchors& Anchors, const FVector2D& Position,
		const FVector2D& Size = FVector2D::ZeroVector, const FVector2D& Alignment = FVector2D::ZeroVector);
};

/** AGlossaryLabel 이 쓰는 한 줄 라벨 */
UCLASS()
class GLOSSARYCORE_API UGlossaryLabelWidget : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	void SetLabel(const FString& Text, const FLinearColor& Color, float FontSize);

protected:
	virtual void BuildTree() override;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> Label;

	// 위젯 트리가 만들어지기 전에 SetLabel 이 불려도 값이 남도록 보관
	FString PendingText;
	FLinearColor PendingColor = FLinearColor::White;
	float PendingSize = 18.f;
};

/**
 * 모든 예제 레벨 왼쪽 위 정보 패널: 용어 · 쉽게 말하면 · 요청 예문 · 조작법 · 실시간 상태.
 * F1 로 접고 펴고, F2 로 허브로 돌아간다(AGlossaryPlayerController).
 * 데모 UI 는 화면 왼쪽 위 620×440 영역을 피해서 배치한다.
 */
UCLASS()
class GLOSSARYCORE_API UGlossaryInfoPanel : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	void Setup(const FString& Title, const FString& Plain, const FString& Request, const FString& Controls, const FString& Implementation);
	void SetStatus(const FString& Status);
	void ToggleCollapsed();

	static constexpr float PanelWidth = 600.f;

protected:
	virtual void BuildTree() override;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PlainText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> RequestText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ControlsText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> FooterText;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Body;

	bool bCollapsed = false;
};
