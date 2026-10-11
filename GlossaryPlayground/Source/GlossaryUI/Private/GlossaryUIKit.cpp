#include "GlossaryUIKit.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

namespace GlossaryUIKit
{
	UTextBlock* MakeText(UWidgetTree* Tree, const FString& Text, float Size, const FLinearColor& Color, bool bBold)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
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

	UBorder* MakeBorder(UWidgetTree* Tree, const FLinearColor& Background, const FMargin& Padding)
	{
		UBorder* Border = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Border->SetBrushColor(Background);
		Border->SetPadding(Padding);
		return Border;
	}

	UButton* MakeButton(UWidgetTree* Tree, const FString& Label, float FontSize, const FLinearColor& Background)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		Button->SetBackgroundColor(Background);
		UTextBlock* Text = MakeText(Tree, Label, FontSize);
		Text->SetJustification(ETextJustify::Center);
		if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Button->AddChild(Text)))
		{
			ButtonSlot->SetPadding(FMargin(14.f, 8.f));
		}
		return Button;
	}

	UProgressBar* MakeBar(UWidgetTree* Tree, const FLinearColor& Fill, const FLinearColor& Background)
	{
		UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
		FProgressBarStyle Style = Bar->GetWidgetStyle();
		Style.BackgroundImage = FSlateColorBrush(Background);
		Style.FillImage = FSlateColorBrush(FLinearColor::White); // 색은 FillColorAndOpacity 로
		Style.MarqueeImage = FSlateColorBrush(FLinearColor::White);
		Bar->SetWidgetStyle(Style);
		Bar->SetFillColorAndOpacity(Fill);
		return Bar;
	}

	USizeBox* WrapSize(UWidgetTree* Tree, UWidget* Content, float Width, float Height)
	{
		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		if (Width > 0.f)
		{
			Box->SetWidthOverride(Width);
		}
		if (Height > 0.f)
		{
			Box->SetHeightOverride(Height);
		}
		Box->SetContent(Content);
		return Box;
	}
}
