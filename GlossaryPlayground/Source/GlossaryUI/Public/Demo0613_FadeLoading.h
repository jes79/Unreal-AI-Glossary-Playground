#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "Demo0613_FadeLoading.generated.h"

class UProgressBar;
class UTextBlock;

/** 화면 전체를 덮는 로딩 위젯 */
UCLASS()
class GLOSSARYUI_API UDemo0613_LoadingWidget : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	void SetProgress(float Percent, const FString& Message);

protected:
	virtual void BuildTree() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(Transient) TObjectPtr<UProgressBar> Bar;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Message;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Spinner;
	float SpinTime = 0.f;
};

/**
 * 0613 페이드 / 로딩 화면
 * StartCameraFade 로 0.5초 페이드 아웃 → 로딩 위젯 → OpenLevel(같은 레벨) → 새 레벨에서 페이드 인.
 */
UCLASS()
class GLOSSARYUI_API ADemo0613_FadeLoading : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0613_FadeLoading();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 페이드 아웃 / 인 시간 */
	UPROPERTY(EditAnywhere, Category = "Fade Loading", meta = (ClampMin = 0)) float FadeDuration = 0.5f;
	/** 로딩 위젯을 보여 주는 최소 시간 (연출용) */
	UPROPERTY(EditAnywhere, Category = "Fade Loading", meta = (ClampMin = 0)) float LoadingMinTime = 1.5f;

	UPROPERTY(Transient) TObjectPtr<UDemo0613_LoadingWidget> Loading;

private:
	enum class EStage : uint8 { Idle, FadingOut, Loading, Opening };
	EStage Stage = EStage::Idle;
	float StageTime = 0.f;
	int32 ReloadCount = 0;
	void StartTransition();
};
