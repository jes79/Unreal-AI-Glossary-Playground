#include "Demo1111_PythonScripting.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Texture2D.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

namespace
{
	/** 빌더가 만든 예제 텍스처 (GlossaryPatternsModule.cpp 의 BuildGlossaryPatternsAssets) */
	const TCHAR* TextureNames[] = { TEXT("T_Brick_D"), TEXT("T_Brick_N"), TEXT("T_Metal_D"), TEXT("T_Metal_N"), TEXT("T_Rock_Normal"), TEXT("T_Mask_R") };

	FString CompressionName(const UTexture* Texture)
	{
		// enum 값 → "TC_Normalmap" 같은 이름 (리플렉션)
		return StaticEnum<TextureCompressionSettings>()->GetNameStringByValue(Texture->CompressionSettings);
	}
}

// ================================================================ UD1111_TextureStrip

void UD1111_TextureStrip::SetTextures(const TArray<UTexture2D*>& InTextures)
{
	Textures.Reset();
	for (UTexture2D* Tex : InTextures)
	{
		Textures.Add(Tex);
	}
	Highlight.Init(false, Textures.Num());
}

void UD1111_TextureStrip::SetHighlight(const TArray<bool>& InHighlight)
{
	Highlight = InHighlight;
	Refresh();
}

void UD1111_TextureStrip::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	AddToCanvas(Canvas, Row, FAnchors(0.5f, 1.f), FVector2D(0.f, -20.f), FVector2D::ZeroVector, FVector2D(0.5f, 1.f));

	for (UTexture2D* Tex : Textures)
	{
		UBorder* Frame = MakeBorder(FLinearColor(0.02f, 0.03f, 0.05f, 0.85f), FMargin(6.f));
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Frame->SetContent(Column);

		USizeBox* ImageBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		ImageBox->SetWidthOverride(150.f);
		ImageBox->SetHeightOverride(150.f);
		UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		if (Tex)
		{
			Image->SetBrushFromTexture(Tex);
		}
		ImageBox->SetContent(Image);
		Column->AddChildToVerticalBox(ImageBox);

		UTextBlock* Caption = MakeText(TEXT(""), 11.f);
		Column->AddChildToVerticalBox(Caption);

		if (UHorizontalBoxSlot* HSlot = Row->AddChildToHorizontalBox(Frame))
		{
			HSlot->SetPadding(FMargin(4.f, 0.f));
		}
		Captions.Add(Caption);
		Frames.Add(Frame);
	}
	Refresh();
}

void UD1111_TextureStrip::Refresh()
{
	for (int32 i = 0; i < Captions.Num() && i < Textures.Num(); ++i)
	{
		const UTexture2D* Tex = Textures[i];
		if (!Tex)
		{
			Captions[i]->SetText(FText::FromString(TEXT("(없음: 빌더 실행 필요)")));
			continue;
		}
		const bool bNormal = Tex->CompressionSettings == TC_Normalmap;
		Captions[i]->SetText(FText::FromString(FString::Printf(TEXT("%s\n%s\nsRGB %s"),
			*Tex->GetName(), *CompressionName(Tex), Tex->SRGB ? TEXT("true") : TEXT("false"))));
		Captions[i]->SetColorAndOpacity(FSlateColor(bNormal ? Colors::Ally : FLinearColor::White));
		const bool bHi = Highlight.IsValidIndex(i) && Highlight[i];
		Frames[i]->SetBrushColor(bHi ? FLinearColor(0.9f, 0.5f, 0.05f, 0.95f) : FLinearColor(0.02f, 0.03f, 0.05f, 0.85f));
	}
}

// ================================================================ ADemo1111

