#pragma once

#include "CoreMinimal.h"
#include "GlossaryWidgets.h"
#include "D03_GraphWidget.generated.h"

/**
 * 화면 오른쪽 아래에 그리는 작은 그래프 패널 (Blend Space 격자, 몽타주 타임라인, 커브 등).
 * Widget Blueprint 없이 NativePaint 에서 Slate 선·사각형·글자를 직접 그린다.
 * 좌표는 데이터 공간(XMin~XMax, YMin~YMax)으로 넣는다.
 */
UCLASS()
class GLOSSARYANIMATION_API UD03_GraphWidget : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	struct FPoint { FVector2D P; FLinearColor Color; FString Label; float Size = 8.f; };
	struct FLine { TArray<FVector2D> Points; FLinearColor Color; float Thickness = 2.f; };
	struct FBar { float X0, X1; float Y0, Y1; FLinearColor Color; FString Label; };
	struct FText2D { FVector2D P; FString Text; FLinearColor Color; float Size = 11.f; };

	FString Title;
	FString XLabel, YLabel;
	float XMin = 0.f, XMax = 1.f, YMin = 0.f, YMax = 1.f;
	/** 격자 칸 수 (0 이면 격자 없음) */
	int32 GridX = 4, GridY = 4;
	FVector2D PanelSize = FVector2D(460.f, 300.f);
	/** 화면 오른쪽 아래 기준 여백 */
	FVector2D Margin = FVector2D(24.f, 24.f);

	TArray<FPoint> Points;
	TArray<FLine> Lines;
	TArray<FBar> Bars;
	TArray<FText2D> Texts;
	/** 현재 위치 마커 (bShowMarker) */
	bool bShowMarker = false;
	FVector2D Marker = FVector2D::ZeroVector;
	FLinearColor MarkerColor = FLinearColor(1.f, 0.85f, 0.1f);
	/** 세로 커서 (타임라인 재생 위치 등) */
	TArray<TPair<float, FLinearColor>> VLines;

	void ClearDynamic() { Points.Reset(); Lines.Reset(); Bars.Reset(); Texts.Reset(); VLines.Reset(); }

	static UD03_GraphWidget* Create(APlayerController* PC, const FString& Title);

protected:
	virtual void BuildTree() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
};
