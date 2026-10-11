// 04 머티리얼·셰이더 분야의 생성 에셋 (/Game/Glossary/04_Material).
// 모든 머티리얼은 "진짜 노드 그래프"로 만든다 — 에디터에서 열면 용어에 해당하는 노드(Fresnel·Panner·DepthFade·VertexColor·
// Custom·CollectionParameter·MaterialFunctionCall …)가 그대로 보이게 하는 것이 목적이다.
// Custom(HLSL) 노드는 0419(Custom 노드 자체가 용어)에서만 쓴다.

#if WITH_EDITOR

#include "GlossaryAssetKit.h"

#include "Engine/Texture2D.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAbs.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionCollectionParameter.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionDecalLifetimeOpacity.h"
#include "Materials/MaterialExpressionDepthFade.h"
#include "Materials/MaterialExpressionDistance.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionDotProduct.h"
#include "Materials/MaterialExpressionFloor.h"
#include "Materials/MaterialExpressionFrac.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionIf.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionMax.h"
#include "Materials/MaterialExpressionMin.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionNormalize.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionPanner.h"
#include "Materials/MaterialExpressionPower.h"
#include "Materials/MaterialExpressionSaturate.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSceneTexture.h"
#include "Materials/MaterialExpressionScreenPosition.h"
#include "Materials/MaterialExpressionSine.h"
#include "Materials/MaterialExpressionSphereMask.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionViewSize.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialParameterCollection.h"

DEFINE_LOG_CATEGORY_STATIC(LogGlossaryMaterialAssets, Log, All);

using namespace GlossaryAssetKit;

namespace
{
	const TCHAR* Folder = TEXT("/Game/Glossary/04_Material/");
	FString P(const TCHAR* Name) { return FString(Folder) + Name; }

	// ------------------------------------------------------------ 그래프 작성 헬퍼
	// 입력 핀은 FExpressionInput 에 직접 연결한다(이름 오타가 컴파일 오류로 잡힌다).

	template <typename T>
	T* E(UMaterial* M, int32 X, int32 Y) { return AddExpression<T>(M, X, Y); }

	template <typename T>
	T* EF(UMaterialFunction* F, int32 X, int32 Y)
	{
		return Cast<T>(UMaterialEditingLibrary::CreateMaterialExpressionInFunction(F, T::StaticClass(), X, Y));
	}

	void Link(FExpressionInput& In, UMaterialExpression* From, int32 OutputIndex = 0)
	{
		In.Connect(OutputIndex, From);
	}

	/** 머티리얼 출력 핀(Base Color, Emissive …)에 연결 */
	void Pin(UMaterial* M, EMaterialProperty Property, UMaterialExpression* From, int32 OutputIndex = 0)
	{
		if (FExpressionInput* In = M->GetExpressionInputForProperty(Property))
		{
			In->Connect(OutputIndex, From);
		}
	}

	UMaterialExpressionConstant* Const1(UMaterial* M, float V, int32 X, int32 Y)
	{
		auto* C = E<UMaterialExpressionConstant>(M, X, Y);
		C->R = V;
		return C;
	}

	UMaterialExpressionConstant2Vector* Const2(UMaterial* M, float R, float G, int32 X, int32 Y)
	{
		auto* C = E<UMaterialExpressionConstant2Vector>(M, X, Y);
		C->R = R;
		C->G = G;
		return C;
	}

	UMaterialExpressionConstant3Vector* Const3(UMaterial* M, const FLinearColor& V, int32 X, int32 Y)
	{
		auto* C = E<UMaterialExpressionConstant3Vector>(M, X, Y);
		C->Constant = V;
		return C;
	}

	/** A × B (B 가 null 이면 ConstB) */
	UMaterialExpressionMultiply* Mul(UMaterial* M, UMaterialExpression* A, UMaterialExpression* B, int32 X, int32 Y, float ConstB = 1.f, int32 AOut = 0, int32 BOut = 0)
	{
		auto* N = E<UMaterialExpressionMultiply>(M, X, Y);
		Link(N->A, A, AOut);
		if (B) { Link(N->B, B, BOut); } else { N->ConstB = ConstB; }
		return N;
	}

	UMaterialExpressionAdd* Add(UMaterial* M, UMaterialExpression* A, UMaterialExpression* B, int32 X, int32 Y, float ConstB = 0.f, int32 AOut = 0, int32 BOut = 0)
	{
		auto* N = E<UMaterialExpressionAdd>(M, X, Y);
		Link(N->A, A, AOut);
		if (B) { Link(N->B, B, BOut); } else { N->ConstB = ConstB; }
		return N;
	}

	UMaterialExpressionSubtract* Sub(UMaterial* M, UMaterialExpression* A, UMaterialExpression* B, int32 X, int32 Y, float ConstB = 0.f, int32 AOut = 0, int32 BOut = 0)
	{
		auto* N = E<UMaterialExpressionSubtract>(M, X, Y);
		Link(N->A, A, AOut);
		if (B) { Link(N->B, B, BOut); } else { N->ConstB = ConstB; }
		return N;
	}

	UMaterialExpressionDivide* Div(UMaterial* M, UMaterialExpression* A, UMaterialExpression* B, int32 X, int32 Y, float ConstB = 1.f, int32 AOut = 0, int32 BOut = 0)
	{
		auto* N = E<UMaterialExpressionDivide>(M, X, Y);
		Link(N->A, A, AOut);
		if (B) { Link(N->B, B, BOut); } else { N->ConstB = ConstB; }
		return N;
	}

	UMaterialExpressionLinearInterpolate* Lerp(UMaterial* M, UMaterialExpression* A, UMaterialExpression* B, UMaterialExpression* Alpha, int32 X, int32 Y, int32 AlphaOut = 0)
	{
		auto* N = E<UMaterialExpressionLinearInterpolate>(M, X, Y);
		Link(N->A, A);
		Link(N->B, B);
		Link(N->Alpha, Alpha, AlphaOut);
		return N;
	}

	UMaterialExpressionSaturate* Sat(UMaterial* M, UMaterialExpression* In, int32 X, int32 Y, int32 Out = 0)
	{
		auto* N = E<UMaterialExpressionSaturate>(M, X, Y);
		Link(N->Input, In, Out);
		return N;
	}

	UMaterialExpressionComponentMask* Mask(UMaterial* M, UMaterialExpression* In, bool R, bool G, bool B, bool A, int32 X, int32 Y, int32 Out = 0)
	{
		auto* N = E<UMaterialExpressionComponentMask>(M, X, Y);
		Link(N->Input, In, Out);
		N->R = R; N->G = G; N->B = B; N->A = A;
		return N;
	}

	UMaterialExpressionTextureCoordinate* UV(UMaterial* M, float Tiling, int32 X, int32 Y)
	{
		auto* N = E<UMaterialExpressionTextureCoordinate>(M, X, Y);
		N->UTiling = Tiling;
		N->VTiling = Tiling;
		return N;
	}

	UMaterialExpressionTextureSample* Sample(UMaterial* M, UTexture* Texture, UMaterialExpression* Coords, int32 X, int32 Y)
	{
		auto* N = E<UMaterialExpressionTextureSample>(M, X, Y);
		N->Texture = Texture;
		N->AutoSetSampleType();
		if (Coords) { Link(N->Coordinates, Coords); }
		return N;
	}

	/** Material Function Call 의 입력/출력을 이름으로 찾는다 */
	FExpressionInput* FuncInput(UMaterialExpressionMaterialFunctionCall* Call, const TCHAR* Name)
	{
		for (FFunctionExpressionInput& In : Call->FunctionInputs)
		{
			if (In.ExpressionInput && In.ExpressionInput->InputName == FName(Name))
			{
				return &In.Input;
			}
		}
		UE_LOG(LogGlossaryMaterialAssets, Error, TEXT("[Glossary] 함수 입력 없음: %s.%s"), *GetNameSafe(Call->MaterialFunction), Name);
		return nullptr;
	}

	int32 FuncOutput(UMaterialExpressionMaterialFunctionCall* Call, const TCHAR* Name)
	{
		for (int32 I = 0; I < Call->FunctionOutputs.Num(); ++I)
		{
			if (Call->FunctionOutputs[I].Output.OutputName == FName(Name))
			{
				return I;
			}
		}
		UE_LOG(LogGlossaryMaterialAssets, Error, TEXT("[Glossary] 함수 출력 없음: %s.%s"), *GetNameSafe(Call->MaterialFunction), Name);
		return 0;
	}

