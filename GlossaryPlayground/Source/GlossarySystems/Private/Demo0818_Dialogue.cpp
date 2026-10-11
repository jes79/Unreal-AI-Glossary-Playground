#include "Demo0818_Dialogue.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "D08_Common.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryCharacter.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"

using namespace GlossaryKit;

// 첫 열(---) = 행 이름(대사 ID). Next 가 End 면 대화 종료
const TCHAR* ADemo0818_Dialogue::DialogueCSV =
	TEXT("---,Speaker,Line,Choice1,Next1,Choice2,Next2,Choice3,Next3\n")
	TEXT("Start,상인,어서 오게 여행자. 무엇이 필요한가?,물건을 보고 싶어요,Shop,이 마을에 대해 알려 주세요,Town,아무것도 아니에요,Bye\n")
	TEXT("Shop,상인,오늘은 물약이 싸다네. 한 병에 50골드야.,살게요,Buy,너무 비싸요,Haggle,,\n")
	TEXT("Haggle,상인,흠... 자네니까 40골드에 주지.,살게요,Buy,다음에 올게요,Bye,,\n")
	TEXT("Buy,상인,좋은 선택이야! 또 오게.,(대화 끝),End,,,,\n")
	TEXT("Town,상인,북쪽 숲에 슬라임이 늘었어. 조심하게.,더 알려 주세요,Town2,처음으로,Start,,\n")
	TEXT("Town2,상인,숲 깊은 곳에 오래된 유적이 있다는 소문이 있지.,처음으로,Start,(대화 끝),End,,\n")
	TEXT("Bye,상인,그럼 조심히 가게.,(대화 끝),End,,,,\n");

// ---------------------------------------------------------------- 위젯

void UD0818_DialogueWidget::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UBorder* Box = MakeBorder(FLinearColor(0.03f, 0.03f, 0.06f, 0.9f), FMargin(20.f, 14.f));
	UCanvasPanelSlot* BoxSlot = AddToCanvas(Canvas, Box, FAnchors(0.5f, 1.f), FVector2D(0.f, -30.f), FVector2D(820.f, 0.f), FVector2D(0.5f, 1.f));
	BoxSlot->SetAutoSize(true);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Box->SetContent(Column);
	SpeakerText = MakeText(TEXT(""), 18.f, FLinearColor(1.f, 0.8f, 0.3f), true);
	Column->AddChildToVerticalBox(SpeakerText);
	LineText = MakeText(TEXT(""), 20.f, FLinearColor::White);
	LineText->SetAutoWrapText(true);
	LineText->SetWrapTextAt(780.f);
	Column->AddChildToVerticalBox(LineText)->SetPadding(FMargin(0.f, 6.f, 0.f, 10.f));

	for (int32 i = 0; i < 3; ++i)
	{
		UButton* Button = MakeButton(TEXT(""), 16.f);
		UTextBlock* Text = Cast<UTextBlock>(Button->GetChildAt(0));
		if (Text)
		{
			Text->SetJustification(ETextJustify::Left);
		}
		Column->AddChildToVerticalBox(Button)->SetPadding(FMargin(0.f, 3.f));
		Buttons.Add(Button);
		ButtonTexts.Add(Text);
	}
	Buttons[0]->OnClicked.AddDynamic(this, &UD0818_DialogueWidget::OnChoice0);
	Buttons[1]->OnClicked.AddDynamic(this, &UD0818_DialogueWidget::OnChoice1);
	Buttons[2]->OnClicked.AddDynamic(this, &UD0818_DialogueWidget::OnChoice2);
}

void UD0818_DialogueWidget::Show(const FD0818_DialogueRow& Row)
{
	if (!SpeakerText)
	{
		return;
	}
	SpeakerText->SetText(FText::FromString(Row.Speaker));
	LineText->SetText(FText::FromString(Row.Line));
	const FString* Choices[] = { &Row.Choice1, &Row.Choice2, &Row.Choice3 };
	for (int32 i = 0; i < 3; ++i)
	{
		const bool bHas = !Choices[i]->IsEmpty();
		Buttons[i]->SetVisibility(bHas ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bHas && ButtonTexts[i])
		{
			ButtonTexts[i]->SetText(FText::FromString(FString::Printf(TEXT("%d.  %s"), i + 1, **Choices[i])));
		}
	}
}

void UD0818_DialogueWidget::OnChoice0() { if (Owner.IsValid()) { Owner->Choose(0); } }
void UD0818_DialogueWidget::OnChoice1() { if (Owner.IsValid()) { Owner->Choose(1); } }
void UD0818_DialogueWidget::OnChoice2() { if (Owner.IsValid()) { Owner->Choose(2); } }

// ---------------------------------------------------------------- 데모

