#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "Demo0607_WidgetAnimation.generated.h"

class UWidgetAnimation;
class UBorder;
class UTextBlock;

/**
 * WBP 'Animations' 탭이 만드는 것과 같은 UWidgetAnimation 을 실행 중에 조립하는 도우미.
 * UWidgetAnimation = MovieScene(트랙·섹션·키) + AnimationBindings(위젯 이름 ↔ 트랙 GUID).
 */
namespace Demo0607
{
	struct FKey2D { float Time; FVector2D Translation; FVector2D Scale; };

	/** Target 의 RenderTransform(2D Transform 트랙) 과 RenderOpacity(Float 트랙) 를 키로 움직이는 애니메이션 */
	UWidgetAnimation* BuildAnimation(UUserWidget* Owner, UWidget* Target, const TArray<FKey2D>& TransformKeys,
		const TArray<TPair<float, float>>& OpacityKeys);
}

/** 마우스를 올리면 1.1배 커지는 버튼 (자기 자신 = 루트 위젯을 애니메이션) */
UCLASS()
class GLOSSARYUI_API UDemo0607_HoverButton : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	FString Label;
	/** 마우스를 올렸을 때 크기 */
	float HoverScale = 1.1f;
	float HoverDuration = 0.15f;

	void SetHovered(bool bHovered);
	bool IsHoveredByDemo() const { return bHoveredState; }

	UPROPERTY(Transient) TObjectPtr<UWidgetAnimation> HoverAnim;

protected:
	virtual void BuildTree() override;
	virtual void NativeConstruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	bool bHoveredState = false;
};

/** 아래에서 슬라이드되며 열리는 창 + 호버 버튼 3개 */
UCLASS()
class GLOSSARYUI_API UDemo0607_Menu : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	void Open();
	void Close();
	bool IsOpenAnimPlaying() const;
	float GetOpenAnimTime() const;

	UPROPERTY(Transient) TObjectPtr<UWidgetAnimation> OpenAnim;
	UPROPERTY(Transient) TArray<TObjectPtr<UDemo0607_HoverButton>> Buttons;
	UPROPERTY(Transient) TObjectPtr<UBorder> Window;

	float SlideDistance = 400.f;
	float OpenDuration = 0.4f;
	float HoverScale = 1.1f;
	bool bOpen = false;

protected:
	virtual void BuildTree() override;
	virtual void NativeConstruct() override;
};

/**
 * 0607 Widget Animation
 * 버튼에 마우스를 올리면 1.1배 커지고, 창은 아래에서 슬라이드되며 열린다 (UWidgetAnimation + PlayAnimation).
 */
UCLASS()
class GLOSSARYUI_API ADemo0607_WidgetAnimation : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0607_WidgetAnimation();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "Widget Animation") float HoverScale = 1.1f;
	UPROPERTY(EditAnywhere, Category = "Widget Animation") float SlideDistance = 400.f;
	UPROPERTY(EditAnywhere, Category = "Widget Animation") float OpenDuration = 0.4f;

	UPROPERTY(Transient) TObjectPtr<UDemo0607_Menu> Menu;

private:
	int32 DemoHoverIndex = -1;
};
