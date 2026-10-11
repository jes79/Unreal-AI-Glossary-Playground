// 05_Effects 분야 생성 에셋: 파티클 머티리얼·텍스처 + Niagara System(NS_*) + 포스트 프로세스 머티리얼.
// 전부 에디터 전용 빌더 코드이며 tools/glossary-build.sh -filter=05 로 다시 만든다.
#include "EffectsAssets.h"

#if WITH_EDITOR

#include "EffectsNiagaraAuthoring.h"
#include "GlossaryAssetKit.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionParticleColor.h"
#include "Materials/MaterialExpressionParticleSubUV.h"
#include "Materials/MaterialExpressionSceneTexture.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Misc/CommandLine.h"
#include "NiagaraEmitter.h"
#include "NiagaraSystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogGlossaryEffectsAssets, Log, All);

/** EffectsNiagaraSystems.cpp: NS_* 시스템 전부 */
void BuildNiagaraSystems();

using namespace GlossaryAssetKit;

namespace
{
	const FString Folder = TEXT("/Game/Glossary/05_Effects/");

	/** 파티클 머티리얼 공통: Unlit + 블렌드 모드 + Niagara 사용 플래그 */
	UMaterial* NewParticleMaterial(const FString& Name, EBlendMode Blend)
	{
		UMaterial* M = CreateMaterial(Folder + Name);
		M->BlendMode = Blend;
		M->SetShadingModel(MSM_Unlit);
		M->bUsedWithNiagaraSprites = true;
		M->bUsedWithNiagaraRibbons = true;
		M->bUsedWithNiagaraMeshParticles = true;
		return M;
	}

	/** Custom(HLSL) 노드에 UV(TexCoord) + 파티클 색(RGBA)을 연결한다 */
	UMaterialExpressionCustom* AddUVColorCustom(UMaterial* M, const FString& Code, int32 OutputType)
	{
		auto* UV = AddExpression<UMaterialExpressionTextureCoordinate>(M, -700, -100);
		auto* Color = AddExpression<UMaterialExpressionParticleColor>(M, -700, 100);
		auto* Code_ = AddCustom(M, Code, { TEXT("UV"), TEXT("Color") }, OutputType, TEXT("FX"), -350, 0);
		Connect(UV, TEXT(""), Code_, TEXT("UV"));
		Connect(Color, TEXT("RGBA"), Code_, TEXT("Color"));
		return Code_;
	}