	void Finish(UMaterial* M)
	{
		const bool bOk = FinishMaterial(M);
		UE_LOG(LogGlossaryMaterialAssets, Display, TEXT("[Glossary] 머티리얼 %s %s"), *M->GetName(), bOk ? TEXT("컴파일 OK") : TEXT("컴파일 오류"));
	}

	// ------------------------------------------------------------ 텍스처 재료 (값 노이즈)

	float Hash(int32 X, int32 Y, int32 Seed)
	{
		uint32 H = static_cast<uint32>(X) * 374761393u + static_cast<uint32>(Y) * 668265263u + static_cast<uint32>(Seed) * 1442695041u;
		H = (H ^ (H >> 13)) * 1274126177u;
		return static_cast<float>((H ^ (H >> 16)) & 0xFFFFFF) / static_cast<float>(0xFFFFFF);
	}

	/** 0~1 타일링 FBM. U,V 는 0~1 */
	float Fbm(float U, float V, int32 Cells, int32 Octaves, int32 Seed)
	{
		float Sum = 0.f, Amp = 0.5f, Total = 0.f;
		for (int32 O = 0; O < Octaves; ++O)
		{
			const int32 Period = Cells << O;
			const float X = U * Period, Y = V * Period;
			const int32 X0 = FMath::FloorToInt(X), Y0 = FMath::FloorToInt(Y);
			const float FX = X - X0, FY = Y - Y0;
			auto At = [&](int32 IX, int32 IY) { return Hash(((IX % Period) + Period) % Period, ((IY % Period) + Period) % Period, Seed + O * 17); };
			const float SX = FX * FX * (3.f - 2.f * FX), SY = FY * FY * (3.f - 2.f * FY);
			Sum += FMath::Lerp(FMath::Lerp(At(X0, Y0), At(X0 + 1, Y0), SX), FMath::Lerp(At(X0, Y0 + 1), At(X0 + 1, Y0 + 1), SX), SY) * Amp;
			Total += Amp;
			Amp *= 0.5f;
		}
		return Sum / Total;
	}

	FColor ToColor(const FLinearColor& C, float Alpha = 1.f)
	{
		FColor Out = FLinearColor(C.R, C.G, C.B).ToFColor(true);
		Out.A = static_cast<uint8>(FMath::Clamp(Alpha, 0.f, 1.f) * 255.f);
		return Out;
	}

	struct FTextures
	{
		UTexture2D* Noise = nullptr;
		UTexture2D* Lava = nullptr;
		UTexture2D* RockGrid = nullptr;
		UTexture2D* Leaf = nullptr;
		UTexture2D* BulletHole = nullptr;
		UTexture2D* Stone = nullptr;
		UTexture2D* Moss = nullptr;
	};

	FTextures BuildTextures()
	{
		FTextures T;
		T.Noise = CreateNoiseTexture(P(TEXT("T_Noise")), 256, 6, 5, 7);

		// 용암: 노이즈 값으로 어두운 껍질 → 주황 → 노랑. 균열(값이 높은 곳)이 밝게 빛난다
		constexpr int32 LavaSize = 256;
		T.Lava = CreateTexture(P(TEXT("T_Lava")), LavaSize, LavaSize, [](int32 X, int32 Y)
		{
			const float U = static_cast<float>(X) / LavaSize, V = static_cast<float>(Y) / LavaSize;
			const float N = Fbm(U, V, 4, 5, 3);
			const float Crack = FMath::Pow(FMath::Clamp(1.f - FMath::Abs(N - 0.5f) * 6.f, 0.f, 1.f), 1.5f);
			const FLinearColor Crust(0.05f, 0.02f, 0.015f), Hot(1.f, 0.28f, 0.02f), White(1.f, 0.85f, 0.3f);
			FLinearColor C = FMath::Lerp(Crust, Hot, FMath::Clamp(Fbm(U, V, 8, 3, 11) * 0.6f + Crack, 0.f, 1.f));
			C = FMath::Lerp(C, White, FMath::Clamp(Crack - 0.6f, 0.f, 1.f) * 2.f);
			return ToColor(C);
		});

		// 바위 격자: 벽돌 줄눈이 또렷해서 UV 가 늘어나면 바로 티가 난다
		constexpr int32 RockSize = 256;
		T.RockGrid = CreateTexture(P(TEXT("T_RockGrid")), RockSize, RockSize, [](int32 X, int32 Y)
		{
			const int32 Row = Y / 32;
			const int32 BX = (X + (Row % 2) * 32) % 64, BY = Y % 32;
			const bool bMortar = BX < 3 || BY < 3;
			const float N = Fbm(static_cast<float>(X) / RockSize, static_cast<float>(Y) / RockSize, 8, 4, 5);
			const float Brick = Hash((X + (Row % 2) * 32) / 64, Row, 9);
			FLinearColor C = bMortar ? FLinearColor(0.08f, 0.08f, 0.08f)
				: FMath::Lerp(FLinearColor(0.35f, 0.3f, 0.25f), FLinearColor(0.55f, 0.5f, 0.42f), Brick) * (0.75f + N * 0.5f);
			return ToColor(C);
		});

		// 잎사귀: 알파 채널이 잎 모양(Masked 블렌드 모드 재료). 잎맥은 밝게, 벌레 먹은 구멍은 알파 0
		constexpr int32 LeafSize = 256;
		T.Leaf = CreateTexture(P(TEXT("T_Leaf")), LeafSize, LeafSize, [](int32 X, int32 Y)
		{
			const float U = (static_cast<float>(X) / LeafSize) * 2.f - 1.f;   // -1~1
			const float V = (static_cast<float>(Y) / LeafSize) * 2.f - 1.f;
			// 위아래가 뾰족한 잎 윤곽: |u| < 폭(v)
			const float Width = 0.62f * FMath::Pow(FMath::Max(0.f, 1.f - V * V), 0.8f);
			const float Edge = Width - FMath::Abs(U) + (Fbm((U + 1.f) * 0.5f, (V + 1.f) * 0.5f, 6, 3, 21) - 0.5f) * 0.12f;
			float Alpha = FMath::Clamp(Edge * 12.f, 0.f, 1.f);
			const float Hole = FMath::Square(U - 0.18f) + FMath::Square(V + 0.25f);
			if (Hole < 0.012f) { Alpha = 0.f; }
			const float Vein = FMath::Clamp(1.f - FMath::Abs(U) * 40.f, 0.f, 1.f)
				+ FMath::Clamp(1.f - FMath::Abs(FMath::Frac((V + FMath::Abs(U) * 0.8f) * 5.f) - 0.5f) * 30.f, 0.f, 1.f) * 0.5f;
			const FLinearColor C = FMath::Lerp(FLinearColor(0.05f, 0.3f, 0.04f), FLinearColor(0.4f, 0.7f, 0.15f), FMath::Clamp(Vein, 0.f, 1.f));
			return ToColor(C, Alpha);
		});

		// 총알 자국: 가운데 검은 구멍 + 그을린 링 + 금. 알파로 가장자리가 사라진다
		constexpr int32 HoleSize = 128;
		T.BulletHole = CreateTexture(P(TEXT("T_BulletHole")), HoleSize, HoleSize, [](int32 X, int32 Y)
		{
			const float U = (static_cast<float>(X) + 0.5f) / HoleSize * 2.f - 1.f;
			const float V = (static_cast<float>(Y) + 0.5f) / HoleSize * 2.f - 1.f;
			const float R = FMath::Sqrt(U * U + V * V);
			const float Angle = FMath::Atan2(V, U);
			const float Crack = FMath::Pow(FMath::Abs(FMath::Sin(Angle * 7.f + 1.3f)), 40.f) * FMath::Clamp(1.f - R, 0.f, 1.f);
			const float Burn = FMath::Clamp(1.f - (R - 0.15f) / 0.6f, 0.f, 1.f);
			float Alpha = FMath::Max(Burn * Burn, Crack);
			FLinearColor C = FMath::Lerp(FLinearColor(0.12f, 0.1f, 0.09f), FLinearColor(0.01f, 0.01f, 0.01f), FMath::Clamp(1.f - R * 2.f, 0.f, 1.f));
			if (R < 0.16f) { C = FLinearColor(0.f, 0.f, 0.f); Alpha = 1.f; }
			return ToColor(C, Alpha);
		});

		// Vertex Color 블렌딩 재료: 돌 바닥과 이끼
		constexpr int32 GroundSize = 256;
		T.Stone = CreateTexture(P(TEXT("T_Stone")), GroundSize, GroundSize, [](int32 X, int32 Y)
		{
			const float U = static_cast<float>(X) / GroundSize, V = static_cast<float>(Y) / GroundSize;
			const float N = Fbm(U, V, 6, 5, 31);
			const float Cell = Fbm(U, V, 3, 1, 33);
			const float Line = FMath::Clamp(1.f - FMath::Abs(Cell - 0.5f) * 40.f, 0.f, 1.f);
			const FLinearColor C = FLinearColor(0.42f, 0.41f, 0.4f) * (0.6f + N * 0.7f) * (1.f - Line * 0.6f);
			return ToColor(C);
		});
		T.Moss = CreateTexture(P(TEXT("T_Moss")), GroundSize, GroundSize, [](int32 X, int32 Y)
		{
			const float U = static_cast<float>(X) / GroundSize, V = static_cast<float>(Y) / GroundSize;
			const float N = Fbm(U, V, 16, 4, 41);
			const FLinearColor C = FMath::Lerp(FLinearColor(0.06f, 0.18f, 0.03f), FLinearColor(0.25f, 0.5f, 0.08f), N);
			return ToColor(C);
		});
		return T;
	}

