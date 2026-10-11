#include "Demo0618_Localization.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GlossaryKit.h"
#include "GlossaryUIKit.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/PolyglotTextData.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTableRegistry.h"
#include "Internationalization/TextLocalizationManager.h"

using namespace GlossaryKit;

const FName ADemo0618_Localization::StringTableId(TEXT("ST_Demo0618_UI"));

namespace
{
	/** String Table 내용: 키 · 원문(영어) · 한국어 번역 */
	struct FEntry { const TCHAR* Key; const TCHAR* En; const TCHAR* Ko; };
	const FEntry Entries[] = {
		{ TEXT("Menu.Title"),    TEXT("MAIN MENU"),        TEXT("메인 메뉴") },
		{ TEXT("Menu.Start"),    TEXT("Start Game"),       TEXT("게임 시작") },
		{ TEXT("Menu.Options"),  TEXT("Options"),          TEXT("설정") },
		{ TEXT("Menu.Quit"),     TEXT("Quit"),             TEXT("종료") },
		{ TEXT("Menu.Language"), TEXT("Language: English"), TEXT("언어: 한국어") },
		{ TEXT("Hud.Health"),    TEXT("HP {Current} / {Max}"), TEXT("체력 {Current} / {Max}") },
		{ TEXT("Hud.Gold"),      TEXT("{Gold} gold · today is {Date}"), TEXT("골드 {Gold} · 오늘은 {Date}") },
	};
	const TCHAR* const TableNamespace = TEXT("Demo0618_UI");
}

// ---------------------------------------------------------------- 메뉴 위젯

UTextBlock* UDemo0618_Menu::AddKeyText(UVerticalBox* Column, const TCHAR* Key, float Size, const FLinearColor& Color, bool bBold)
{
	// 핵심: 문자열 대신 (테이블 ID, 키) 로 FText 를 만든다. 언어가 바뀌어도 이 FText 가 가리키는 키는 그대로
	const FText Text = FText::FromStringTable(ADemo0618_Localization::StringTableId, Key);
	UTextBlock* Block = MakeText(TEXT(""), Size, Color, bBold);
	Block->SetText(Text);
	Column->AddChildToVerticalBox(Block)->SetPadding(FMargin(0.f, 4.f));
	Bound.Add({ Block, Text });
	return Block;
}

void UDemo0618_Menu::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UBorder* Panel = MakeBorder(FLinearColor(0.03f, 0.04f, 0.08f, 0.92f), FMargin(32.f, 24.f));
	AddToCanvas(Canvas, Panel, FAnchors(1.f, 0.5f), FVector2D(-80.f, 40.f), FVector2D(620.f, 0.f), FVector2D(1.f, 0.5f));
	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Col);

	AddKeyText(Col, TEXT("Menu.Title"), 34.f, FLinearColor(1.f, 0.85f, 0.35f), true);
	for (const TCHAR* Key : { TEXT("Menu.Start"), TEXT("Menu.Options"), TEXT("Menu.Quit") })
	{
		AddKeyText(Col, Key, 24.f, FLinearColor::White);
	}
	AddKeyText(Col, TEXT("Menu.Language"), 18.f, FLinearColor(0.6f, 0.85f, 1.f));
	HealthLine = MakeText(TEXT(""), 20.f, FLinearColor(0.4f, 1.f, 0.5f), true);
	Col->AddChildToVerticalBox(HealthLine)->SetPadding(FMargin(0.f, 16.f, 0.f, 4.f));
	FormatLine = MakeText(TEXT(""), 18.f, FLinearColor(0.9f, 0.9f, 0.9f));
	FormatLine->SetAutoWrapText(true);
	Col->AddChildToVerticalBox(FormatLine);
	CultureLine = MakeText(TEXT(""), 13.f, FLinearColor(0.55f, 0.6f, 0.7f));
	Col->AddChildToVerticalBox(CultureLine)->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	RefreshTexts();
}

void UDemo0618_Menu::RefreshTexts()
{
	for (const TPair<TObjectPtr<UTextBlock>, FText>& Pair : Bound)
	{
		Pair.Key->SetText(Pair.Value); // 같은 FText 객체 — 현재 문화권의 번역으로 자동으로 바뀌어 있다
	}
	if (!HealthLine)
	{
		return;
	}
	// FText::Format 의 패턴도 String Table 에서 가져온다 (어순이 다른 언어도 번역가가 바꿀 수 있게)
	FFormatNamedArguments HpArgs;
	HpArgs.Add(TEXT("Current"), FText::AsNumber(72));
	HpArgs.Add(TEXT("Max"), FText::AsNumber(100));
	HealthLine->SetText(FText::Format(FText::FromStringTable(ADemo0618_Localization::StringTableId, TEXT("Hud.Health")), HpArgs));

	// 숫자·날짜는 FText::AsNumber / AsDate 가 현재 문화권 규칙(ICU)으로 쓴다
	FFormatNamedArguments GoldArgs;
	GoldArgs.Add(TEXT("Gold"), FText::AsNumber(1234567));
	GoldArgs.Add(TEXT("Date"), FText::AsDate(FDateTime(2026, 10, 11), EDateTimeStyle::Full));
	FormatLine->SetText(FText::Format(FText::FromStringTable(ADemo0618_Localization::StringTableId, TEXT("Hud.Gold")), GoldArgs));

	CultureLine->SetText(FText::FromString(FString::Printf(TEXT("CurrentCulture = %s · String Table %s"),
		*FInternationalization::Get().GetCurrentCulture()->GetName(), *ADemo0618_Localization::StringTableId.ToString())));
}