	void BuildParticleMaterials()
	{
		// 1) 가산(Additive) 둥근 입자: 불꽃·불씨·스파크·마법 입자. 겹칠수록 밝아진다
		{
			UMaterial* M = NewParticleMaterial(TEXT("M_FX_Additive"), BLEND_Additive);
			auto* C = AddUVColorCustom(M, TEXT("float d = length(UV - 0.5) * 2.0; float m = saturate(1.0 - d); m = m * m; return Color.rgb * Color.a * m;"), CMOT_Float3);
			ConnectToPin(C, MP_EmissiveColor);
			FinishMaterial(M);
		}
		// 2) 연기: 반투명, 노이즈로 가장자리를 뭉게뭉게
		{
			UMaterial* M = NewParticleMaterial(TEXT("M_FX_Smoke"), BLEND_Translucent);
			UTexture2D* Noise = CreateNoiseTexture(Folder + TEXT("T_FX_Noise"), 128, 4, 4, 7);
			auto* NoiseParam = AddTextureParam(M, TEXT("Noise"), Noise, -700, 300);
			auto* UV = AddExpression<UMaterialExpressionTextureCoordinate>(M, -900, -100);
			auto* Color = AddExpression<UMaterialExpressionParticleColor>(M, -700, 100);
			Connect(UV, TEXT(""), NoiseParam, TEXT("UVs"));
			auto* Op = AddCustom(M, TEXT("float d = length(UV - 0.5) * 2.0; float m = saturate(1.0 - d); return saturate(m * (0.4 + N) * m) * Color.a;"),
				{ TEXT("UV"), TEXT("Color"), TEXT("N") }, CMOT_Float1, TEXT("SmokeOpacity"), -350, 100);
			Connect(UV, TEXT(""), Op, TEXT("UV"));
			Connect(Color, TEXT("RGBA"), Op, TEXT("Color"));
			Connect(NoiseParam, TEXT("R"), Op, TEXT("N"));
			ConnectToPin(Color, MP_EmissiveColor, TEXT("RGB"));
			ConnectToPin(Op, MP_Opacity);
			FinishMaterial(M);
		}
		// 3) 리본·빔: 폭 방향(V)으로 가운데가 밝은 띠
		{
			UMaterial* M = NewParticleMaterial(TEXT("M_FX_Trail"), BLEND_Additive);
			M->TwoSided = true;
			auto* C = AddUVColorCustom(M, TEXT("float m = sin(saturate(UV.y) * 3.14159); m = m * m; return Color.rgb * Color.a * m;"), CMOT_Float3);
			ConnectToPin(C, MP_EmissiveColor);
			FinishMaterial(M);
		}
		// 4) 충격파 링
		{
			UMaterial* M = NewParticleMaterial(TEXT("M_FX_Ring"), BLEND_Additive);
			M->TwoSided = true;
			auto* C = AddUVColorCustom(M, TEXT("float d = length(UV - 0.5) * 2.0; float m = saturate(1.0 - abs(d - 0.85) / 0.12); return Color.rgb * Color.a * m * m;"), CMOT_Float3);
			ConnectToPin(C, MP_EmissiveColor);
			FinishMaterial(M);
		}
		// 5) 메시 파티클용 불투명 Lit 머티리얼 (파티클 색 = Base Color)
		{
			UMaterial* M = CreateMaterial(Folder + TEXT("M_FX_Mesh"));
			M->bUsedWithNiagaraMeshParticles = true;
			M->bUsedWithNiagaraSprites = true;
			auto* Color = AddExpression<UMaterialExpressionParticleColor>(M, -500, 0);
			ConnectToPin(Color, MP_BaseColor, TEXT("RGB"));
			FinishMaterial(M);
		}
	}

	/** 8x8 플립북: 64 프레임 동안 불덩이가 커지며 주황 → 검은 연기로 식는다 (프레임당 64px) */
	UTexture2D* BuildFlipbookTexture()
	{
		constexpr int32 Frames = 8, Cell = 64;
		return CreateTexture(Folder + TEXT("T_FX_Explosion8x8"), Frames * Cell, Frames * Cell, [](int32 X, int32 Y)
		{
			const int32 Frame = (Y / Cell) * Frames + (X / Cell);
			const float T = Frame / 63.f;                         // 0 → 1 진행도
			const float U = (X % Cell + 0.5f) / Cell - 0.5f, V = (Y % Cell + 0.5f) / Cell - 0.5f;
			const float Angle = FMath::Atan2(V, U);
			// 가장자리가 울퉁불퉁한 원: 반지름이 시간에 따라 커진다
			const float Wobble = 0.06f * FMath::Sin(Angle * 7.f + T * 9.f) + 0.04f * FMath::Sin(Angle * 13.f - T * 5.f);
			const float Radius = FMath::Lerp(0.12f, 0.46f, FMath::Sqrt(T)) + Wobble * T;
			const float D = FMath::Sqrt(U * U + V * V);
			const float Inside = FMath::Clamp((Radius - D) / 0.08f, 0.f, 1.f);
			const float Core = FMath::Clamp(1.f - D / FMath::Max(Radius, 0.01f), 0.f, 1.f);
			const float Heat = FMath::Clamp((1.f - T * 1.3f) * (0.4f + Core), 0.f, 1.f);   // 뜨거움: 시간이 갈수록 식는다
			const FLinearColor Hot(1.f, 0.85f, 0.4f), Warm(1.f, 0.35f, 0.05f), Smoke(0.12f, 0.11f, 0.1f);
			FLinearColor C = Heat > 0.5f ? FMath::Lerp(Warm, Hot, (Heat - 0.5f) * 2.f) : FMath::Lerp(Smoke, Warm, Heat * 2.f);
			const float Alpha = Inside * FMath::Clamp(1.2f - T, 0.f, 1.f);
			return FLinearColor(C.R, C.G, C.B, Alpha).ToFColor(true);
		}, true, false);
	}