	// ------------------------------------------------------------ 0401 PBR

	void Build0401()
	{
		// 데모 격자용: 파라미터 3개 (Metallic × Roughness 표를 MID 로 채운다)
		UMaterial* M = CreateMaterial(P(TEXT("M_PBR")));
		Pin(M, MP_BaseColor, AddVectorParam(M, TEXT("BaseColor"), FLinearColor(0.9f, 0.9f, 0.9f), -500, -200));
		Pin(M, MP_Metallic, AddScalarParam(M, TEXT("Metallic"), 0.f, -500, 0));
		Pin(M, MP_Roughness, AddScalarParam(M, TEXT("Roughness"), 0.5f, -500, 150));
		Finish(M);

		// 요청 예문 그대로: Base Color 빨강, Metallic 1, Roughness 0.2 를 상수 노드로
		UMaterial* Red = CreateMaterial(P(TEXT("M_PBR_RedMetal")));
		Pin(Red, MP_BaseColor, Const3(Red, FLinearColor(0.9f, 0.04f, 0.03f), -500, -200));
		Pin(Red, MP_Metallic, Const1(Red, 1.f, -500, 0));
		Pin(Red, MP_Roughness, Const1(Red, 0.2f, -500, 150));
		Finish(Red);
	}

	// ------------------------------------------------------------ 0402 Material Instance

	void Build0402()
	{
		UMaterial* M = CreateMaterial(P(TEXT("M_Base")));
		Pin(M, MP_BaseColor, AddVectorParam(M, TEXT("Color"), FLinearColor(0.7f, 0.7f, 0.7f), -500, -100));
		Pin(M, MP_Roughness, AddScalarParam(M, TEXT("Roughness"), 0.4f, -500, 100));
		Finish(M);

		// 부모 하나 + 색만 다른 인스턴스 5개 (셰이더는 다시 컴파일하지 않는다)
		const TPair<const TCHAR*, FLinearColor> Instances[] = {
			{ TEXT("MI_Base_Red"),    FLinearColor(0.85f, 0.08f, 0.06f) },
			{ TEXT("MI_Base_Orange"), FLinearColor(0.95f, 0.45f, 0.05f) },
			{ TEXT("MI_Base_Green"),  FLinearColor(0.15f, 0.75f, 0.2f) },
			{ TEXT("MI_Base_Blue"),   FLinearColor(0.1f, 0.3f, 0.95f) },
			{ TEXT("MI_Base_Purple"), FLinearColor(0.55f, 0.2f, 0.9f) },
		};
		for (const auto& Pair : Instances)
		{
			CreateMaterialInstance(P(Pair.Key), M, {}, { { TEXT("Color"), Pair.Value } });
		}
	}

	// ------------------------------------------------------------ 0403 Dynamic Material Instance (피격 플래시)

	void Build0403()
	{
		UMaterial* M = CreateMaterial(P(TEXT("M_HitFlash")));
		M->bUsedWithSkeletalMesh = true;   // 마네킹(스켈레탈 메시)에 쓴다
		Pin(M, MP_BaseColor, AddVectorParam(M, TEXT("BaseColor"), FLinearColor(0.35f, 0.35f, 0.38f), -600, -200));
		Pin(M, MP_Roughness, Const1(M, 0.5f, -600, -50));
		// Emissive = FlashColor × FlashAmount × FlashIntensity  — FlashAmount 를 DMI 로 0↔1 바꾼다
		auto* FlashColor = AddVectorParam(M, TEXT("FlashColor"), FLinearColor(1.f, 0.02f, 0.01f), -800, 100);
		auto* FlashAmount = AddScalarParam(M, TEXT("FlashAmount"), 0.f, -800, 250);
		auto* FlashIntensity = AddScalarParam(M, TEXT("FlashIntensity"), 20.f, -800, 350);
		auto* A = Mul(M, FlashColor, FlashAmount, -450, 150);
		Pin(M, MP_EmissiveColor, Mul(M, A, FlashIntensity, -250, 150));
		Finish(M);
	}

	// ------------------------------------------------------------ 0404 Material Parameter Collection

	UMaterialParameterCollection* BuildPlayerCollection()
	{
		UMaterialParameterCollection* MPC = CreateOrLoad<UMaterialParameterCollection>(P(TEXT("MPC_Player")));
		// 이름으로 찾아 기본값만 갱신한다(Id 가 바뀌지 않게 → 다시 빌드해도 참조가 깨지지 않음)
		auto SetVector = [MPC](FName Name, const FLinearColor& Value)
		{
			const int32 Index = MPC->VectorParameters.IndexOfByPredicate([Name](const FCollectionVectorParameter& P) { return P.ParameterName == Name; });
			FCollectionVectorParameter& Param = Index == INDEX_NONE ? MPC->VectorParameters.AddDefaulted_GetRef() : MPC->VectorParameters[Index];
			Param.ParameterName = Name;
			Param.DefaultValue = Value;
		};
		auto SetScalar = [MPC](FName Name, float Value)
		{
			const int32 Index = MPC->ScalarParameters.IndexOfByPredicate([Name](const FCollectionScalarParameter& P) { return P.ParameterName == Name; });
			FCollectionScalarParameter& Param = Index == INDEX_NONE ? MPC->ScalarParameters.AddDefaulted_GetRef() : MPC->ScalarParameters[Index];
			Param.ParameterName = Name;
			Param.DefaultValue = Value;
		};
		SetVector(TEXT("PlayerPosition"), FLinearColor(0.f, 0.f, -100000.f, 0.f));
		SetScalar(TEXT("PushRadius"), 220.f);
		SetScalar(TEXT("PushStrength"), 70.f);
		MPC->PostEditChange();
		SaveAsset(MPC);
		return MPC;
	}

	UMaterialExpressionCollectionParameter* CollectionParam(UMaterial* M, UMaterialParameterCollection* MPC, FName Name, int32 X, int32 Y)
	{
		auto* N = E<UMaterialExpressionCollectionParameter>(M, X, Y);
		N->Collection = MPC;
		N->ParameterName = Name;
		N->ParameterId = MPC->GetParameterId(Name);
		return N;
	}

