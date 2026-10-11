#include "D03_GraphWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
	// UMG 기본 폰트(한글 폴백 포함)를 그대로 쓰기 위해 TextBlock 의 폰트를 빌려 온다
	FSlateFontInfo GD03Font;
}

UD03_GraphWidget* UD03_GraphWidget::Create(APlayerController* PC, const FString& InTitle)
{
	if (!PC)
	{
		return nullptr;
	}
	UD03_GraphWidget* W = CreateWidget<UD03_GraphWidget>(PC, UD03_GraphWidget::StaticClass());
	W->Title = InTitle;
	W->SetVisibility(ESlateVisibility::HitTestInvisible);
	W->AddToViewport(10);
	return W;
}

void UD03_GraphWidget::BuildTree()
{
	MakeRootCanvas();
	UTextBlock* Probe = MakeText(TEXT(" "), 11.f);
	GD03Font = Probe->GetFont();
}

int32 UD03_GraphWidget::NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, G, MyCullingRect, Out, LayerId, InWidgetStyle, bParentEnabled);

	const FVector2D Screen = G.GetLocalSize();
	const FVector2D Origin = Screen - PanelSize - Margin;   // 패널 왼쪽 위
	const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox"));

	// 그래프 영역 (제목·축 이름 자리 제외)
	const FVector2D PlotMin = Origin + FVector2D(52.f, 46.f);
	const FVector2D PlotMax = Origin + PanelSize - FVector2D(16.f, 34.f);
	auto ToScreen = [&](const FVector2D& P)
	{
		const float U = (P.X - XMin) / FMath::Max(XMax - XMin, KINDA_SMALL_NUMBER);
		const float V = (P.Y - YMin) / FMath::Max(YMax - YMin, KINDA_SMALL_NUMBER);
		return FVector2D(FMath::Lerp(PlotMin.X, PlotMax.X, U), FMath::Lerp(PlotMax.Y, PlotMin.Y, V));
	};
	auto Font = [](float Size) { FSlateFontInfo F = GD03Font.HasValidFont() ? GD03Font : FCoreStyle::GetDefaultFontStyle("Regular", 10); F.Size = FMath::RoundToInt(Size); return F; };
	auto Text = [&](const FVector2D& P, const FString& S, const FLinearColor& C, float Size, int32 Layer)
	{
		FSlateDrawElement::MakeText(Out, Layer, G.ToPaintGeometry(FVector2f(400.f, 24.f), FSlateLayoutTransform(FVector2f(P))), S, Font(Size), ESlateDrawEffect::None, C);
	};
	auto Box = [&](const FVector2D& Min, const FVector2D& Size, const FLinearColor& C, int32 Layer)
	{
		FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Min))), White, ESlateDrawEffect::None, C);
	};
	auto Line = [&](const TArray<FVector2D>& Pts, const FLinearColor& C, float Thick, int32 Layer)
	{
		TArray<FVector2f> P2;
		for (const FVector2D& P : Pts) { P2.Add(FVector2f(P)); }
		FSlateDrawElement::MakeLines(Out, Layer, G.ToPaintGeometry(), P2, ESlateDrawEffect::None, C, true, Thick);
	};

	// 배경
	Box(Origin, PanelSize, FLinearColor(0.02f, 0.025f, 0.035f, 0.82f), LayerId + 1);
	Box(PlotMin, PlotMax - PlotMin, FLinearColor(0.06f, 0.07f, 0.09f, 0.9f), LayerId + 2);
	Text(Origin + FVector2D(12.f, 8.f), Title, FLinearColor(1.f, 0.85f, 0.3f), 13.f, LayerId + 3);

	// 격자 + 축 눈금
	for (int32 i = 0; i <= GridX && GridX > 0; ++i)
	{
		const float X = FMath::Lerp(XMin, XMax, float(i) / GridX);
		Line({ ToScreen(FVector2D(X, YMin)), ToScreen(FVector2D(X, YMax)) }, FLinearColor(1, 1, 1, 0.12f), 1.f, LayerId + 3);
		Text(ToScreen(FVector2D(X, YMin)) + FVector2D(-10.f, 4.f), FString::Printf(TEXT("%g"), FMath::RoundToFloat(X * 100.f) / 100.f), FLinearColor(0.7f, 0.7f, 0.7f), 9.f, LayerId + 3);
	}
	for (int32 j = 0; j <= GridY && GridY > 0; ++j)
	{
		const float Y = FMath::Lerp(YMin, YMax, float(j) / GridY);
		Line({ ToScreen(FVector2D(XMin, Y)), ToScreen(FVector2D(XMax, Y)) }, FLinearColor(1, 1, 1, 0.12f), 1.f, LayerId + 3);
		Text(ToScreen(FVector2D(XMin, Y)) + FVector2D(-46.f, -7.f), FString::Printf(TEXT("%g"), FMath::RoundToFloat(Y * 100.f) / 100.f), FLinearColor(0.7f, 0.7f, 0.7f), 9.f, LayerId + 3);
	}
	Text(FVector2D(PlotMax.X - 160.f, PlotMax.Y + 14.f), XLabel, FLinearColor(0.8f, 0.8f, 0.8f), 10.f, LayerId + 3);
	Text(FVector2D(Origin.X + 10.f, Origin.Y + 22.f), YLabel, FLinearColor(0.8f, 0.8f, 0.8f), 10.f, LayerId + 3);

	// 막대(타임라인 구간)
	for (const FBar& B : Bars)
	{
		const FVector2D A = ToScreen(FVector2D(B.X0, B.Y1));
		const FVector2D C = ToScreen(FVector2D(B.X1, B.Y0));
		Box(A, C - A, B.Color, LayerId + 4);
		if (!B.Label.IsEmpty())
		{
			Text(A + FVector2D(6.f, 4.f), B.Label, FLinearColor::White, 11.f, LayerId + 5);
		}
	}
	// 선
	for (const FLine& L : Lines)
	{
		TArray<FVector2D> Pts;
		for (const FVector2D& P : L.Points) { Pts.Add(ToScreen(P)); }
		Line(Pts, L.Color, L.Thickness, LayerId + 5);
	}
	// 세로 커서
	for (const TPair<float, FLinearColor>& V : VLines)
	{
		Line({ ToScreen(FVector2D(V.Key, YMin)), ToScreen(FVector2D(V.Key, YMax)) }, V.Value, 2.f, LayerId + 6);
	}
	// 점
	for (const FPoint& P : Points)
	{
		const FVector2D S = ToScreen(P.P);
		Box(S - FVector2D(P.Size * 0.5f), FVector2D(P.Size), P.Color, LayerId + 6);
		if (!P.Label.IsEmpty())
		{
			Text(S + FVector2D(6.f, -16.f), P.Label, P.Color, 9.f, LayerId + 6);
		}
	}
	for (const FText2D& T : Texts)
	{
		Text(ToScreen(T.P), T.Text, T.Color, T.Size, LayerId + 7);
	}
	if (bShowMarker)
	{
		const FVector2D S = ToScreen(Marker);
		Box(S - FVector2D(8.f), FVector2D(16.f), FLinearColor::Black, LayerId + 7);
		Box(S - FVector2D(6.f), FVector2D(12.f), MarkerColor, LayerId + 8);
	}
	return LayerId + 9;
}
