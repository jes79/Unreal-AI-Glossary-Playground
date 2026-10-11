#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GlossaryDemoBase.h"
#include "Demo0601_WidgetBlueprint.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * 권장 패턴: "레이아웃은 WBP, 로직은 C++ 베이스 클래스".
 * WBP_HUD 를 만들고 Class Settings > Parent Class 를 이 클래스로 두면,
 * 디자이너에서 같은 이름(HealthBar · HealthText · ScoreText)으로 만든 위젯이 컴파일 때 자동으로 이 포인터에 연결된다(BindWidget).
 * 이름이 없으면 WBP 컴파일 오류가 나서 실수를 바로 잡을 수 있다. BindWidgetOptional 은 없어도 된다.
 */
UCLASS(Abstract)
class GLOSSARYUI_API UDemo0601_HudBase : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetHealth(float Current, float Max);
	void SetScore(int32 Score);

protected:
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> HealthText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ScoreText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> HintText;
};

/**
 * 이 예제의 실제 인스턴스: WBP 대신 C++ 에서 같은 이름의 위젯 트리를 만든다.
 * WBP 디자이너가 하는 일(WidgetTree 에 위젯을 만들고 BindWidget 포인터를 채우기)을 코드로 그대로 한 것.
 */
UCLASS()
class GLOSSARYUI_API UDemo0601_CodeBuiltHud : public UDemo0601_HudBase
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	void BuildTree();
};

/**
 * 0601 Widget Blueprint (UMG)
 * 체력바·점수 텍스트 HUD 를 CreateWidget + AddToViewport 로 띄운다.
 */
UCLASS()
class GLOSSARYUI_API ADemo0601_WidgetBlueprint : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0601_WidgetBlueprint();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 실제 프로젝트라면 여기에 WBP_HUD 를 지정한다 (TSubclassOf 라서 WBP 든 C++ 클래스든 받는다) */
	UPROPERTY(EditAnywhere, Category = "Widget Blueprint")
	TSubclassOf<UDemo0601_HudBase> HudClass;

	UPROPERTY(EditAnywhere, Category = "Widget Blueprint")
	float MaxHealth = 100.f;

	UPROPERTY(Transient)
	TObjectPtr<UDemo0601_HudBase> Hud;

private:
	float Health = 100.f;
	int32 Score = 0;
	void RefreshHud();
};
