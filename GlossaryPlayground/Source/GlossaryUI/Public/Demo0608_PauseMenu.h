#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "Demo0608_PauseMenu.generated.h"

class ADemo0608_PauseMenu;
class UButton;

/** 일시정지 메뉴: 화면을 어둡게 덮고 '계속하기 · 허브로' 버튼. 포커스를 받아 ESC 를 직접 처리한다 */
UCLASS()
class GLOSSARYUI_API UDemo0608_PauseWidget : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<ADemo0608_PauseMenu> Demo;
	UPROPERTY(Transient) TObjectPtr<UButton> ResumeButton;

protected:
	virtual void BuildTree() override;
	virtual void NativeOnInitialized() override;
	/** UI Only 모드에서는 키 입력이 게임(PlayerController)이 아니라 포커스된 위젯으로 온다 → 여기서 ESC 를 받는다 */
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION() void HandleResume();
	UFUNCTION() void HandleHub();
};

/**
 * 0608 일시정지 메뉴 + Input Mode
 * ESC: SetGamePaused(true) + Input Mode UI Only + 마우스 커서 표시. 다시 ESC(또는 계속하기): Game Only 로 복귀.
 */
UCLASS()
class GLOSSARYUI_API ADemo0608_PauseMenu : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0608_PauseMenu();

	void SetPaused(bool bPause);
	bool IsPausedNow() const { return bPausedNow; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(Transient) TObjectPtr<UDemo0608_PauseWidget> PauseWidget;

private:
	bool bPausedNow = false;
	int32 PauseCount = 0;
	/** 자동 캡처(-GlossaryShot) 보조: 일시정지 중에는 PlayerController 의 캡처 타이머가 멈추므로 실제 시간으로 대신 찍는다 */
	float PausedRealTime = 0.f;
	int32 PausedShotStage = 0;
};