	/** 플립북을 입자로 재생하는 머티리얼: Particle SubUV 노드가 SubImageIndex 로 칸을 고른다 */
	void BuildFlipbookMaterials(UTexture2D* Sheet)
	{
		{
			UMaterial* M = NewParticleMaterial(TEXT("M_FX_SubUV"), BLEND_Translucent);
			auto* SubUV = AddExpression<UMaterialExpressionParticleSubUV>(M, -700, 0);
			SubUV->Texture = Sheet;
			SubUV->bBlend = true;   // 프레임 사이를 섞어 부드럽게
			SubUV->AutoSetSampleType();
			auto* Color = AddExpression<UMaterialExpressionParticleColor>(M, -700, 250);
			auto* Emissive = AddCustom(M, TEXT("return Tex.rgb * Color.rgb * 4.0;"), { TEXT("Tex"), TEXT("Color") }, CMOT_Float3, TEXT("Emissive"), -350, 0);
			auto* Opacity = AddCustom(M, TEXT("return Tex.a * Color.a;"), { TEXT("Tex"), TEXT("Color") }, CMOT_Float1, TEXT("Opacity"), -350, 200);
			Connect(SubUV, TEXT("RGBA"), Emissive, TEXT("Tex"));
			Connect(Color, TEXT("RGBA"), Emissive, TEXT("Color"));
			Connect(SubUV, TEXT("RGBA"), Opacity, TEXT("Tex"));
			Connect(Color, TEXT("RGBA"), Opacity, TEXT("Color"));
			ConnectToPin(Emissive, MP_EmissiveColor);
			ConnectToPin(Opacity, MP_Opacity);
			FinishMaterial(M);
		}
		// 원본 시트를 판에 그대로 보여 주는 머티리얼 (투명 부분은 어두운 배경)
		{
			UMaterial* M = CreateMaterial(Folder + TEXT("M_FX_SheetPreview"));
			M->SetShadingModel(MSM_Unlit);
			M->TwoSided = true;
			auto* Tex = AddTextureParam(M, TEXT("Sheet"), Sheet, -700, 0);
			auto* Op = AddCustom(M, TEXT("return lerp(float3(0.03, 0.03, 0.04), Tex.rgb * 2.0, Tex.a);"), { TEXT("Tex") }, CMOT_Float3, TEXT("Preview"), -350, 0);
			Connect(Tex, TEXT("RGBA"), Op, TEXT("Tex"));
			ConnectToPin(Op, MP_EmissiveColor);
			FinishMaterial(M);
		}
	}

