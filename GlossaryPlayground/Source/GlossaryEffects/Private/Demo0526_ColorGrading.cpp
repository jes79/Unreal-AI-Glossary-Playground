#include "Demo0526_ColorGrading.h"

#include "EffectsDemoKit.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/Texture.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0526_ColorGrading::ADemo0526_ColorGrading()
{
	DemoId = TEXT("0526");
	Controls = TEXT("1 : 원본   2 : Color Grading (채도 0.7 · White Temp 8500K · 주황 Gain)   3 : LUT 텍스처 (T_LUT_Sunset)\n")
		TEXT("관찰: 2 와 3 은 같은 석양 톤. 2 는 볼륨의 숫자 슬라이더로, 3 은 256x16 LUT 이미지 한 장으로 색을 바꾼다.\n")
		TEXT("LUT 방식: 화면 캡처를 포토샵에서 보정 → 같은 보정을 중립 LUT 에 적용 → Color Grading LUT 에 지정");
	Showcase = TEXT("채도↓·색온도↑ 석양 톤 Color Grading ↔ 같은 톤을 코드로 만든 LUT 텍스처로 적용, 원본 비교");
	PlayerPawnClass = nullptr;
}

void ADemo0526_ColorGrading::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	// 색 견본: 빨강·주황·노랑·초록·청록·파랑·보라·회색 기둥 + 공
	const FLinearColor Palette[] = { Colors::Enemy, Colors::Accent, Colors::Item, Colors::Ally, Colors::Cyan, Colors::Player, Colors::Purple, Colors::Neutral };
	for (int32 i = 0; i < 8; ++i)
	{
		SpawnBlock(World, FVector(300.f, -700.f + i * 200.f, 0.f), FVector(150.f, 150.f, 250.f), Palette[i]);
		SpawnShape(World, EShape::Sphere, FVector(100.f, -700.f + i * 200.f, 60.f), FVector(110.f), Palette[7 - i]);
	}
	Volume = EffectsKit::SpawnUnboundPostProcess(World);
	EffectsKit::SetupFixedCamera(this, FVector(-800.f, 0.f, 320.f), FVector(200.f, 0.f, 100.f), 70.f);
	ApplyMode();
}

void ADemo0526_ColorGrading::ApplyMode()
{
	if (!Volume)
	{
		return;
	}
	FPostProcessSettings& S = Volume->Settings;
	S.bOverride_ColorSaturation = Mode == 2;
	S.bOverride_WhiteTemp = Mode == 2;
	S.bOverride_ColorGain = Mode == 2;
	S.bOverride_ColorGradingLUT = Mode == 3;
	S.bOverride_ColorGradingIntensity = Mode == 3;
	if (Mode == 2)
	{
		// 숫자로 하는 Color Grading: 채도 낮추고 색온도 높여 따뜻한 석양 톤
		S.ColorSaturation = FVector4(Saturation, Saturation, Saturation, 1.f);
		S.WhiteTemp = WhiteTemp;
		S.ColorGain = FVector4(1.08f, 0.98f, 0.85f, 1.f);
	}
	else if (Mode == 3)
	{
		// LUT 방식: 빌더가 같은 보정을 굽어 놓은 256x16 텍스처
		S.ColorGradingLUT = LoadAsset<UTexture>(EffectsKit::AssetPath(TEXT("T_LUT_Sunset")));
		S.ColorGradingIntensity = 1.f;
	}
}

void ADemo0526_ColorGrading::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One))   { Mode = 1; ApplyMode(); }
	if (WasKeyPressed(EKeys::Two))   { Mode = 2; ApplyMode(); }
	if (WasKeyPressed(EKeys::Three)) { Mode = 3; ApplyMode(); }
	const FString Desc = Mode == 1 ? FString(TEXT("원본 (보정 없음)"))
		: Mode == 2 ? FString::Printf(TEXT("Color Grading: Saturation %.1f · White Temp %.0fK · Gain (1.08, 0.98, 0.85)"), Saturation, WhiteTemp)
		: FString(TEXT("LUT: Color Grading LUT = T_LUT_Sunset (256x16, 16단계 큐브)"));
	SetStatus(Desc);
}
