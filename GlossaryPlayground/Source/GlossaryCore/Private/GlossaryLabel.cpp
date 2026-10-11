#include "GlossaryLabel.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "GlossaryWidgets.h"

AGlossaryLabel::AGlossaryLabel()
{
	PrimaryActorTick.bCanEverTick = false;
	Widget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	RootComponent = Widget;
	Widget->SetWidgetSpace(EWidgetSpace::Screen);
	Widget->SetDrawAtDesiredSize(true);
	Widget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Widget->SetWidgetClass(UGlossaryLabelWidget::StaticClass());
}

void AGlossaryLabel::BeginPlay()
{
	Super::BeginPlay();
	Widget->InitWidget();
	Refresh();
}

void AGlossaryLabel::SetText(const FString& InText)
{
	Text = InText;
	Refresh();
}

void AGlossaryLabel::SetColor(const FLinearColor& InColor)
{
	Color = InColor;
	Refresh();
}

void AGlossaryLabel::SetFontSize(float InSize)
{
	FontSize = InSize;
	Refresh();
}

void AGlossaryLabel::Refresh()
{
	if (UGlossaryLabelWidget* Label = Cast<UGlossaryLabelWidget>(Widget->GetUserWidgetObject()))
	{
		Label->SetLabel(Text, Color, FontSize);
	}
}