// ---------------------------------------------------------------- 데모

ADemo0618_Localization::ADemo0618_Localization()
{
	DemoId = TEXT("0618");
	Controls = TEXT("1 : 한국어   2 : English   L : 전환\n")
		TEXT("관찰: 메뉴 문자열·체력 문구 어순·날짜 형식이 한꺼번에 바뀐다. 코드는 키(Menu.Start 등)만 알고 문장은 모른다");
	Showcase = TEXT("코드로 만든 String Table 키만 쓰는 메뉴, SetCurrentCulture 로 한/영 전환 (문구·FText::Format·날짜 형식)");
	Implementation = EGlossaryImplementation::Alternative;
	AlternativeNote = TEXT("String Table·FText·SetCurrentCulture 는 실제 기능. Localization Dashboard 가 만드는 .locres 번역 파일 대신 "
		"같은 네임스페이스·키의 한국어 번역을 FPolyglotTextData 로 실행 중에 등록했다 (Gather → 번역 → Compile 단계를 생략)");
	PlayerPawnClass = nullptr;
}

void ADemo0618_Localization::RegisterStringTable()
{
	if (FStringTableRegistry::Get().FindStringTable(StringTableId).IsValid())
	{
		return; // 레벨을 다시 열어도 한 번만
	}
	// 1) String Table 을 코드로 만든다: LOCTABLE_NEW("ST_Demo0618_UI", "Demo0618_UI") 와 같음
	LOCTABLE_NEW("ST_Demo0618_UI", "Demo0618_UI");
	TArray<FPolyglotTextData> Translations;
	for (const FEntry& E : Entries)
	{
		// 2) 키 + 원문(영어): LOCTABLE_SETSTRING("ST_Demo0618_UI", "Menu.Start", "Start Game") 와 같음
		FStringTableRegistry::Get().Internal_SetLocTableEntry(StringTableId, E.Key, E.En);
		// 3) 번역: 실제 프로젝트에서는 .locres 가 하는 일. 같은 (네임스페이스, 키) 에 ko 문자열을 붙인다
		FPolyglotTextData& Data = Translations.Emplace_GetRef(ELocalizedTextSourceCategory::Game, TableNamespace, E.Key, E.En, TEXT("en"));
		Data.AddLocalizedString(TEXT("ko"), E.Ko);
	}
	FTextLocalizationManager::Get().RegisterPolyglotTextData(Translations);
}

void ADemo0618_Localization::BeginPlay()
{
	Super::BeginPlay();
	SpawnShape(GetWorld(), EShape::Cube, FVector(-100.f, -250.f, 60.f), FVector(120.f), Colors::Accent);
	OriginalCulture = FInternationalization::Get().GetCurrentCulture()->GetName();
	RegisterStringTable();
	SetLanguage(TEXT("ko"));
	Menu = CreateWidget<UDemo0618_Menu>(GetPC(), UDemo0618_Menu::StaticClass());
	Menu->AddToViewport(0);
	SwitchCount = 0;
}

void ADemo0618_Localization::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Menu)
	{
		Menu->RemoveFromParent();
	}
	if (!OriginalCulture.IsEmpty())
	{
		FInternationalization::Get().SetCurrentCulture(OriginalCulture); // 다른 레벨에 영향 없게 원래대로
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0618_Localization::SetLanguage(const FString& Culture)
{
	// 설정 메뉴의 '언어' 옵션이 부르는 함수. 문화권이 바뀌면 모든 FText 가 새 번역을 가리킨다
	FInternationalization::Get().SetCurrentCulture(Culture);
	CurrentCulture = Culture;
	++SwitchCount;
	if (Menu)
	{
		Menu->RefreshTexts();
	}
}

void ADemo0618_Localization::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One)) { SetLanguage(TEXT("ko")); }
	if (WasKeyPressed(EKeys::Two)) { SetLanguage(TEXT("en")); }
	if (WasKeyPressed(EKeys::L))   { SetLanguage(CurrentCulture == TEXT("ko") ? TEXT("en") : TEXT("ko")); }

	const FText Sample = FText::FromStringTable(StringTableId, TEXT("Menu.Start"));
	SetStatus(FString::Printf(TEXT("CurrentCulture %s · 전환 %d회\nFromStringTable(\"%s\", \"Menu.Start\") → \"%s\""),
		*FInternationalization::Get().GetCurrentCulture()->GetName(), SwitchCount, *StringTableId.ToString(), *Sample.ToString()));
}