ADemo1111_PythonScripting::ADemo1111_PythonScripting()
{
	DemoId = TEXT("1111");
	Controls = TEXT("1 : 스크립트 규칙 미리보기 — 바뀔 텍스처를 주황 테두리로 표시\n")
		TEXT("실제 실행(에디터): Output Log 아래 입력칸을 Cmd → py glossary_texture_normalmap.py  (또는 Python 모드에서 import 후 run())\n")
		TEXT("관찰: 실행 뒤 이 레벨을 다시 열면 _N·Normal 텍스처가 TC_Normalmap · sRGB false 로 바뀌어 있다");
	Showcase = TEXT("Python 스크립트로 노멀맵 압축 설정 일괄 변경, 레벨은 텍스처의 현재 설정을 실행 중에 읽어 전/후 확인");
	Implementation = EGlossaryImplementation::EditorTool;
	AlternativeNote = TEXT("Python 은 에디터에서만 돈다. 스크립트 파일 Content/Python/glossary_texture_normalmap.py + 결과를 보여 주는 레벨");
	PlayerPawnClass = nullptr;
}

bool ADemo1111_PythonScripting::IsNormalMapName(const FString& AssetName)
{
	// Python 의 is_normal_map() 과 같은 규칙
	return AssetName.EndsWith(TEXT("_N")) || AssetName.Contains(TEXT("normal"), ESearchCase::IgnoreCase);
}

void ADemo1111_PythonScripting::BeginPlay()
{
	Super::BeginPlay();
	TArray<UTexture2D*> Loaded;
	for (const TCHAR* Name : TextureNames)
	{
		UTexture2D* Tex = LoadAsset<UTexture2D>(FString(TEXT("/Game/Glossary/11_Patterns/Textures/")) + Name);
		Loaded.Add(Tex);
		Textures.Add(Tex);
	}
	Strip = CreateWidget<UD1111_TextureStrip>(GetPC(), UD1111_TextureStrip::StaticClass());
	Strip->SetTextures(Loaded);
	Strip->SetVisibility(ESlateVisibility::HitTestInvisible);
	Strip->AddToViewport(40);

	Panel = UD11_SidePanel::Create(GetPC(), TEXT("glossary_texture_normalmap.py"), FVector2D(1.f, 0.f), 13.f);
}

void ADemo1111_PythonScripting::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One))
	{
		bPreview = !bPreview;
		TArray<bool> Hi;
		for (const UTexture2D* Tex : Textures)
		{
			Hi.Add(bPreview && Tex && IsNormalMapName(Tex->GetName()) && Tex->CompressionSettings != TC_Normalmap);
		}
		Strip->SetHighlight(Hi);
	}

	int32 NormalCandidates = 0, AlreadyNormal = 0;
	for (const UTexture2D* Tex : Textures)
	{
		if (Tex && IsNormalMapName(Tex->GetName()))
		{
			++NormalCandidates;
			AlreadyNormal += Tex->CompressionSettings == TC_Normalmap ? 1 : 0;
		}
	}

	if (Panel)
	{
		Panel->SetBody(FString::Printf(
			TEXT("위치: GlossaryPlayground/Content/Python/\n  (Content/Python 은 엔진이 자동으로 sys.path 에 넣는다)\n\n")
			TEXT("실행 방법 (에디터)\n")
			TEXT("  A) Output Log 입력칸 왼쪽을 Cmd 로 두고\n     py glossary_texture_normalmap.py\n")
			TEXT("     다른 폴더: py glossary_texture_normalmap.py /Game/Textures\n")
			TEXT("  B) 입력칸을 Python 으로 바꾸고\n     import glossary_texture_normalmap as g; g.run()\n")
			TEXT("  C) File > Execute Python Script... 로 파일 선택\n\n")
			TEXT("스크립트가 하는 일\n")
			TEXT("  EditorAssetLibrary.list_assets(폴더)\n  → Texture2D 중 이름이 _N / Normal 이면\n")
			TEXT("  compression_settings = TC_NORMALMAP, srgb = False\n  → save_loaded_asset\n\n")
			TEXT("노멀맵 후보 %d개 중 이미 변환됨 %d개%s"),
			NormalCandidates, AlreadyNormal, AlreadyNormal == NormalCandidates && NormalCandidates > 0 ? TEXT(" (스크립트 실행 후 상태)") : TEXT(" (스크립트 실행 전 상태)")));
	}
	SetStatus(FString::Printf(TEXT("노멀맵 후보 %d · TC_Normalmap %d · 미리보기 %s\n(빌더가 텍스처를 다시 만들면 TC_Default 로 돌아간다)"),
		NormalCandidates, AlreadyNormal, bPreview ? TEXT("ON") : TEXT("OFF")));
}