	void Build0404()
	{
		UMaterialParameterCollection* MPC = BuildPlayerCollection();

		// 풀: 플레이어(MPC.PlayerPosition) 반경 안의 정점을 바깥·아래로 밀어 눌린 것처럼 보이게 한다 (World Position Offset)
		UMaterial* Grass = CreateMaterial(P(TEXT("M_Grass_MPC")));
		Grass->bUsedWithInstancedStaticMeshes = true;
		Grass->TwoSided = true;
		auto* WP = E<UMaterialExpressionWorldPosition>(Grass, -1400, 0);
		auto* PlayerPos = CollectionParam(Grass, MPC, TEXT("PlayerPosition"), -1400, 200);
		auto* Radius = CollectionParam(Grass, MPC, TEXT("PushRadius"), -1400, 350);
		auto* Strength = CollectionParam(Grass, MPC, TEXT("PushStrength"), -1400, 450);
		auto* PlayerXY = Mask(Grass, PlayerPos, true, true, false, false, -1200, 200);
		auto* Delta = Sub(Grass, WP, PlayerXY, -1000, 100, 0.f, 1 /*XY*/);
		auto* Dist = E<UMaterialExpressionDistance>(Grass, -1000, 250);
		Link(Dist->A, WP, 1);
		Link(Dist->B, PlayerXY);
		auto* Falloff = Sat(Grass, E<UMaterialExpressionOneMinus>(Grass, -700, 250), -550, 250);
		Link(Cast<UMaterialExpressionOneMinus>(Falloff->Input.Expression)->Input, Div(Grass, Dist, Radius, -850, 250));
		auto* Height = Sat(Grass, Div(Grass, WP, nullptr, -1000, -150, 55.f, 2 /*Z*/), -850, -150);
		auto* Dir = E<UMaterialExpressionNormalize>(Grass, -850, 100);
		Link(Dir->VectorInput, Delta);
		auto* K = Mul(Grass, Mul(Grass, Falloff, Height, -400, 250), Strength, -250, 300);
		auto* OffsetXY = Mul(Grass, Dir, K, -100, 100);
		auto* OffsetZ = Mul(Grass, K, nullptr, -100, 300, -0.6f);
		auto* Offset = E<UMaterialExpressionAppendVector>(Grass, 80, 200);
		Link(Offset->A, OffsetXY);
		Link(Offset->B, OffsetZ);
		Pin(Grass, MP_WorldPositionOffset, Offset);
		Pin(Grass, MP_BaseColor, Lerp(Grass, Const3(Grass, FLinearColor(0.03f, 0.12f, 0.02f), -600, -350),
			Const3(Grass, FLinearColor(0.3f, 0.65f, 0.1f), -600, -250), Height, -300, -300));
		Pin(Grass, MP_Roughness, Const1(Grass, 0.7f, -300, -150));
		Finish(Grass);

		// 같은 MPC 를 읽는 두 번째 머티리얼: 흙바닥에 PushRadius 링을 그린다 (값 하나를 여러 머티리얼이 공유)
		UMaterial* Soil = CreateMaterial(P(TEXT("M_Soil_MPC")));
		auto* SWP = E<UMaterialExpressionWorldPosition>(Soil, -1200, 0);
		auto* SPlayer = Mask(Soil, CollectionParam(Soil, MPC, TEXT("PlayerPosition"), -1400, 200), true, true, false, false, -1200, 200);
		auto* SRadius = CollectionParam(Soil, MPC, TEXT("PushRadius"), -1200, 350);
		auto* SDist = E<UMaterialExpressionDistance>(Soil, -1000, 100);
		Link(SDist->A, SWP, 1);
		Link(SDist->B, SPlayer);
		auto* Abs = E<UMaterialExpressionAbs>(Soil, -650, 150);
		Link(Abs->Input, Sub(Soil, SDist, SRadius, -800, 150));
		auto* Ring = Sat(Soil, E<UMaterialExpressionOneMinus>(Soil, -350, 150), -200, 150);
		Link(Cast<UMaterialExpressionOneMinus>(Ring->Input.Expression)->Input, Div(Soil, Abs, nullptr, -500, 150, 8.f));
		Pin(Soil, MP_EmissiveColor, Mul(Soil, Const3(Soil, FLinearColor(1.f, 0.75f, 0.1f), -200, 0), Ring, 0, 100, 1.f));
		Pin(Soil, MP_BaseColor, Const3(Soil, FLinearColor(0.12f, 0.08f, 0.05f), -200, -150));
		Pin(Soil, MP_Roughness, Const1(Soil, 0.95f, -200, -50));
		Finish(Soil);
	}

	// ------------------------------------------------------------ 0405 Material Function (림 라이트 재사용)

	UMaterialFunction* BuildRimFunction()
	{
		UMaterialFunction* F = CreateMaterialFunction(P(TEXT("MF_RimLight")), TEXT("Fresnel 로 가장자리를 RimColor × RimIntensity 로 빛나게 한다 (Emissive 에 연결)"));
		auto* RimColor = EF<UMaterialExpressionFunctionInput>(F, -700, -150);
		RimColor->InputName = TEXT("RimColor");
		RimColor->InputType = FunctionInput_Vector3;
		RimColor->PreviewValue = FVector4f(0.2f, 0.6f, 1.f, 0.f);
		RimColor->bUsePreviewValueAsDefault = true;
		RimColor->SortPriority = 0;
		auto* RimPower = EF<UMaterialExpressionFunctionInput>(F, -700, 0);
		RimPower->InputName = TEXT("RimPower");
		RimPower->InputType = FunctionInput_Scalar;
		RimPower->PreviewValue = FVector4f(3.f, 0.f, 0.f, 0.f);
		RimPower->bUsePreviewValueAsDefault = true;
		RimPower->SortPriority = 1;
		auto* RimIntensity = EF<UMaterialExpressionFunctionInput>(F, -700, 150);
		RimIntensity->InputName = TEXT("RimIntensity");
		RimIntensity->InputType = FunctionInput_Scalar;
		RimIntensity->PreviewValue = FVector4f(2.f, 0.f, 0.f, 0.f);
		RimIntensity->bUsePreviewValueAsDefault = true;
		RimIntensity->SortPriority = 2;

		auto* Fresnel = EF<UMaterialExpressionFresnel>(F, -450, 0);
		Fresnel->BaseReflectFraction = 0.f;
		Link(Fresnel->ExponentIn, RimPower);
		auto* Colored = EF<UMaterialExpressionMultiply>(F, -250, -50);
		Link(Colored->A, Fresnel);
		Link(Colored->B, RimColor);
		auto* Scaled = EF<UMaterialExpressionMultiply>(F, -100, 50);
		Link(Scaled->A, Colored);
		Link(Scaled->B, RimIntensity);
		auto* Out = EF<UMaterialExpressionFunctionOutput>(F, 100, 0);
		Out->OutputName = TEXT("Rim");
		Link(Out->A, Scaled);

		UMaterialEditingLibrary::LayoutMaterialFunctionExpressions(F);
		UMaterialEditingLibrary::UpdateMaterialFunction(F, nullptr);
		SaveAsset(F);
		return F;
	}

	void BuildRimMaterial(UMaterialFunction* Rim, const TCHAR* Name, const FLinearColor& Base, float Metallic, float Roughness,
		const FLinearColor& RimColor, float RimPower, float RimIntensity)
	{
		UMaterial* M = CreateMaterial(P(Name));
		M->bUsedWithSkeletalMesh = true;
		Pin(M, MP_BaseColor, Const3(M, Base, -500, -300));
		Pin(M, MP_Metallic, Const1(M, Metallic, -500, -200));
		Pin(M, MP_Roughness, Const1(M, Roughness, -500, -120));
		// 같은 MF_RimLight 를 호출 — 함수 하나를 고치면 세 머티리얼이 함께 바뀐다
		auto* Call = E<UMaterialExpressionMaterialFunctionCall>(M, -300, 100);
		Call->SetMaterialFunction(Rim);
		if (FExpressionInput* In = FuncInput(Call, TEXT("RimColor"))) { Link(*In, Const3(M, RimColor, -600, 0)); }
		if (FExpressionInput* In = FuncInput(Call, TEXT("RimPower"))) { Link(*In, Const1(M, RimPower, -600, 100)); }
		if (FExpressionInput* In = FuncInput(Call, TEXT("RimIntensity"))) { Link(*In, AddScalarParam(M, TEXT("RimIntensity"), RimIntensity, -600, 200)); }
		Pin(M, MP_EmissiveColor, Call, FuncOutput(Call, TEXT("Rim")));
		Finish(M);
	}

	void Build0405()
	{
		UMaterialFunction* Rim = BuildRimFunction();
		BuildRimMaterial(Rim, TEXT("M_Rim_Metal"), FLinearColor(0.75f, 0.75f, 0.78f), 1.f, 0.3f, FLinearColor(1.f, 0.45f, 0.05f), 4.f, 6.f);
		BuildRimMaterial(Rim, TEXT("M_Rim_Stone"), FLinearColor(0.25f, 0.2f, 0.16f), 0.f, 0.9f, FLinearColor(0.2f, 1.f, 0.3f), 3.f, 3.f);
		BuildRimMaterial(Rim, TEXT("M_Rim_Ghost"), FLinearColor(0.05f, 0.03f, 0.1f), 0.f, 0.5f, FLinearColor(0.3f, 0.6f, 1.f), 1.5f, 6.f);
	}

	// ------------------------------------------------------------ 0406 Fresnel

	void Build0406()
	{
		UMaterial* M = CreateMaterial(P(TEXT("M_Fresnel")));
		M->bUsedWithSkeletalMesh = true;
		Pin(M, MP_BaseColor, Const3(M, FLinearColor(0.04f, 0.04f, 0.05f), -500, -300));
		Pin(M, MP_Roughness, Const1(M, 0.45f, -500, -200));
		// Fresnel: 면이 카메라와 비스듬할수록(가장자리) 1 에 가까워진다. Exponent 3
		auto* Fresnel = E<UMaterialExpressionFresnel>(M, -500, 0);
		Fresnel->BaseReflectFraction = 0.f;
		Link(Fresnel->ExponentIn, AddScalarParam(M, TEXT("FresnelExponent"), 3.f, -750, 0));
		auto* Colored = Mul(M, Fresnel, AddVectorParam(M, TEXT("RimColor"), FLinearColor(0.05f, 0.35f, 1.f), -750, 150), -300, 50);
		Pin(M, MP_EmissiveColor, Mul(M, Colored, AddScalarParam(M, TEXT("RimIntensity"), 8.f, -750, 300), -150, 100));
		Finish(M);
	}

