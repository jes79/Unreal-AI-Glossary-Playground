#include "Demo0612_Typewriter.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GlossaryKit.h"
#include "GlossaryUIKit.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- 대화창

void UDemo0612_DialogueBox::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UBorder* Box = MakeBorder(FLinearColor(0.02f, 0.02f, 0.04f, 0.88f), FMargin(28.f, 18.f));
	// 아래쪽 가로 늘이기 (좌우 여백 200, 높이 230)
	UCanvasPanelSlot* BoxSlot = AddToCanvas(Canvas, Box, FAnchors(0.f, 1.f, 1.f, 1.f), FVector2D(200.f, -40.f), FVector2D(200.f, 230.f), FVector2D(0.f, 1.f));
	BoxSlot->SetOffsets(FMargin(200.f, -40.f, 200.f, 230.f));

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Box->SetContent(Col);
	SpeakerText = MakeText(TEXT(""), 22.f, FLinearColor(1.f, 0.8f, 0.3f), true);
	Col->AddChildToVerticalBox(SpeakerText)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	BodyText = MakeText(TEXT(""), 24.f, FLinearColor::White);
	BodyText->SetAutoWrapText(true);
	UVerticalBoxSlot* BodySlot = Col->AddChildToVerticalBox(BodyText);
	BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NextMark = MakeText(TEXT("▼ 클릭"), 16.f, FLinearColor(1.f, 0.85f, 0.35f));
	Col->AddChildToVerticalBox(NextMark)->SetHorizontalAlignment(HAlign_Right);
}

void UDemo0612_DialogueBox::StartLine(const FDemo0612Line& Line)
{
	Full = Line.Text;
	Visible = 0;
	Accumulator = 0.f;
	if (SpeakerText)
	{
		SpeakerText->SetText(FText::FromString(Line.Speaker));
	}
	ApplyVisible();
}

bool UDemo0612_DialogueBox::SkipToEnd()
{
	if (!IsTyping())
	{
		return false;
	}
	Visible = Full.Len(); // 남은 글자를 한 번에
	ApplyVisible();
	return true;
}

void UDemo0612_DialogueBox::ApplyVisible()
{
	if (BodyText)
	{
		// 앞에서 Visible 글자만 보여 준다 (FString::Left)
		BodyText->SetText(FText::FromString(Full.Left(Visible)));
	}
}

void UDemo0612_DialogueBox::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (IsTyping())
	{
		// 핵심: 시간이 CharInterval 만큼 쌓일 때마다 한 글자씩. 프레임이 느려도 밀린 글자를 한꺼번에 따라잡는다
		Accumulator += InDeltaTime;
		int32 Added = 0;
		while (Accumulator >= CharInterval && Visible < Full.Len())
		{
			Accumulator -= CharInterval;
			++Visible;
			++Added;
		}
		if (Added > 0)
		{
			ApplyVisible();
		}
	}
	// 다 찍혔을 때만 ▼ 깜빡임
	BlinkTime += InDeltaTime;
	if (NextMark)
	{
		NextMark->SetRenderOpacity(IsTyping() ? 0.f : (FMath::Fmod(BlinkTime, 0.8f) < 0.5f ? 1.f : 0.25f));
	}
}

// ---------------------------------------------------------------- 데모

ADemo0612_Typewriter::ADemo0612_Typewriter()
{
	DemoId = TEXT("0612");
	Controls = TEXT("클릭 / Space / Enter : 타이핑 중이면 전체 표시, 다 나왔으면 다음 대사\n")
		TEXT("[ / ] : 글자 간격 -0.01 / +0.01초\n")
		TEXT("관찰: 글자가 0.03초마다 하나씩 찍히고, 한 번 클릭하면 남은 글자가 한꺼번에 나타난다");
	Showcase = TEXT("대화창에 글자당 0.03초 타이핑, 클릭 시 전체 표시 → 다음 대사, ▼ 깜빡임");
	PlayerPawnClass = nullptr;
	bShowMouseCursor = true;

	Lines = {
		{ TEXT("마을 촌장"), TEXT("오, 드디어 왔구먼. 북쪽 숲에서 이상한 빛이 보인다는 소문을 들었나?") },
		{ TEXT("주인공"), TEXT("네, 오는 길에 들었습니다. 무슨 일이 있는 건가요?") },
		{ TEXT("마을 촌장"), TEXT("밤마다 숲 속에서 노랫소리가 들리고, 다녀온 사람들은 아무것도 기억하지 못한다네. 자네가 한번 가 봐 주겠나?") },
		{ TEXT("주인공"), TEXT("맡겨 주세요. 해가 지기 전에 출발하겠습니다.") },
	};
}

void ADemo0612_Typewriter::BeginPlay()
{
	Super::BeginPlay();
	GlossaryUIKit::SetupFixedCamera(this, FVector(-650.f, 0.f, 260.f), FRotator(-12.f, 0.f, 0.f)); // 고정 카메라
	SpawnShape(GetWorld(), EShape::Cylinder, FVector(-100.f, -120.f, 90.f), FVector(80.f, 80.f, 180.f), Colors::Ally);
	SpawnShape(GetWorld(), EShape::Cylinder, FVector(-100.f, 120.f, 90.f), FVector(80.f, 80.f, 180.f), Colors::Player);
	Box = CreateWidget<UDemo0612_DialogueBox>(GetPC(), UDemo0612_DialogueBox::StaticClass());
	Box->CharInterval = CharInterval;
	Box->SetVisibility(ESlateVisibility::HitTestInvisible); // 클릭은 PlayerController 가 받는다
	Box->AddToViewport(0);
	if (Lines.Num() > 0)
	{
		Box->StartLine(Lines[0]);
	}
}

void ADemo0612_Typewriter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Box)
	{
		Box->RemoveFromParent();
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0612_Typewriter::Advance()
{
	if (Box->SkipToEnd())
	{
		++SkipCount; // 1단계: 타이핑 중 클릭 → 전체 표시
		return;
	}
	// 2단계: 다 나온 상태에서 클릭 → 다음 대사 (끝나면 처음부터)
	LineIndex = (LineIndex + 1) % Lines.Num();
	Box->StartLine(Lines[LineIndex]);
}

void ADemo0612_Typewriter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Box || Lines.Num() == 0)
	{
		return;
	}
	if (WasKeyPressed(EKeys::LeftMouseButton) || WasKeyPressed(EKeys::SpaceBar) || WasKeyPressed(EKeys::Enter))
	{
		Advance();
	}
	if (WasKeyPressed(EKeys::LeftBracket)) { CharInterval = FMath::Max(0.01f, CharInterval - 0.01f); Box->CharInterval = CharInterval; }
	if (WasKeyPressed(EKeys::RightBracket)) { CharInterval += 0.01f; Box->CharInterval = CharInterval; }
	SetStatus(FString::Printf(TEXT("대사 %d / %d · %s %d / %d 글자 · 글자 간격 %.2fs\n건너뛰기(전체 표시) %d회"),
		LineIndex + 1, Lines.Num(), Box->IsTyping() ? TEXT("타이핑 중") : TEXT("완료"), Box->GetVisibleCount(), Box->GetFullCount(),
		CharInterval, SkipCount));
}