ADemo0818_Dialogue::ADemo0818_Dialogue()
{
	DemoId = TEXT("0818");
	Controls = TEXT("NPC 가까이에서 E : 말 걸기 · 선택지는 마우스 클릭 또는 1/2/3\n")
		TEXT("관찰: 고른 선택지의 Next 행으로 이동한다. 대사·분기는 모두 Data Table(CSV)에 있다");
	Showcase = TEXT("DataTable 의 대사·선택지·다음 ID 로 분기하는 대화창 (상점/마을 정보/작별 분기)");
	bShowMouseCursor = true;
	DialogueTable = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Glossary/08_Systems/DT_0818_Dialogue.DT_0818_Dialogue")));
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-250.f, 0.f, 100.f));
}

void ADemo0818_Dialogue::BeginPlay()
{
	Super::BeginPlay();
	Table = DialogueTable.LoadSynchronous();

	// NPC (초록 상인) + 가판대
	AStaticMeshActor* Body = SpawnShape(GetWorld(), EShape::Cylinder, FVector(150.f, 0.f, 80.f), FVector(70.f, 70.f, 160.f), Colors::Ally);
	SpawnShape(GetWorld(), EShape::Sphere, FVector(150.f, 0.f, 190.f), FVector(60.f), FLinearColor(0.95f, 0.8f, 0.65f));
	SpawnBlock(GetWorld(), FVector(60.f, 0.f, 0.f), FVector(60.f, 300.f, 90.f), FLinearColor(0.5f, 0.3f, 0.12f));
	for (int32 i = 0; i < 4; ++i)
	{
		SpawnShape(GetWorld(), EShape::Sphere, FVector(60.f, -110.f + i * 70.f, 110.f), FVector(30.f), Colors::Enemy);
	}
	Npc = Body;
	NpcPrompt = SpawnLabel(GetWorld(), TEXT("상인  [E] 말 걸기"), FVector(150.f, 0.f, 270.f), Colors::Item, 20.f);

	Window = CreateWidget<UD0818_DialogueWidget>(GetPC(), UD0818_DialogueWidget::StaticClass());
	Window->Owner = this;
	Window->AddToViewport(60);
	Window->SetVisibility(ESlateVisibility::Collapsed);

	if (!Table)
	{
		SetStatus(TEXT("DT_0818_Dialogue 가 없다 — tools/glossary-build.sh -filter=08"));
		return;
	}
	StartDialogue(); // 시작하자마자 말을 걸어 둔다 (가까이 서 있음)
}

void ADemo0818_Dialogue::StartDialogue()
{
	Path.Reset();
	bTalking = true;
	Window->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (AGlossaryCharacter* Character = Cast<AGlossaryCharacter>(GetPlayerPawn()))
	{
		Character->bInputEnabled = false; // 대화 중 이동 잠금
	}
	GoTo(StartRow);
}

void ADemo0818_Dialogue::GoTo(FName RowName)
{
	if (RowName.IsNone() || RowName == TEXT("End"))
	{
		EndDialogue();
		return;
	}
	// 핵심: 다음 대사 ID 로 행을 찾는다
	const FD0818_DialogueRow* Row = Table->FindRow<FD0818_DialogueRow>(RowName, TEXT("0818 대화"));
	if (!Row)
	{
		EndDialogue();
		return;
	}
	CurrentRow = RowName;
	Path.Add(RowName.ToString());
	Window->Show(*Row);
}

void ADemo0818_Dialogue::Choose(int32 Index)
{
	if (!bTalking)
	{
		return;
	}
	const FD0818_DialogueRow* Row = Table->FindRow<FD0818_DialogueRow>(CurrentRow, TEXT("0818 선택"));
	if (!Row)
	{
		return;
	}
	const FString* Choices[] = { &Row->Choice1, &Row->Choice2, &Row->Choice3 };
	const FName Nexts[] = { Row->Next1, Row->Next2, Row->Next3 };
	if (Index < 0 || Index > 2 || Choices[Index]->IsEmpty())
	{
		return;
	}
	GoTo(Nexts[Index]); // 분기
}

void ADemo0818_Dialogue::EndDialogue()
{
	bTalking = false;
	Path.Add(TEXT("End"));
	Window->SetVisibility(ESlateVisibility::Collapsed);
	if (AGlossaryCharacter* Character = Cast<AGlossaryCharacter>(GetPlayerPawn()))
	{
		Character->bInputEnabled = true;
	}
}

void ADemo0818_Dialogue::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Table)
	{
		return;
	}
	if (bTalking)
	{
		if (WasKeyPressed(EKeys::One))   { Choose(0); }
		else if (WasKeyPressed(EKeys::Two))   { Choose(1); }
		else if (WasKeyPressed(EKeys::Three)) { Choose(2); }
	}
	else if (WasKeyPressed(EKeys::E))
	{
		APawn* Pawn = GetPlayerPawn();
		if (Pawn && FVector::Dist2D(Pawn->GetActorLocation(), Npc->GetActorLocation()) <= TalkDistance)
		{
			StartDialogue();
		}
	}
	NpcPrompt->SetActorHiddenInGame(bTalking);
	SetStatus(FString::Printf(TEXT("현재 행: %s · 지나온 길: %s"), bTalking ? *CurrentRow.ToString() : TEXT("(대화 아님)"),
		*FString::Join(Path, TEXT(" → "))));
}
