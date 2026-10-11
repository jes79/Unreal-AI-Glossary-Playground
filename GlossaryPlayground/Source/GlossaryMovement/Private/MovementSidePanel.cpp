#include "MovementSidePanel.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"

UMovementSidePanel* UMovementSidePanel::Create(APlayerController* PC, const FString& Title, float Width)
{
	if (!PC)
	{
		return nullptr;
	}
	UMovementSidePanel* Panel = CreateWidget<UMovementSidePanel>(PC, UMovementSidePanel::StaticClass());
	Panel->PanelWidth = Width;
	Panel->PendingTitle = Title;
	// 클릭을 막지 않는다 (0114 탑다운 클릭 이동 등)
	Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
	Panel->AddToViewport(90);
	return Panel;
}

void UMovementSidePanel::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UBorder* Border = MakeBorder(FLinearColor(0.02f, 0.03f, 0.05f, 0.72f), FMargin(14.f, 10.f));
	// 오른쪽 위 앵커 + 정렬 (1,0): 화면 오른쪽 끝에서 16px 안쪽
	AddToCanvas(Canvas, Border, FAnchors(1.f, 0.f), FVector2D(-16.f, 16.f), FVector2D::ZeroVector, FVector2D(1.f, 0.f));

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Border->SetContent(Column);

	TitleText = MakeText(PendingTitle, 16.f, FLinearColor(1.f, 0.85f, 0.35f), true);
	Column->AddChildToVerticalBox(TitleText);

	BodyText = MakeText(PendingBody, 14.f, FLinearColor(0.92f, 0.95f, 1.f));
	BodyText->SetAutoWrapText(true);
	BodyText->SetWrapTextAt(PanelWidth);
	BodyText->SetMinDesiredWidth(PanelWidth * 0.6f);
	Column->AddChildToVerticalBox(BodyText)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
}

void UMovementSidePanel::SetTitle(const FString& InTitle)
{
	PendingTitle = InTitle;
	if (TitleText)
	{
		TitleText->SetText(FText::FromString(InTitle));
	}
}

void UMovementSidePanel::SetBody(const FString& InBody)
{
	if (InBody == PendingBody && BodyText)
	{
		return;
	}
	PendingBody = InBody;
	if (BodyText)
	{
		BodyText->SetText(FText::FromString(InBody));
	}
}
