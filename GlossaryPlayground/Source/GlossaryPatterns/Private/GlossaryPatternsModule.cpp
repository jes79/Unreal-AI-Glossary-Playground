#include "Modules/ModuleManager.h"

#include "Demo1110_EditorUtility.h"
#include "Demo1115_GameplayDebugger.h"

#if WITH_EDITOR
#include "Engine/Texture2D.h"
#include "GlossaryAssetKit.h"

/**
 * 11_Patterns 분야 데모가 쓰는 생성 에셋 (/Game/Glossary/11_Patterns)
 *  - S_Died                 : 1104 OnDied 사운드 구독자가 재생하는 하강음
 *  - Textures/T_*_D, T_*_N  : 1111 Python 스크립트가 압축 설정을 바꿀 예제 텍스처 (일부러 모두 TC_Default 로 만든다)
 */
static void BuildGlossaryPatternsAssets()
{
	using namespace GlossaryAssetKit;

	// ---- 1104: 쓰러짐 효과음 (반음 7 → 3 → 0 → -5 로 내려가는 짧은 멜로디)
	CreateSoundWave(TEXT("/Game/Glossary/11_Patterns/S_Died"), MakeMelody({ 7, 3, 0, -5 }, 0.11f, 0.35f));

	// ---- 1111: 텍스처 6장
	constexpr int32 Size = 128;
	const FString Folder = TEXT("/Game/Glossary/11_Patterns/Textures/");

	// 높이 함수 → 노멀(탄젠트 공간) 색으로 인코딩. 노멀맵은 (N*0.5+0.5) 를 RGB 로 저장한다
	auto NormalFromHeight = [](TFunctionRef<float(float, float)> Height, int32 X, int32 Y)
	{
		const float E = 1.f;
		const float DX = Height(X + E, Y) - Height(X - E, Y);
		const float DY = Height(X, Y + E) - Height(X, Y - E);
		const FVector N = FVector(-DX, -DY, 2.f).GetSafeNormal();
		return FColor(uint8((N.X * 0.5f + 0.5f) * 255.f), uint8((N.Y * 0.5f + 0.5f) * 255.f), uint8((N.Z * 0.5f + 0.5f) * 255.f), 255);
	};
	auto BrickHeight = [](float X, float Y)
	{
		const int32 Row = FMath::FloorToInt(Y / 32.f);
		const float BX = FMath::Fmod(X + (Row % 2) * 32.f, 64.f);
		const float BY = FMath::Fmod(Y, 32.f);
		const float Edge = FMath::Min(FMath::Min(BX, 64.f - BX), FMath::Min(BY, 32.f - BY));
		return FMath::Clamp(Edge / 4.f, 0.f, 1.f) * 6.f; // 줄눈은 낮고 벽돌 면은 높다
	};
	auto MetalHeight = [](float X, float Y)
	{
		return FMath::Sin(X * 0.35f) * 0.6f + FMath::Sin((X + Y) * 0.05f) * 2.f; // 가로 줄무늬 + 완만한 굴곡
	};
	auto RockHeight = [](float X, float Y)
	{
		return FMath::Sin(X * 0.11f) * FMath::Cos(Y * 0.13f) * 5.f + FMath::Sin((X - Y) * 0.31f) * 1.5f;
	};

	TArray<UTexture2D*> Made;
	Made.Add(CreateTexture(Folder + TEXT("T_Brick_D"), Size, Size, [&](int32 X, int32 Y)
	{
		const float H = BrickHeight(X, Y) / 6.f;
		return FLinearColor::LerpUsingHSV(FLinearColor(0.35f, 0.33f, 0.3f), FLinearColor(0.62f, 0.22f, 0.12f), H).ToFColor(true);
	}));
	Made.Add(CreateTexture(Folder + TEXT("T_Brick_N"), Size, Size, [&](int32 X, int32 Y) { return NormalFromHeight(BrickHeight, X, Y); }));
	Made.Add(CreateTexture(Folder + TEXT("T_Metal_D"), Size, Size, [&](int32 X, int32 Y)
	{
		const uint8 V = uint8(150 + 40 * FMath::Sin(X * 0.35f));
		return FColor(V, V, uint8(V + 10), 255);
	}));
	Made.Add(CreateTexture(Folder + TEXT("T_Metal_N"), Size, Size, [&](int32 X, int32 Y) { return NormalFromHeight(MetalHeight, X, Y); }));
	Made.Add(CreateTexture(Folder + TEXT("T_Rock_Normal"), Size, Size, [&](int32 X, int32 Y) { return NormalFromHeight(RockHeight, X, Y); }));
	Made.Add(CreateTexture(Folder + TEXT("T_Mask_R"), Size, Size, [&](int32 X, int32 Y)
	{
		const uint8 V = uint8(X * 255 / (Size - 1));
		return FColor(V, V, V, 255);
	}));

	// "스크립트 실행 전" 상태로 되돌린다: 모두 TC_Default · sRGB · World 그룹 (Python 이 바꿀 대상)
	for (UTexture2D* Tex : Made)
	{
		if (Tex && (Tex->LODGroup != TEXTUREGROUP_World || Tex->CompressionSettings != TC_Default || !Tex->SRGB))
		{
			Tex->PreEditChange(nullptr);
			Tex->LODGroup = TEXTUREGROUP_World;
			Tex->CompressionSettings = TC_Default;
			Tex->SRGB = true;
			Tex->PostEditChange();
			SaveAsset(Tex);
		}
	}
}
#endif

class FGlossaryPatternsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::RegisterBuilder(TEXT("11_Patterns"), &BuildGlossaryPatternsAssets);
		D1110_EditorTool::Register();     // Tools > Glossary > Patterns 메뉴 + Align Tool 창 (에디터 UI 가 있을 때만)
#endif
#if WITH_GAMEPLAY_DEBUGGER
		D1115_DebuggerCategory::Register(); // Gameplay Debugger 'Glossary' 카테고리
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_GAMEPLAY_DEBUGGER
		D1115_DebuggerCategory::Unregister();
#endif
#if WITH_EDITOR
		D1110_EditorTool::Unregister();
		GlossaryAssetKit::UnregisterBuilder(TEXT("11_Patterns"));
#endif
	}
};

IMPLEMENT_MODULE(FGlossaryPatternsModule, GlossaryPatterns);
