#pragma once

#include "CoreMinimal.h"
#include "GlossaryWidgets.h"
#include "D1014_ScalabilityWidget.generated.h"

class UTextBlock;
class UButton;
class ADemo1014_Scalability;

/**
 * 1014 그래픽 옵션 메뉴 (화면 오른쪽). 품질·해상도·프레임 제한을 고르고 [적용]을 누르면
 * 데모가 UGameUserSettings 에 값을 넣고 ApplySettings 한다. 실제 게임의 옵션 화면과 같은 흐름.
 */
UCLASS()
class GLOSSARYRENDERING_API UD1014_ScalabilityWidget : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<ADemo1014_Scalability> Demo;

	/** 고른 값(아직 적용 전)과 적용된 값을 다시 그린다 */
	void Refresh(int32 PendingQuality, FIntPoint PendingResolution, float PendingFrameLimit, const FString& AppliedText);

protected:
	virtual void BuildTree() override;

	UFUNCTION() void OnLow();
	UFUNCTION() void OnMedium();
	UFUNCTION() void OnHigh();
	UFUNCTION() void OnEpic();
	UFUNCTION() void OnCinematic();
	UFUNCTION() void OnRes720();
	UFUNCTION() void OnRes900();
	UFUNCTION() void OnRes1080();
	UFUNCTION() void OnFps30();
	UFUNCTION() void OnFps60();
	UFUNCTION() void OnFpsUnlimited();
	UFUNCTION() void OnApply();

	UPROPERTY(Transient) TObjectPtr<UTextBlock> PendingText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> AppliedText;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> QualityButtons;
};
