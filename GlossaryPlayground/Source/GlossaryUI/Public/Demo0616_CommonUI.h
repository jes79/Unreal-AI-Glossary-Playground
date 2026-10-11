#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "Demo0616_CommonUI.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UCommonActivatableWidgetStack;

/**
 * Common UI 메뉴 화면의 공통 베이스 (UCommonActivatableWidget).
 * 스택에 올라가면 활성화(Activate) 되어 포커스를 받고, '뒤로' 를 받으면 비활성화되어 스택에서 빠진다.
 */
UCLASS(Abstract)
class GLOSSARYUI_API UDemo0616_MenuBase : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<UCommonActivatableWidgetStack> Stack;
	FString Title;
	/** 데모·키 입력에서 부르는 '뒤로' (Common UI 의 HandleBackAction 경로) */
	void RequestBack() { HandleBackAction(); }
	/** 첫 버튼을 누른 것처럼 (키보드 시연용) */
	virtual void ActivateFirst() {}
	int32 GetButtonCount() const { return Buttons.Num(); }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual bool NativeOnHandleBackAction() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** 서브클래스가 버튼을 채운다 */
	virtual void BuildButtons(UVerticalBox* Column) {}
	UButton* AddButton(UVerticalBox* Column, const FString& Label);

	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> Buttons;
	/** 루트(메인) 화면은 뒤로 가지 않는다 */
	bool bIsRootScreen = false;
};

UCLASS()
class GLOSSARYUI_API UDemo0616_MainMenu : public UDemo0616_MenuBase
{
	GENERATED_BODY()
public:
	UDemo0616_MainMenu();
	virtual void ActivateFirst() override { OpenOptions(); }
protected:
	virtual void BuildButtons(UVerticalBox* Column) override;
	UFUNCTION() void OpenOptions();
	UFUNCTION() void StartGame();
};

UCLASS()
class GLOSSARYUI_API UDemo0616_OptionsMenu : public UDemo0616_MenuBase
{
	GENERATED_BODY()
public:
	UDemo0616_OptionsMenu();
	virtual void ActivateFirst() override { OpenGraphics(); }
protected:
	virtual void BuildButtons(UVerticalBox* Column) override;
	UFUNCTION() void OpenGraphics();
	UFUNCTION() void GoBack();
};

UCLASS()
class GLOSSARYUI_API UDemo0616_GraphicsMenu : public UDemo0616_MenuBase
{
	GENERATED_BODY()
public:
	UDemo0616_GraphicsMenu();
	virtual void ActivateFirst() override { CycleQuality(); }
protected:
	virtual void BuildButtons(UVerticalBox* Column) override;
	UFUNCTION() void CycleQuality();
	UFUNCTION() void GoBack();
	UPROPERTY(Transient) TObjectPtr<UTextBlock> QualityText;
	int32 Quality = 2;
};

/** 화면 레이아웃: 오른쪽 영역에 Activatable Widget Stack 하나 */
UCLASS()
class GLOSSARYUI_API UDemo0616_RootLayout : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient) TObjectPtr<UCommonActivatableWidgetStack> MenuStack;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Breadcrumb;

protected:
	virtual void BuildTree() override;
};

/**
 * 0616 Common UI
 * Activatable Widget Stack 으로 메인 → 옵션 → 그래픽 메뉴 스택을 만들고, 패드 B(또는 Esc)로 뒤로 간다.
 */
UCLASS()
class GLOSSARYUI_API ADemo0616_CommonUI : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0616_CommonUI();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(Transient) TObjectPtr<UDemo0616_RootLayout> Layout;

	int32 PendingGraphicsSteps = 0;
};