	// ------------------------------------------------------------ 0407 Emissive

	void Build0407()
	{
		UMaterial* M = CreateMaterial(P(TEXT("M_Emissive")));
		Pin(M, MP_BaseColor, Const3(M, FLinearColor(0.02f, 0.02f, 0.02f), -500, -200));
		// Emissive Color = 주황 × 20 (1 보다 큰 HDR 값이라 Bloom 이 번진다)
		auto* Color = AddVectorParam(M, TEXT("EmissiveColor"), FLinearColor(1.f, 0.35f, 0.03f), -600, 0);
		auto* Strength = AddScalarParam(M, TEXT("EmissiveStrength"), 20.f, -600, 150);
		Pin(M, MP_EmissiveColor, Mul(M, Color, Strength, -300, 50));
		Finish(M);
	}

	// ------------------------------------------------------------ 0408 Dissolve

	void Build0408(const FTextures& T)
	{
		UMaterial* M = CreateMaterial(P(TEXT("M_Dissolve")));
		M->BlendMode = BLEND_Masked;
		M->TwoSided = true;
		M->OpacityMaskClipValue = 0.5f;
		M->bUsedWithSkeletalMesh = true;
		auto* Noise = AddTextureParam(M, TEXT("NoiseTexture"), T.Noise, -1100, 0);
		Link(Noise->Coordinates, UV(M, 2.f, -1300, 0));
		auto* Amount = AddScalarParam(M, TEXT("DissolveAmount"), 0.f, -1100, 250);
		// 0→1 이 "완전히 보임 → 완전히 사라짐" 이 되도록 -0.1~1.1 로 넓힌다
		auto* Remap = Sub(M, Mul(M, Amount, nullptr, -900, 250, 1.2f), nullptr, -750, 250, 0.1f);
		auto* D = Sub(M, Noise, Remap, -600, 100, 0.f, 1 /*R*/);
		// Opacity Mask = (노이즈 − Amount) + 0.5 → Clip 0.5 아래는 구멍
		Pin(M, MP_OpacityMask, Add(M, D, nullptr, -400, 300, 0.5f));
		// 경계선: 잘린 곳에서 EdgeWidth 이내만 1 → 주황 Emissive
		auto* EdgeWidth = AddScalarParam(M, TEXT("EdgeWidth"), 0.06f, -750, 0);
		auto* Edge = E<UMaterialExpressionOneMinus>(M, -300, 100);
		Link(Edge->Input, Sat(M, Div(M, D, EdgeWidth, -450, 50), -380, 50));
		auto* EdgeColor = AddVectorParam(M, TEXT("EdgeColor"), FLinearColor(1.f, 0.35f, 0.02f), -400, -150);
		auto* EdgeGlow = AddScalarParam(M, TEXT("EdgeGlow"), 25.f, -400, -50);
		Pin(M, MP_EmissiveColor, Mul(M, Mul(M, EdgeColor, EdgeGlow, -200, -100), Edge, -50, 0));
		Pin(M, MP_BaseColor, AddVectorParam(M, TEXT("BaseColor"), FLinearColor(0.3f, 0.32f, 0.38f), -200, -300));
		Pin(M, MP_Metallic, Const1(M, 0.6f, -200, -220));
		Pin(M, MP_Roughness, Const1(M, 0.35f, -200, -180));
		Finish(M);
	}

	// ------------------------------------------------------------ 0409 Hologram

	void Build0409()
	{
		UMaterial* M = CreateMaterial(P(TEXT("M_Hologram")));
		M->BlendMode = BLEND_Translucent;
		M->SetShadingModel(MSM_Unlit);
		M->bUsedWithSkeletalMesh = true;
		auto* WP = E<UMaterialExpressionWorldPosition>(M, -1300, 0);
		auto* Time = E<UMaterialExpressionTime>(M, -1300, 200);
		// 스캔라인: frac(높이 × 밀도 − 시간 × 속도) → 시간이 갈수록 줄무늬가 위로 흐른다
		auto* Phase = Sub(M, Mul(M, WP, AddScalarParam(M, TEXT("ScanDensity"), 0.06f, -1300, 100), -1100, 50, 1.f, 2 /*Z*/),
			Mul(M, Time, AddScalarParam(M, TEXT("ScanSpeed"), 0.8f, -1300, 300), -1100, 250), -950, 150);
		auto* Frac = E<UMaterialExpressionFrac>(M, -800, 150);
		Link(Frac->Input, Phase);
		auto* Scan = E<UMaterialExpressionPower>(M, -650, 150);
		Link(Scan->Base, Frac);
		Scan->ConstExponent = 8.f;
		// 가장자리 Fresnel
		auto* Fresnel = E<UMaterialExpressionFresnel>(M, -650, -100);
		Fresnel->Exponent = 2.5f;
		Fresnel->BaseReflectFraction = 0.f;
		// 깜빡임: 0.85 + 0.15 × sin(시간 × FlickerSpeed)
		auto* Sine = E<UMaterialExpressionSine>(M, -800, 400);
		Link(Sine->Input, Mul(M, Time, AddScalarParam(M, TEXT("FlickerSpeed"), 7.f, -1100, 450), -950, 400));
		auto* Flicker = Add(M, Mul(M, Sine, AddScalarParam(M, TEXT("FlickerAmount"), 0.15f, -800, 500), -650, 400), nullptr, -500, 400, 0.85f);
		auto* Glow = Add(M, Add(M, Mul(M, Fresnel, nullptr, -500, -100, 1.6f), Mul(M, Scan, nullptr, -500, 150, 0.9f), -350, 0), nullptr, -250, 0, 0.12f);
		auto* GlowFlicker = Mul(M, Glow, Flicker, -150, 150);
		auto* Color = AddVectorParam(M, TEXT("HoloColor"), FLinearColor(0.1f, 0.8f, 1.f), -300, -250);
		Pin(M, MP_EmissiveColor, Mul(M, Mul(M, Color, GlowFlicker, 0, -100), AddScalarParam(M, TEXT("HoloIntensity"), 3.f, -150, -250), 150, -150));
		Pin(M, MP_Opacity, Sat(M, Mul(M, GlowFlicker, nullptr, 0, 200, 0.8f), 150, 200));
		Finish(M);
	}

	// ------------------------------------------------------------ 0410 Toon

	void Build0410()
	{
		// 머티리얼 방식: Unlit + Dot(노멀, 라이트 방향) 을 ToonBands(3) 단계로 끊는다
		UMaterial* M = CreateMaterial(P(TEXT("M_Toon")));
		M->SetShadingModel(MSM_Unlit);
		M->bUsedWithSkeletalMesh = true;
		auto* Normal =E<UMaterialExpressionVertexNormalWS>(M, -1300, -100);
		auto* LightDir = E<UMaterialExpressionNormalize>(M, -1100, 50);
		Link(LightDir->VectorInput, Mask(M, AddVectorParam(M, TEXT("LightDirection"), FLinearColor(0.4f, -0.4f, 0.8f), -1500, 50), true, true, true, false, -1300, 50));
		auto* Dot = E<UMaterialExpressionDotProduct>(M, -950, 0);
		Link(Dot->A, Normal);
		Link(Dot->B, LightDir);
		auto* Bands = AddScalarParam(M, TEXT("ToonBands"), 3.f, -950, 200);
		auto* Floor = E<UMaterialExpressionFloor>(M, -650, 50);
		Link(Floor->Input, Mul(M, Sat(M, Dot, -800, 0), Bands, -700, 0));
		auto* Band = Sat(M, Div(M, Floor, Sub(M, Bands, nullptr, -650, 200, 1.f), -500, 100), -380, 100);
		auto* Shade = Lerp(M, AddScalarParam(M, TEXT("ShadowTone"), 0.3f, -500, 250), Const1(M, 1.f, -500, 350), Band, -250, 200);
		auto* Base = AddVectorParam(M, TEXT("BaseColor"), FLinearColor(0.9f, 0.3f, 0.2f), -300, -150);
		Pin(M, MP_EmissiveColor, Mul(M, Mul(M, Base, Shade, -100, 0), AddScalarParam(M, TEXT("Brightness"), 1.6f, -250, 350), 50, 100));
		Finish(M);

		// 포스트 프로세스 방식: 화면 전체 밝기를 ToonBands 단계로 양자화(색조는 유지)
		UMaterial* PP = CreateMaterial(P(TEXT("M_ToonPostProcess")));
		PP->MaterialDomain = MD_PostProcess;
		PP->BlendableLocation = BL_SceneColorAfterTonemapping;
		auto* Scene = E<UMaterialExpressionSceneTexture>(PP, -1200, 0);
		Scene->SceneTextureId = PPI_PostProcessInput0;
		auto* RGB = Mask(PP, Scene, true, true, true, false, -1000, 0);
		auto* Lum = E<UMaterialExpressionDotProduct>(PP, -850, 100);
		Link(Lum->A, RGB);
		Link(Lum->B, Const3(PP, FLinearColor(0.299f, 0.587f, 0.114f), -1000, 200));
		auto* PPBands = AddScalarParam(PP, TEXT("ToonBands"), 4.f, -850, 300);
		auto* PPFloor = E<UMaterialExpressionFloor>(PP, -550, 150);
		Link(PPFloor->Input, Add(PP, Mul(PP, Lum, PPBands, -700, 150), nullptr, -620, 150, 0.5f));
		auto* Quant = Div(PP, PPFloor, PPBands, -450, 200);
		auto* MaxLum = E<UMaterialExpressionMax>(PP, -550, 300);
		Link(MaxLum->A, Lum);
		MaxLum->ConstB = 0.001f;
		Pin(PP, MP_EmissiveColor, Mul(PP, RGB, Div(PP, Quant, MaxLum, -300, 250), -150, 100));
		Finish(PP);
	}