	/** 0515 발밑 마법진: 시간에 따라 무늬가 맥동하는 가산 원형 문양 (판 자체 회전은 C++) */
	void BuildMagicCircleMaterial()
	{
		UMaterial* M = CreateMaterial(Folder + TEXT("M_FX_MagicCircle"));
		M->BlendMode = BLEND_Additive;
		M->SetShadingModel(MSM_Unlit);
		M->TwoSided = true;
		auto* UV = AddExpression<UMaterialExpressionTextureCoordinate>(M, -800, 0);
		auto* Time = AddExpression<UMaterialExpressionTime>(M, -800, 150);
		auto* Color = AddVectorParam(M, TEXT("Color"), FLinearColor(0.2f, 0.7f, 1.f), -800, 300);
		auto* Code = AddCustom(M, TEXT(
			"float2 p = (UV - 0.5) * 2.0; float r = length(p); float a = atan2(p.y, p.x);\n"
			"float ring1 = saturate(1.0 - abs(r - 0.92) / 0.03);\n"
			"float ring2 = saturate(1.0 - abs(r - 0.70) / 0.02);\n"
			"float ring3 = saturate(1.0 - abs(r - 0.35) / 0.02);\n"
			"float runes = step(0.70, r) * step(r, 0.92) * step(0.6, frac(a * 12.0 / 6.28318)) * 0.6;\n"
			"float star = saturate(1.0 - abs(cos(a * 3.0) * r - 0.30) / 0.02) * step(r, 0.70);\n"
			"float m = ring1 + ring2 + ring3 + runes + star;\n"
			"float pulse = 0.75 + 0.25 * sin(T * 4.0);\n"
			"return C.rgb * m * pulse * 4.0 * step(r, 1.0);"),
			{ TEXT("UV"), TEXT("T"), TEXT("C") }, CMOT_Float3, TEXT("MagicCircle"), -400, 0);
		Connect(UV, TEXT(""), Code, TEXT("UV"));
		Connect(Time, TEXT(""), Code, TEXT("T"));
		Connect(Color, TEXT(""), Code, TEXT("C"));
		ConnectToPin(Code, MP_EmissiveColor);
		FinishMaterial(M);
	}

	/** 0519 Hit Flash: BaseColor 를 FlashAmount 만큼 흰색으로 섞고 같은 양만큼 발광 */
	void BuildHitFlashMaterial()
	{
		UMaterial* M = CreateMaterial(Folder + TEXT("M_HitFlash"));
		auto* Base = AddVectorParam(M, TEXT("BaseColor"), FLinearColor(0.7f, 0.2f, 0.15f), -700, -100);
		auto* Flash = AddScalarParam(M, TEXT("FlashAmount"), 0.f, -700, 100);
		auto* Color = AddCustom(M, TEXT("return lerp(Base.rgb, float3(1,1,1), Flash);"), { TEXT("Base"), TEXT("Flash") }, CMOT_Float3, TEXT("FlashColor"), -350, -100);
		auto* Glow = AddCustom(M, TEXT("return float3(1,1,1) * Flash * 6.0;"), { TEXT("Flash") }, CMOT_Float3, TEXT("FlashGlow"), -350, 100);
		Connect(Base, TEXT(""), Color, TEXT("Base"));
		Connect(Flash, TEXT(""), Color, TEXT("Flash"));
		Connect(Flash, TEXT(""), Glow, TEXT("Flash"));
		ConnectToPin(Color, MP_BaseColor);
		ConnectToPin(Glow, MP_EmissiveColor);
		FinishMaterial(M);
	}

	/** 0529 Post Process 머티리얼: 지금 화면(PostProcessInput0) → 흑백 + 가로 스캔라인 */
	void BuildPostProcessMaterial()
	{
		UMaterial* M = CreateMaterial(Folder + TEXT("M_PP_GrayScanline"));
		M->MaterialDomain = MD_PostProcess;            // 핵심: Post Process 도메인
		M->BlendableLocation = BL_SceneColorAfterTonemapping;
		M->SetShadingModel(MSM_Unlit);
		auto* Scene = AddExpression<UMaterialExpressionSceneTexture>(M, -800, 0);
		Scene->SceneTextureId = PPI_PostProcessInput0;
		auto* UV = AddExpression<UMaterialExpressionTextureCoordinate>(M, -800, 200);
		auto* Lines = AddScalarParam(M, TEXT("Lines"), 270.f, -800, 300);
		auto* Strength = AddScalarParam(M, TEXT("Strength"), 1.f, -800, 400);
		auto* Scroll = AddScalarParam(M, TEXT("ScrollTime"), 0.f, -800, 500);
		auto* Code = AddCustom(M, TEXT(
			"float g = dot(C.rgb, float3(0.299, 0.587, 0.114));\n"
			"float s = 0.72 + 0.28 * sin((UV.y * Lines + Scroll * 2.0) * 6.28318);\n"
			"float3 mono = g * s * float3(0.95, 1.0, 0.95);\n"
			"return lerp(C.rgb, mono, Strength);"),
			{ TEXT("C"), TEXT("UV"), TEXT("Lines"), TEXT("Strength"), TEXT("Scroll") }, CMOT_Float3, TEXT("GrayScanline"), -400, 0);
		Connect(Scene, TEXT("Color"), Code, TEXT("C"));
		Connect(UV, TEXT(""), Code, TEXT("UV"));
		Connect(Lines, TEXT(""), Code, TEXT("Lines"));
		Connect(Strength, TEXT(""), Code, TEXT("Strength"));
		Connect(Scroll, TEXT(""), Code, TEXT("Scroll"));
		ConnectToPin(Code, MP_EmissiveColor);
		FinishMaterial(M);
	}

