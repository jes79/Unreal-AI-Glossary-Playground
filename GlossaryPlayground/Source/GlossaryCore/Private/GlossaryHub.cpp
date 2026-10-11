#include "GlossaryHub.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GlossaryCatalog.h"
#include "GlossaryGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"

// ---------------------------------------------------------------- UGlossaryPayloadButton

void UGlossaryPayloadButton::Bind()
{
	OnClicked.AddUniqueDynamic(this, &UGlossaryPayloadButton::HandleClicked);
}

void UGlossaryPayloadButton::HandleClicked()
{
	if (OnPicked)
	{
		OnPicked(Payload);
	}
}

// ---------------------------------------------------------------- UGlossaryHubWidget

void UGlossaryHubWidget::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();

	UBorder* Frame = MakeBorder(FLinearColor(0.02f, 0.03f, 0.05f, 0.88f), FMargin(24.f));
	AddToCanvas(Canvas, Frame, FAnchors(0.f, 0.f, 1.f, 1.f), FVector2D::ZeroVector)->SetOffsets(FMargin(60.f, 40.f, 60.f, 40.f));

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Frame->SetContent(Column);

	Column->AddChildToVerticalBox(MakeText(TEXT("언리얼 AI 요청 용어집 플레이그라운드"), 30.f, FLinearColor(1.f, 0.85f, 0.35f), true));
	Column->AddChildToVerticalBox(MakeText(
		FString::Printf(TEXT("용어 %d개 · 왼쪽에서 분야를 고르고, 오른쪽 용어를 누르면 예제 레벨로 이동합니다. 레벨에서 F2 를 누르면 여기로 돌아옵니다."),
			UGlossaryCatalog::GetEntries().Num()), 15.f, FLinearColor(0.8f, 0.85f, 0.9f)))
		->SetPadding(FMargin(0.f, 4.f, 0.f, 16.f));

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* RowSlot = Column->AddChildToVerticalBox(Row);
	RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// 왼쪽: 분야
	UVerticalBox* Categories = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Row->AddChildToHorizontalBox(Categories)->SetPadding(FMargin(0.f, 0.f, 24.f, 0.f));
	const TArray<FGlossaryCategory>& AllCategories = UGlossaryCatalog::GetCategories();
	for (int32 Index = 0; Index < AllCategories.Num(); ++Index)
	{
		const FGlossaryCategory& Category = AllCategories[Index];
		int32 Count = 0;
		for (const FGlossaryEntry& Entry : UGlossaryCatalog::GetEntries())
		{
			Count += Entry.GetCategoryIndex() == Index ? 1 : 0;
		}
		UGlossaryPayloadButton* Button = WidgetTree->ConstructWidget<UGlossaryPayloadButton>(UGlossaryPayloadButton::StaticClass());
		UTextBlock* Label = MakeText(FString::Printf(TEXT("%s  %s  (%d)"), *Category.Code, *Category.Title, Count), 16.f);
		Button->AddChild(Label);
		Button->Payload = FString::FromInt(Index);
		Button->OnPicked = [this](const FString& Payload) { ShowCategory(FCString::Atoi(*Payload)); };
		Button->Bind();
		Categories->AddChildToVerticalBox(Button)->SetPadding(FMargin(0.f, 2.f));
		CategoryButtons.Add(Button);
	}

	// 오른쪽: 용어 목록
	UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Row->AddChildToHorizontalBox(Right)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	CategoryTitle = MakeText(TEXT(""), 22.f, FLinearColor(0.55f, 0.8f, 1.f), true);
	Right->AddChildToVerticalBox(CategoryTitle)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	TermList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	Right->AddChildToVerticalBox(TermList)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
}

void UGlossaryHubWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ShowCategory(CurrentCategory);
}