	// ------------------------------------------------------------ 0411 Outline (Custom Depth)

	void Build0411()
	{
		UMaterial* M = CreateMaterial(P(TEXT("M_OutlinePostProcess")));
		M->MaterialDomain = MD_PostProcess;
		M->BlendableLocation = BL_SceneColorAfterTonemapping;
		auto* ScreenUV = E<UMaterialExpressionScreenPosition>(M, -1700, 0);
		auto* ViewSize = E<UMaterialExpressionViewSize>(M, -1700, 200);
		// 한 픽셀 × OutlineThickness 를 UV 로
		auto* Pixel = Div(M, AddScalarParam(M, TEXT("OutlineThickness"), 3.f, -1700, 300), ViewSize, -1500, 250);

		auto SampleDepth = [&](UMaterialExpression* Coords, int32 Y) -> UMaterialExpression*
		{
			auto* Tex = E<UMaterialExpressionSceneTexture>(M, -1000, Y);
			Tex->SceneTextureId = PPI_CustomDepth;   // Render CustomDepth Pass 를 켠 물체만 들어 있는 깊이
			if (Coords) { Link(Tex->Coordinates, Coords); }
			return Mask(M, Tex, true, false, false, false, -850, Y);
		};
		auto Offset = [&](float DX, float DY, int32 Y)
		{
			return Add(M, ScreenUV, Mul(M, Pixel, Const2(M, DX, DY, -1500, Y), -1300, Y), -1150, Y);
		};
		UMaterialExpression* Center = SampleDepth(ScreenUV, -200);
		auto* MinA = E<UMaterialExpressionMin>(M, -650, 50);
		Link(MinA->A, SampleDepth(Offset(1.f, 0.f, 0), 0));
		Link(MinA->B, SampleDepth(Offset(-1.f, 0.f, 120), 120));
		auto* MinB = E<UMaterialExpressionMin>(M, -650, 300);
		Link(MinB->A, SampleDepth(Offset(0.f, 1.f, 240), 240));
		Link(MinB->B, SampleDepth(Offset(0.f, -1.f, 360), 360));
		auto* Neighbor = E<UMaterialExpressionMin>(M, -500, 150);
		Link(Neighbor->A, MinA);
		Link(Neighbor->B, MinB);

		// 외곽선 픽셀 = 이웃 중 하나는 물체(깊이 < Threshold) 이고 자기 자신은 빈 곳
		auto* Threshold = AddScalarParam(M, TEXT("DepthThreshold"), 1000000.f, -650, 500);
		auto* One = Const1(M, 1.f, -650, 600);
		auto* Zero = Const1(M, 0.f, -650, 650);
		auto* NearObject = E<UMaterialExpressionIf>(M, -350, 150);
		Link(NearObject->A, Neighbor);
		Link(NearObject->B, Threshold);
		Link(NearObject->AGreaterThanB, Zero);
		Link(NearObject->AEqualsB, Zero);
		Link(NearObject->ALessThanB, One);
		auto* CenterEmpty = E<UMaterialExpressionIf>(M, -350, -150);
		Link(CenterEmpty->A, Center);
		Link(CenterEmpty->B, Threshold);
		Link(CenterEmpty->AGreaterThanB, One);
		Link(CenterEmpty->AEqualsB, One);
		Link(CenterEmpty->ALessThanB, Zero);
		auto* Edge = Mul(M, NearObject, CenterEmpty, -200, 0);

		auto* Scene = E<UMaterialExpressionSceneTexture>(M, -400, -350);
		Scene->SceneTextureId = PPI_PostProcessInput0;
		auto* SceneRGB = Mask(M, Scene, true, true, true, false, -250, -350);
		auto* Color = AddVectorParam(M, TEXT("OutlineColor"), FLinearColor(1.f, 0.85f, 0.f), -250, -250);
		Pin(M, MP_EmissiveColor, Lerp(M, SceneRGB, Color, Edge, 0, -200));
		Finish(M);
	}

	// ------------------------------------------------------------ 0412 World Position Offset

	void Build0412()
	{
		// 풀: WPO = sin(Time × WindFrequency + 위치) × WindStrength × 높이² → 뿌리는 고정, 끝만 흔들린다
		UMaterial* Grass = CreateMaterial(P(TEXT("M_WindGrass")));
		Grass->bUsedWithInstancedStaticMeshes = true;
		Grass->TwoSided = true;
		auto* WP = E<UMaterialExpressionWorldPosition>(Grass, -1300, 0);
		auto* Time = E<UMaterialExpressionTime>(Grass, -1300, 200);
		auto* Phase = Add(Grass, Mul(Grass, Time, AddScalarParam(Grass, TEXT("WindFrequency"), 0.6f, -1300, 300), -1100, 200),
			Mul(Grass, Mask(Grass, WP, true, false, false, false, -1100, 0), nullptr, -950, 0, 0.0015f), -850, 100);
		auto* Sine = E<UMaterialExpressionSine>(Grass, -700, 100);   // Period 1 → sin(2π × 입력)
		Link(Sine->Input, Phase);
		auto* Height = E<UMaterialExpressionPower>(Grass, -700, -100);
		Link(Height->Base, Sat(Grass, Div(Grass, WP, nullptr, -1000, -150, 100.f, 2 /*Z*/), -850, -150));
		Height->ConstExponent = 2.f;
		auto* Amount = Mul(Grass, Mul(Grass, Sine, AddScalarParam(Grass, TEXT("WindStrength"), 25.f, -700, 250), -550, 150), Height, -400, 50);
		Pin(Grass, MP_WorldPositionOffset, Mul(Grass, Amount, Const3(Grass, FLinearColor(1.f, 0.35f, 0.f), -400, 250), -250, 100));
		Pin(Grass, MP_BaseColor, Lerp(Grass, Const3(Grass, FLinearColor(0.03f, 0.12f, 0.02f), -400, -350),
			Const3(Grass, FLinearColor(0.35f, 0.7f, 0.12f), -400, -250), Height, -200, -300));
		Pin(Grass, MP_Roughness, Const1(Grass, 0.7f, -200, -150));
		Finish(Grass);

		// 깃발: U(깃대에서 멀어질수록 0→1) 만큼 Y 방향으로 물결. 프로시저럴 메시(정점이 많은 격자)에 쓴다
		UMaterial* Flag = CreateMaterial(P(TEXT("M_Flag")));
		Flag->TwoSided = true;
		auto* FUV = UV(Flag, 1.f, -1300, 0);
		auto* U = Mask(Flag, FUV, true, false, false, false, -1100, 0);
		auto* V = Mask(Flag, FUV, false, true, false, false, -1100, 300);
		auto* FTime = E<UMaterialExpressionTime>(Flag, -1300, 150);
		auto* FPhase = Sub(Flag, Mul(Flag, FTime, AddScalarParam(Flag, TEXT("WindFrequency"), 1.2f, -1300, 250), -1000, 150), Mul(Flag, U, nullptr, -1000, 50, 2.f), -850, 100);
		auto* FSine = E<UMaterialExpressionSine>(Flag, -700, 100);
		Link(FSine->Input, FPhase);
		auto* FAmount = Mul(Flag, Mul(Flag, FSine, AddScalarParam(Flag, TEXT("WindStrength"), 25.f, -700, 250), -550, 150), U, -400, 100);
		// 깃발 면의 노멀 방향으로 밀어낸다 (깃발이 어느 방향을 보든 동작)
		Pin(Flag, MP_WorldPositionOffset, Mul(Flag, FAmount, E<UMaterialExpressionVertexNormalWS>(Flag, -400, 250), -250, 150));
		// 가로 줄무늬 (floor(frac(V×2)×2) = 0/1)
		auto* Stripe = E<UMaterialExpressionFloor>(Flag, -550, 300);
		auto* StripeFrac = E<UMaterialExpressionFrac>(Flag, -800, 300);
		Link(StripeFrac->Input, Mul(Flag, V, nullptr, -950, 300, 2.5f));
		Link(Stripe->Input, Mul(Flag, StripeFrac, nullptr, -680, 300, 2.f));
		Pin(Flag, MP_BaseColor, Lerp(Flag, Const3(Flag, FLinearColor(0.8f, 0.05f, 0.04f), -400, 350), Const3(Flag, FLinearColor(0.9f, 0.9f, 0.85f), -400, 450), Stripe, -200, 350));
		Pin(Flag, MP_Roughness, Const1(Flag, 0.8f, -200, 500));
		Finish(Flag);
	}

