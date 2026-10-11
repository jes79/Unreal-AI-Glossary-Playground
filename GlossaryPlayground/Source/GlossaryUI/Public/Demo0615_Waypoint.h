#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "Demo0615_Waypoint.generated.h"

class UCanvasPanel;
class UTextBlock;
class UVerticalBox;

/** 목표 하나에 대한 화면 마커 */
USTRUCT()
struct FDemo0615Marker
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<UVerticalBox> Box;
	UPROPERTY() TObjectPtr<UTextBlock> Icon;    // 화면 안: ◆ / 화면 밖: ▲ (방향으로 회전)
	UPROPERTY() TObjectPtr<UTextBlock> Label;
	FVector WorldLocation = FVector::ZeroVector;
	FString Name;
	FLinearColor Color = FLinearColor::White;
	bool bOnScreen = false;
	float DistanceM = 0.f;
};

/** 목표 마커 레이어: 화면 안이면 목표 위에, 밖이면 가장자리에 방향 화살표 + 거리(m) */
UCLASS()
class GLOSSARYUI_API UDemo0615_WaypointLayer : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	void AddTarget(const FString& Name, const FVector& Location, const FLinearColor& Color);
	/** 가장자리에서 띄울 여백 (Slate 단위) */
	float EdgeMargin = 60.f;
	bool bShowOffscreenArrows = true;

	UPROPERTY(Transient) TArray<FDemo0615Marker> Markers;

protected:
	virtual void BuildTree() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Canvas;
};

/**
 * 0615 화면 밖 마커 (Waypoint)
 * ProjectWorldToScreen 으로 목표 위치를 표시하고, 화면 밖이면 가장자리에 방향 화살표와 거리(m)를 띄운다.
 */
UCLASS()
class GLOSSARYUI_API ADemo0615_Waypoint : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0615_Waypoint();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "Waypoint") float EdgeMargin = 60.f;

	UPROPERTY(Transient) TObjectPtr<UDemo0615_WaypointLayer> Layer;
};