void UGlossaryHubWidget::ShowCategory(int32 CategoryIndex)
{
	if (!TermList)
	{
		return;
	}
	CurrentCategory = CategoryIndex;
	const TArray<FGlossaryCategory>& Categories = UGlossaryCatalog::GetCategories();
	if (!Categories.IsValidIndex(CategoryIndex))
	{
		return;
	}
	CategoryTitle->SetText(FText::FromString(FString::Printf(TEXT("%s  %s"), *Categories[CategoryIndex].Code, *Categories[CategoryIndex].Title)));
	for (int32 Index = 0; Index < CategoryButtons.Num(); ++Index)
	{
		CategoryButtons[Index]->SetBackgroundColor(Index == CategoryIndex ? FLinearColor(0.2f, 0.45f, 0.9f) : FLinearColor(0.12f, 0.14f, 0.18f));
	}

	TermList->ClearChildren();
	for (const FGlossaryEntry& Entry : UGlossaryCatalog::GetEntries())
	{
		if (Entry.GetCategoryIndex() != CategoryIndex)
		{
			continue;
		}
		UGlossaryPayloadButton* Button = WidgetTree->ConstructWidget<UGlossaryPayloadButton>(UGlossaryPayloadButton::StaticClass());
		Button->SetBackgroundColor(FLinearColor(0.10f, 0.11f, 0.14f));
		UVerticalBox* Lines = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Lines->AddChildToVerticalBox(MakeText(FString::Printf(TEXT("%s  %s"), *Entry.Id, *Entry.Term), 17.f, FLinearColor::White, true));
		Lines->AddChildToVerticalBox(MakeText(Entry.Plain, 13.f, FLinearColor(0.7f, 0.75f, 0.8f)));
		if (UButtonSlot* LinesSlot = Cast<UButtonSlot>(Button->AddChild(Lines)))
		{
			LinesSlot->SetHorizontalAlignment(HAlign_Left);
			LinesSlot->SetPadding(FMargin(12.f, 6.f));
		}
		Button->Payload = Entry.Id;
		Button->OnPicked = [this](const FString& Id) { OpenDemo(Id); };
		Button->Bind();
		TermList->AddChild(Button);
		if (UScrollBoxSlot* ScrollSlot = Cast<UScrollBoxSlot>(Button->Slot))
		{
			ScrollSlot->SetPadding(FMargin(0.f, 2.f));
		}
	}
	TermList->ScrollToStart();
}

void UGlossaryHubWidget::OpenDemo(const FString& Id)
{
	const FGlossaryEntry* Entry = UGlossaryCatalog::Find(Id);
	if (!Entry)
	{
		return;
	}
	const FString Package = UGlossaryCatalog::GetLevelPackage(*Entry);
	if (!FPackageName::DoesPackageExist(Package))
	{
		CategoryTitle->SetText(FText::FromString(FString::Printf(TEXT("레벨이 아직 없습니다: %s  (Tools > Glossary > Build All Levels)"), *Package)));
		return;
	}
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
	UGameplayStatics::OpenLevel(this, FName(*Package));
}

// ---------------------------------------------------------------- AGlossaryHubDemo

AGlossaryHubDemo::AGlossaryHubDemo()
{
	DemoId = TEXT("0000");
	PlayerPawnClass = nullptr;
	bShowMouseCursor = true;
	Environment.Sky = EGlossarySky::Sunset;
	Environment.bClouds = true;
}

void AGlossaryHubDemo::BeginPlay()
{
	// 허브에는 정보 패널 대신 허브 화면을 띄운다 → Super(AGlossaryDemoBase) 를 건너뛰지 않고, 패널은 0000 이라 내용이 없으니 숨긴다
	Super::BeginPlay();
	if (UGlossaryInfoPanel* Panel = GetInfoPanel())
	{
		Panel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (APlayerController* PC = GetPC())
	{
		Hub = CreateWidget<UGlossaryHubWidget>(PC, UGlossaryHubWidget::StaticClass());
		Hub->SetIsFocusable(true);
		Hub->AddToViewport(10);
		PC->SetShowMouseCursor(true);
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(Hub->TakeWidget());
		PC->SetInputMode(Mode);
	}
}
