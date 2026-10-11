#include "D11_SidePanel.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY(LogGlossaryPatterns);

UD11_SidePanel* UD11_SidePanel::Create(APlayerController* PC, const FString& Title, const FVector2D& Anchor, float FontSize)
{
	if (!PC)
	{
		return nullptr;
	}
	UD11_SidePanel* Panel = CreateWidget<UD11_SidePanel>(PC, UD11_SidePanel::StaticClass());
	Panel->PendingTitle = Title;
	Panel->AnchorPoint = Anchor;
	Panel->BodyFontSize = FontSize;
	// 클릭을 막지 않게(월드 조작 그대로)
	Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
	Panel->AddToViewport(50);
	return Panel;
}

void UD11_SidePanel::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UBorder* Frame = MakeBorder(FLinearColor(0.02f, 0.03f, 0.05f, 0.78f), FMargin(14.f, 10.f));
	const FVector2D Offset(AnchorPoint.X > 0.5f ? -16.f : 16.f, AnchorPoint.Y > 0.5f ? -16.f : 16.f);
	AddToCanvas(Canvas, Frame, FAnchors(AnchorPoint.X, AnchorPoint.Y), Offset, FVector2D::ZeroVector, AnchorPoint);

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Frame->SetContent(Box);

	TitleText = MakeText(PendingTitle, 16.f, FLinearColor(1.f, 0.8f, 0.2f), true);
	Box->AddChildToVerticalBox(TitleText);

	BodyText = MakeText(PendingBody, BodyFontSize, FLinearColor(0.92f, 0.94f, 0.97f));
	if (UVerticalBoxSlot* BodySlot = Box->AddChildToVerticalBox(BodyText))
	{
		BodySlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	}
}

void UD11_SidePanel::SetTitle(const FString& InTitle)
{
	PendingTitle = InTitle;
	if (TitleText)
	{
		TitleText->SetText(FText::FromString(InTitle));
	}
}

void UD11_SidePanel::SetBody(const FString& InBody)
{
	if (PendingBody == InBody && BodyText)
	{
		return; // 같은 내용이면 레이아웃을 다시 계산하지 않는다
	}
	PendingBody = InBody;
	if (BodyText)
	{
		BodyText->SetText(FText::FromString(InBody));
	}
}

// ---------------------------------------------------------------- FD11_LogLines

void FD11_LogLines::Add(const FString& Line)
{
	UE_LOG(LogGlossaryPatterns, Log, TEXT("%s"), *Line);
	Lines.Add(Line);
	while (Lines.Num() > MaxLines)
	{
		Lines.RemoveAt(0);
	}
}

FString FD11_LogLines::Join() const
{
	return FString::Join(Lines, TEXT("\n"));
}