	// ------------------------------------------------------------ 0413 Panner

	void Build0413(const FTextures& T)
	{
		UMaterial* M = CreateMaterial(P(TEXT("M_Lava")));
		// Panner: UV 에 Time × Speed 를 더한다. Speed = (PanSpeed, 0) → 초당 0.1 UV 만큼 흐름
		auto* Panner = E<UMaterialExpressionPanner>(M, -800, 0);
		Link(Panner->Coordinate, UV(M, 2.f, -1000, -50));
		auto* Speed = E<UMaterialExpressionAppendVector>(M, -1000, 100);
		Link(Speed->A, AddScalarParam(M, TEXT("PanSpeed"), 0.1f, -1200, 100));
		Link(Speed->B, Const1(M, 0.f, -1200, 200));
		Link(Panner->Speed, Speed);
		auto* Lava = Sample(M, T.Lava, Panner, -600, 0);
		// 두 번째 Panner(느린 노이즈)로 밝기가 일렁이게
		auto* Panner2 = E<UMaterialExpressionPanner>(M, -800, 300);
		Link(Panner2->Coordinate, UV(M, 1.3f, -1000, 300));
		Panner2->SpeedX = 0.03f;
		Panner2->SpeedY = 0.05f;
		auto* Pulse = Sample(M, T.Noise, Panner2, -600, 300);
		auto* PulseScale = Lerp(M, Const1(M, 0.5f, -450, 400), Const1(M, 1.4f, -450, 450), Pulse, -350, 350, 1 /*R*/);
		auto* Glow = Mul(M, Mul(M, Lava, PulseScale, -250, 150), AddScalarParam(M, TEXT("LavaGlow"), 6.f, -400, 250), -100, 150);
		Pin(M, MP_EmissiveColor, Glow);
		Pin(M, MP_BaseColor, Mul(M, Lava, nullptr, -250, -100, 0.3f));
		Pin(M, MP_Roughness, Const1(M, 0.6f, -250, -200));
		Finish(M);
	}

	// ------------------------------------------------------------ 0414 World Aligned Texture

	void Build0414(const FTextures& T)
	{
		// 비교용: 메시 UV 그대로 (큰 바위일수록 텍스처가 늘어난다)
		UMaterial* UVMat = CreateMaterial(P(TEXT("M_Rock_UV")));
		Pin(UVMat, MP_BaseColor, Sample(UVMat, T.RockGrid, UV(UVMat, 1.f, -700, 0), -500, 0));
		Pin(UVMat, MP_Roughness, Const1(UVMat, 0.85f, -500, 200));
		Finish(UVMat);

		// 엔진 Material Function "WorldAlignedTexture" 호출: 월드 좌표로 X·Y·Z 세 방향 투영(트라이플래너)
		UMaterial* M = CreateMaterial(P(TEXT("M_Rock_WorldAligned")));
		UMaterialFunction* WorldAligned = LoadObject<UMaterialFunction>(nullptr,
			TEXT("/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedTexture.WorldAlignedTexture"));
		auto* Tex = E<UMaterialExpressionTextureObjectParameter>(M, -800, -100);
		Tex->ParameterName = TEXT("RockTexture");
		Tex->Texture = T.RockGrid;
		Tex->AutoSetSampleType();
		auto* Call = E<UMaterialExpressionMaterialFunctionCall>(M, -450, 0);
		if (WorldAligned && Call->SetMaterialFunction(WorldAligned))
		{
			if (FExpressionInput* In = FuncInput(Call, TEXT("TextureObject"))) { Link(*In, Tex); }
			auto* Size = Mask(M, AddVectorParam(M, TEXT("TextureSize"), FLinearColor(150.f, 150.f, 150.f), -900, 100), true, true, true, false, -700, 100);
			if (FExpressionInput* In = FuncInput(Call, TEXT("TextureSize"))) { Link(*In, Size); }
			Pin(M, MP_BaseColor, Call, FuncOutput(Call, TEXT("XYZ Texture")));
		}
		else
		{
			UE_LOG(LogGlossaryMaterialAssets, Error, TEXT("[Glossary] WorldAlignedTexture 함수를 찾지 못함"));
		}
		Pin(M, MP_Roughness, Const1(M, 0.85f, -300, 200));
		Finish(M);
	}

	// ------------------------------------------------------------ 0415 Blend Mode

	void Build0415(const FTextures& T)
	{
		UMaterial* Opaque = CreateMaterial(P(TEXT("M_Blend_Opaque")));
		Pin(Opaque, MP_BaseColor, Sample(Opaque, T.RockGrid, UV(Opaque, 1.f, -700, 0), -500, 0));
		Finish(Opaque);

		// Masked: 알파가 Clip 값(0.33)보다 낮은 곳은 아예 그리지 않는다 (반투명 정렬 문제 없음)
		UMaterial* Masked = CreateMaterial(P(TEXT("M_Blend_Masked")));
		Masked->BlendMode = BLEND_Masked;
		Masked->OpacityMaskClipValue = 0.33f;
		Masked->TwoSided = true;
		auto* Leaf = Sample(Masked, T.Leaf, UV(Masked, 1.f, -700, 0), -500, 0);
		Pin(Masked, MP_BaseColor, Leaf, 0);
		Pin(Masked, MP_OpacityMask, Leaf, 4 /*A*/);
		Pin(Masked, MP_Roughness, Const1(Masked, 0.6f, -300, 200));
		Finish(Masked);

		// Translucent: 뒤가 비치는 유리 (Opacity 0.25, 표면 반사)
		UMaterial* Glass = CreateMaterial(P(TEXT("M_Blend_Translucent")));
		Glass->BlendMode = BLEND_Translucent;
		Glass->TranslucencyLightingMode = TLM_SurfacePerPixelLighting;
		Pin(Glass, MP_BaseColor, Const3(Glass, FLinearColor(0.6f, 0.85f, 1.f), -500, -100));
		Pin(Glass, MP_Roughness, Const1(Glass, 0.05f, -500, 0));
		Pin(Glass, MP_Specular, Const1(Glass, 1.f, -500, 80));
		Pin(Glass, MP_Opacity, AddScalarParam(Glass, TEXT("GlassOpacity"), 0.25f, -500, 200));
		Finish(Glass);

		// Additive: 뒤 색에 더하기만 한다(검정 = 투명). 불빛·광륜
		UMaterial* Additive = CreateMaterial(P(TEXT("M_Blend_Additive")));
		Additive->BlendMode = BLEND_Additive;
		Additive->SetShadingModel(MSM_Unlit);
		Additive->TwoSided = true;
		auto* Sphere = E<UMaterialExpressionSphereMask>(Additive, -500, 0);
		Link(Sphere->A, UV(Additive, 1.f, -700, -50));
		Link(Sphere->B, Const2(Additive, 0.5f, 0.5f, -700, 50));
		Sphere->AttenuationRadius = 0.5f;
		Sphere->HardnessPercent = 0.f;
		auto* Soft = E<UMaterialExpressionPower>(Additive, -350, 0);
		Link(Soft->Base, Sphere);
		Soft->ConstExponent = 2.f;
		Pin(Additive, MP_EmissiveColor, Mul(Additive, Mul(Additive, Soft, AddVectorParam(Additive, TEXT("GlowColor"), FLinearColor(1.f, 0.5f, 0.1f), -500, 150), -200, 50),
			AddScalarParam(Additive, TEXT("GlowIntensity"), 6.f, -350, 250), -50, 100));
		Finish(Additive);
	}

