#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "Demo0602_AnchorDPI.generated.h"

class UTextBlock;

/** 네 모서리 + 가운데 Anchor 위젯과, Anchor 없이 절대 좌표로 놓은 '나쁜 예'를 함께 보여 주는 HUD */
UCLASS()
class GLOSSARYUI_API UDemo0602_AnchorHud : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ScaleText;

protected:
	virtual void BuildTree() override;
	/** Anchor 하나를 보여 주는 상자 + 앵커 지점 표시 점 */
	void AddAnchored(class UCanvasPanel* Canvas, const FString& Label, const FAnchors& Anchors, const FVector2D& Alignment,
		const FVector2D& Position, const FVector2D& Size, const FLinearColor& Color);
};

/**
 * 0602 Anchor / DPI Scaling
 * 체력바는 왼쪽 위, 미니맵은 오른쪽 위 Anchor 로 고정하고, DPI Scaling 커브(1080p = 1.0)로 해상도가 바뀌어도 비율을 유지한다.
 */
UCLASS()
class GLOSSARYUI_API ADemo0602_AnchorDPI : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0602_AnchorDPI();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** DPI 커브 기준 해상도 (Shortest Side 1080 일 때 배율 1.0) */
	UPROPERTY(EditAnywhere, Category = "DPI Scaling")
	float ReferenceShortSide = 1080.f;

	UPROPERTY(Transient)
	TObjectPtr<UDemo0602_AnchorHud> Hud;

private:
	void ApplyDpiCurve(bool bUse1080Curve);

	bool bDpiOn = true;
	FRuntimeFloatCurve SavedCurve;
	uint8 SavedRule = 0;
};
