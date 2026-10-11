#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "Demo0614_Minimap.generated.h"

class UImage;
class UTextBlock;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

/** 오른쪽 위 미니맵: Render Target 을 그리는 Image + 가운데 플레이어 화살표 */
UCLASS()
class GLOSSARYUI_API UDemo0614_MinimapWidget : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	void SetRenderTarget(UTextureRenderTarget2D* Target);
	void SetPlayerYaw(float Yaw);
	void SetCaption(const FString& Text);

	float MapSize = 320.f;

protected:
	virtual void BuildTree() override;

	UPROPERTY(Transient) TObjectPtr<UImage> MapImage;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Arrow;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Caption;
};

/**
 * 0614 미니맵 (SceneCapture2D)
 * 플레이어 위 SceneCaptureComponent2D(Orthographic) 를 Render Target 에 그려 오른쪽 위 미니맵으로 보여 준다.
 */
UCLASS()
class GLOSSARYUI_API ADemo0614_Minimap : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0614_Minimap();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 위에서 내려다보는 직교 카메라 (지도 촬영용) */
	UPROPERTY(VisibleAnywhere, Category = "Minimap") TObjectPtr<USceneCaptureComponent2D> MinimapCapture;

	/** 미니맵이 보여 주는 가로 폭 (cm). 작을수록 확대 */
	UPROPERTY(EditAnywhere, Category = "Minimap", meta = (ClampMin = 500)) float OrthoWidth = 4000.f;
	/** 플레이어 머리 위 카메라 높이 */
	UPROPERTY(EditAnywhere, Category = "Minimap") float CaptureHeight = 3000.f;
	UPROPERTY(EditAnywhere, Category = "Minimap") int32 RenderTargetSize = 512;
	/** true 면 플레이어가 보는 방향이 항상 위 (회전 미니맵) */
	UPROPERTY(EditAnywhere, Category = "Minimap") bool bRotateWithPlayer = false;

	UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> RenderTarget;
	UPROPERTY(Transient) TObjectPtr<UDemo0614_MinimapWidget> MinimapWidget;
};
