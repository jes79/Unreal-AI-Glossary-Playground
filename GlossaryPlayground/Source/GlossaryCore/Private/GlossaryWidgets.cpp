#include "GlossaryWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

// ---------------------------------------------------------------- UGlossaryWidgetBase

TSharedRef<SWidget> UGlossaryWidgetBase::RebuildWidget()
{
	// WBP 가 없으니 처음 화면에 올라갈 때 트리를 코드로 만든다.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

UTextBlock* UGlossaryWidgetBase::MakeText(const FString& Text, float Size, const FLinearColor& Color, bool bBold)
{
	UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Block->SetText(FText::FromString(Text));
	FSlateFontInfo Font = Block->GetFont();
	Font.Size = FMath::RoundToInt(Size);
	Font.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
	Block->SetFont(Font);
	Block->SetColorAndOpacity(FSlateColor(Color));
	Block->SetShadowOffset(FVector2D(1.f, 1.f));
	Block->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	return Block;
}

UBorder* UGlossaryWidgetBase::MakeBorder(const FLinearColor& Background, const FMargin& InPadding)
{
	UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Border->SetBrushColor(Background);
	Border->SetPadding(InPadding);
	return Border;
}

UButton* UGlossaryWidgetBase::MakeButton(const FString& Label, float FontSize, const FLinearColor& Background)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetBackgroundColor(Background);
	UTextBlock* Text = MakeText(Label, FontSize, FLinearColor::White);
	Text->SetJustification(ETextJustify::Center);
	if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Button->AddChild(Text)))
	{
		ButtonSlot->SetPadding(FMargin(12.f, 6.f));
	}
	return Button;
}

UCanvasPanel* UGlossaryWidgetBase::MakeRootCanvas()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Canvas;
	return Canvas;
}

UCanvasPanelSlot* UGlossaryWidgetBase::AddToCanvas(UCanvasPanel* Canvas, UWidget* Child, const FAnchors& Anchors,
	const FVector2D& Position, const FVector2D& Size, const FVector2D& Alignment)
{
	UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Child);
	CanvasSlot->SetAnchors(Anchors);
	CanvasSlot->SetAlignment(Alignment);
	CanvasSlot->SetPosition(Position);
	if (Size.IsNearlyZero())
	{
		CanvasSlot->SetAutoSize(true);
	}
	else
	{
		CanvasSlot->SetSize(Size);
	}
	return CanvasSlot;
}

// ---------------------------------------------------------------- UGlossaryLabelWidget

void UGlossaryLabelWidget::BuildTree()
{
	UBorder* Border = MakeBorder(FLinearColor(0.f, 0.f, 0.f, 0.45f), FMargin(6.f, 2.f));
	Label = MakeText(PendingText, PendingSize, PendingColor);
	Label->SetJustification(ETextJustify::Center);
	Border->SetContent(Label);
	WidgetTree->RootWidget = Border;
}

void UGlossaryLabelWidget::SetLabel(const FString& Text, const FLinearColor& Color, float FontSize)
{
	PendingText = Text;
	PendingColor = Color;
	PendingSize = FontSize;
	if (!Label)
	{
		return; // 트리가 아직 없으면 BuildTree 에서 Pending 값으로 만든다
	}
	Label->SetText(FText::FromString(Text));
	Label->SetColorAndOpacity(FSlateColor(Color));
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = FMath::RoundToInt(FontSize);
	Label->SetFont(Font);
}

// ---------------------------------------------------------------- UGlossaryInfoPanel

void UGlossaryInfoPanel::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();

	UBorder* Panel = MakeBorder(FLinearColor(0.02f, 0.03f, 0.05f, 0.78f), FMargin(16.f, 12.f));
	AddToCanvas(Canvas, Panel, FAnchors(0.f, 0.f), FVector2D(16.f, 16.f), FVector2D::ZeroVector);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Column);

	TitleText = MakeText(TEXT(""), 22.f, FLinearColor(1.f, 0.85f, 0.35f), true);
	Column->AddChildToVerticalBox(TitleText);

	Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Column->AddChildToVerticalBox(Body);

	auto AddSection = [this](const FString& Heading, TObjectPtr<UTextBlock>& Out, const FLinearColor& Color)
	{
		UTextBlock* HeadingText = MakeText(Heading, 12.f, FLinearColor(0.55f, 0.75f, 1.f), true);
		Body->AddChildToVerticalBox(HeadingText)->SetPadding(FMargin(0.f, 10.f, 0.f, 2.f));
		Out = MakeText(TEXT(""), 15.f, Color);
		Out->SetAutoWrapText(true);
		Out->SetWrapTextAt(PanelWidth);
		Body->AddChildToVerticalBox(Out);
	};

	PlainText = MakeText(TEXT(""), 16.f, FLinearColor::White);
	PlainText->SetAutoWrapText(true);
	PlainText->SetWrapTextAt(PanelWidth);
	Body->AddChildToVerticalBox(PlainText)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));

	AddSection(TEXT("이렇게 요청하세요"), RequestText, FLinearColor(0.85f, 0.95f, 0.85f));
	AddSection(TEXT("조작 · 관찰할 것"), ControlsText, FLinearColor(0.92f, 0.92f, 0.92f));
	AddSection(TEXT("상태"), StatusText, FLinearColor(1.f, 0.9f, 0.4f));

	FooterText = MakeText(TEXT("F1 접기 · F2 허브 · F3 마우스 커서"), 11.f, FLinearColor(0.6f, 0.6f, 0.65f));
	Column->AddChildToVerticalBox(FooterText)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
}

void UGlossaryInfoPanel::Setup(const FString& Title, const FString& Plain, const FString& Request, const FString& Controls, const FString& Implementation)
{
	if (!TitleText)
	{
		return;
	}
	TitleText->SetText(FText::FromString(Title));
	PlainText->SetText(FText::FromString(Implementation.IsEmpty() ? Plain : FString::Printf(TEXT("%s\n[%s]"), *Plain, *Implementation)));
	RequestText->SetText(FText::FromString(FString::Printf(TEXT("\"%s\""), *Request)));
	ControlsText->SetText(FText::FromString(Controls));
	StatusText->SetText(FText::GetEmpty());
}

void UGlossaryInfoPanel::SetStatus(const FString& Status)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Status));
	}
}

void UGlossaryInfoPanel::ToggleCollapsed()
{
	bCollapsed = !bCollapsed;
	if (Body)
	{
		Body->SetVisibility(bCollapsed ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
}