	/** 0526 석양 톤 LUT: 256x16 (16x16x16 색 큐브를 가로로 펼친 것). 채도 0.7 + 따뜻하게 */
	void BuildSunsetLUT()
	{
		UTexture2D* LUT = CreateTexture(Folder + TEXT("T_LUT_Sunset"), 256, 16, [](int32 X, int32 Y)
		{
			const float R = (X % 16) / 15.f, G = Y / 15.f, B = (X / 16) / 15.f;   // 중립 LUT 좌표
			const float Luma = R * 0.299f + G * 0.587f + B * 0.114f;
			auto Grade = [Luma](float C, float Gain, float Lift) { return FMath::Clamp((Luma + (C - Luma) * 0.7f) * Gain + Lift, 0.f, 1.f); };
			const float OutR = Grade(R, 1.12f, 0.03f), OutG = Grade(G, 0.97f, 0.01f), OutB = Grade(B, 0.78f, 0.f);
			return FColor(FMath::RoundToInt(OutR * 255.f), FMath::RoundToInt(OutG * 255.f), FMath::RoundToInt(OutB * 255.f), 255);
		}, true, false);
		// LUT 텍스처 설정: Color Lookup Table 그룹, 밉맵 없음, 무압축
		LUT->PreEditChange(nullptr);
		LUT->LODGroup = TEXTUREGROUP_ColorLookupTable;
		LUT->MipGenSettings = TMGS_NoMipmaps;
		LUT->CompressionSettings = TC_VectorDisplacementmap;
		LUT->PostEditChange();
		SaveAsset(LUT);
	}

	/** -NSDump: 템플릿 이미터의 모듈·입력 이름을 로그로 출력 (작성용 조사) */
	void DumpTemplates()
	{
		FGlossaryNiagaraBuilder B(Folder + TEXT("NS_Dump"));
		for (const TCHAR* Template : { TEXT("Fountain"), TEXT("SimpleSpriteBurst"), TEXT("OmnidirectionalBurst"), TEXT("DirectionalBurst"),
			TEXT("DynamicBeam"), TEXT("StaticBeam"), TEXT("LocationBasedRibbon"), TEXT("UpwardMeshBurst"), TEXT("HangingParticulates"),
			TEXT("SingleLoopingParticle"), TEXT("Minimal"), TEXT("BlowingParticles"), TEXT("ConfettiBurst"), TEXT("RecycleParticlesInView") })
		{
			const FGuid Id = B.AddEmitter(Template, FName(Template));
			B.DumpStack(Id);
		}
	}
}

void BuildGlossaryEffectsAssets()
{
	BuildParticleMaterials();
	BuildFlipbookMaterials(BuildFlipbookTexture());
	BuildMagicCircleMaterial();
	BuildHitFlashMaterial();
	BuildPostProcessMaterial();
	BuildSunsetLUT();
	BuildNiagaraSystems();
	if (FParse::Param(FCommandLine::Get(), TEXT("NSDump")))
	{
		DumpTemplates();
	}
}

#endif // WITH_EDITOR