	// ------------------------------------------------------------ 0416 Decal

	void Build0416(const FTextures& T)
	{
		UMaterial* M = CreateMaterial(P(TEXT("M_BulletHoleDecal")));
		M->MaterialDomain = MD_DeferredDecal;   // Deferred Decal: GBuffer 에 직접 그린다
		M->BlendMode = BLEND_Translucent;
		auto* Tex = Sample(M, T.BulletHole, UV(M, 1.f, -800, 0), -600, 0);
		Pin(M, MP_BaseColor, Tex, 0);
		Pin(M, MP_Roughness, Const1(M, 0.9f, -300, 100));
		// Decal Lifetime Opacity: UDecalComponent::SetFadeOut 의 페이드가 이 값으로 들어온다(1→0)
		auto* Lifetime = E<UMaterialExpressionDecalLifetimeOpacity>(M, -600, 250);
		Pin(M, MP_Opacity, Mul(M, Tex, Lifetime, -300, 250, 1.f, 4 /*A*/));
		Finish(M);
	}

	// ------------------------------------------------------------ 0417 Vertex Color

	void Build0417(const FTextures& T)
	{
		UMaterial* M = CreateMaterial(P(TEXT("M_VertexColorBlend")));
		auto* Coords = UV(M, 6.f, -1000, 0);
		auto* Stone = Sample(M, T.Stone, Coords, -750, -150);
		auto* Moss = Sample(M, T.Moss, Coords, -750, 100);
		// Vertex Color R 채널(0=돌, 1=이끼)로 Lerp
		auto* VColor = E<UMaterialExpressionVertexColor>(M, -750, 350);
		auto* Blend = Lerp(M, Stone, Moss, VColor, -450, 0, 1 /*R*/);
		// 디버그: ShowVertexColor 1 이면 정점 색 자체를 보여 준다
		auto* Debug = Lerp(M, Blend, VColor, AddScalarParam(M, TEXT("ShowVertexColor"), 0.f, -450, 300), -250, 100);
		Pin(M, MP_BaseColor, Debug);
		Pin(M, MP_Roughness, Lerp(M, Const1(M, 0.6f, -450, 450), Const1(M, 0.95f, -450, 500), VColor, -250, 450, 1));
		Finish(M);
	}

	// ------------------------------------------------------------ 0418 Depth Fade

	void Build0418(const FTextures& T)
	{
		UMaterial* M = CreateMaterial(P(TEXT("M_Water_DepthFade")));
		M->BlendMode = BLEND_Translucent;
		M->TranslucencyLightingMode = TLM_SurfacePerPixelLighting;
		// Depth Fade: 물 표면과 뒤 지형의 깊이 차가 FadeDistance(100cm) 보다 작을수록 Opacity → 0
		auto* Fade = E<UMaterialExpressionDepthFade>(M, -400, 200);
		Link(Fade->InOpacity, AddScalarParam(M, TEXT("WaterOpacity"), 0.85f, -650, 150));
		Link(Fade->FadeDistance, AddScalarParam(M, TEXT("FadeDistance"), 100.f, -650, 250));
		Pin(M, MP_Opacity, Fade);
		// 물결 느낌: 느린 Panner 노이즈로 색을 살짝 바꾼다
		auto* Panner = E<UMaterialExpressionPanner>(M, -900, -100);
		Link(Panner->Coordinate, UV(M, 4.f, -1100, -100));
		Panner->SpeedX = 0.02f;
		Panner->SpeedY = 0.015f;
		auto* Noise = Sample(M, T.Noise, Panner, -700, -100);
		Pin(M, MP_BaseColor, Lerp(M, AddVectorParam(M, TEXT("DeepColor"), FLinearColor(0.01f, 0.12f, 0.2f), -500, -300),
			AddVectorParam(M, TEXT("ShallowColor"), FLinearColor(0.08f, 0.45f, 0.5f), -500, -200), Noise, -250, -200, 1));
		Pin(M, MP_Roughness, Const1(M, 0.05f, -250, 0));
		Pin(M, MP_Specular, Const1(M, 1.f, -250, 80));
		Finish(M);
	}

	// ------------------------------------------------------------ 0419 Custom 노드 (HLSL)

	void Build0419()
	{
		UMaterial* M = CreateMaterial(P(TEXT("M_CustomChecker")));
		// HLSL 함수 본문: UV × 칸 수의 정수 칸 좌표 합이 짝수/홀수인지로 0/1
		auto* Checker = AddCustom(M,
			TEXT("// 체커보드: 칸 좌표 (x+y) 의 홀짝\n")
			TEXT("float2 Cell = floor(UV * Cells);\n")
			TEXT("return fmod(Cell.x + Cell.y, 2.0);"),
			{ TEXT("UV"), TEXT("Cells") }, CMOT_Float1, TEXT("Checkerboard"), -500, 0);
		Link(Checker->Inputs[0].Input, UV(M, 1.f, -800, -50));
		Link(Checker->Inputs[1].Input, AddScalarParam(M, TEXT("CellCount"), 8.f, -800, 100));
		Pin(M, MP_BaseColor, Lerp(M, AddVectorParam(M, TEXT("ColorA"), FLinearColor(0.9f, 0.9f, 0.88f), -300, -200),
			AddVectorParam(M, TEXT("ColorB"), FLinearColor(0.05f, 0.08f, 0.15f), -300, -100), Checker, -100, -50));
		Pin(M, MP_Roughness, Const1(M, 0.5f, -100, 150));
		Finish(M);
	}

	// ------------------------------------------------------------ 0420 Refraction (열 아지랑이)

	void Build0420(const FTextures& T)
	{
		UMaterial* M = CreateMaterial(P(TEXT("M_HeatHaze")));
		M->BlendMode = BLEND_Translucent;
		M->SetShadingModel(MSM_Unlit);
		M->TwoSided = true;
		M->RefractionMethod = RM_2DOffset;   // Refraction 입력 = 화면 공간 2D 오프셋
		M->bUsesDistortion = true;
		// 위로 흐르는 노이즈 두 장 (Panner) → (x, y) 오프셋
		auto* Coords = UV(M, 2.f, -1300, 0);
		auto* PanA = E<UMaterialExpressionPanner>(M, -1100, -100);
		Link(PanA->Coordinate, Coords);
		PanA->SpeedX = 0.05f;
		PanA->SpeedY = 0.6f;
		auto* PanB = E<UMaterialExpressionPanner>(M, -1100, 150);
		Link(PanB->Coordinate, Coords);
		PanB->SpeedX = -0.07f;
		PanB->SpeedY = 0.45f;
		auto* NA = Sample(M, T.Noise, PanA, -900, -100);
		auto* NB = Sample(M, T.Noise, PanB, -900, 150);
		auto* Offset = E<UMaterialExpressionAppendVector>(M, -700, 0);
		Link(Offset->A, NA, 1);
		Link(Offset->B, NB, 1);
		auto* Centered = Sub(M, Offset, nullptr, -550, 0, 0.5f);
		// 가장자리로 갈수록 약하게 (평면 테두리가 보이지 않게)
		auto* Edge = E<UMaterialExpressionSphereMask>(M, -700, 300);
		Link(Edge->A, UV(M, 1.f, -900, 300));
		Link(Edge->B, Const2(M, 0.5f, 0.5f, -900, 400));
		Edge->AttenuationRadius = 0.5f;
		Edge->HardnessPercent = 0.f;
		auto* Strength = AddScalarParam(M, TEXT("HazeStrength"), 0.03f, -550, 200);
		Pin(M, MP_Refraction, Mul(M, Mul(M, Centered, Strength, -400, 100), Edge, -250, 150));
		Pin(M, MP_Opacity, Const1(M, 0.f, -250, -50));
		Pin(M, MP_EmissiveColor, Const3(M, FLinearColor::Black, -250, -150));
		Finish(M);
	}
}

void BuildGlossaryMaterialAssets()
{
	const FTextures Textures = BuildTextures();
	Build0401();
	Build0402();
	Build0403();
	Build0404();
	Build0405();
	Build0406();
	Build0407();
	Build0408(Textures);
	Build0409();
	Build0410();
	Build0411();
	Build0412();
	Build0413(Textures);
	Build0414(Textures);
	Build0415(Textures);
	Build0416(Textures);
	Build0417(Textures);
	Build0418(Textures);
	Build0419();
	Build0420(Textures);
}

#endif // WITH_EDITOR
