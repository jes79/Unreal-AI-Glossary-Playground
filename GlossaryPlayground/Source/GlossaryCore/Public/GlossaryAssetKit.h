#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR

#include "Materials/MaterialExpression.h"
#include "SceneTypes.h"

class UMaterial;
class UMaterialFunction;
class UMaterialInstanceConstant;
class UMaterialInterface;
class UMaterialExpressionCustom;
class UMaterialExpressionScalarParameter;
class UMaterialExpressionVectorParameter;
class UMaterialExpressionTextureSampleParameter2D;
class USoundWave;
class UTexture;
class UTexture2D;

/**
 * 에디터 전용: 예제 레벨이 쓰는 에셋(머티리얼·텍스처·사운드)을 코드로 만든다. Unity 판의 GlossaryKit.Create* 에 해당.
 * 파일을 들여오지 않고 매번 같은 결과를 다시 만든다(덮어쓰기). 경로 규칙: /Game/Glossary/{NN_분야}/{에셋 이름}.
 *
 * 분야 모듈은 StartupModule 에서 RegisterBuilder("04_Material", &BuildMaterialAssets) 처럼 등록하고,
 * GlossaryBuild 커맨드렛(Tools > Glossary > Build All)이 레벨을 만들기 전에 모두 실행한다.
 */
namespace GlossaryAssetKit
{
	using FBuilderFunc = TFunction<void()>;

	/** 분야 이름(정렬 키)으로 에셋 빌더 등록. 같은 이름이면 덮어쓴다. */
	GLOSSARYCORE_API void RegisterBuilder(const FString& Name, FBuilderFunc Func);
	GLOSSARYCORE_API void UnregisterBuilder(const FString& Name);
	GLOSSARYCORE_API TArray<FString> GetBuilderNames();
	/** Filter 가 비어 있으면 전부, 아니면 이름에 Filter 가 들어간 것만 */
	GLOSSARYCORE_API void RunBuilders(const FString& Filter = FString());

	// ---------------------------------------------------------------- 머티리얼

	/** 새 머티리얼(있으면 노드를 모두 지우고 재사용). PackagePath = "/Game/Glossary/04_Material/M_Dissolve" */
	GLOSSARYCORE_API UMaterial* CreateMaterial(const FString& PackagePath);

	/** 노드 하나 만들기. X,Y 는 그래프 위치(가독성용, 왼쪽이 음수) */
	template <typename T>
	T* AddExpression(UMaterial* Material, int32 X = -400, int32 Y = 0);
	GLOSSARYCORE_API UMaterialExpression* AddExpression(UMaterial* Material, UClass* ExpressionClass, int32 X, int32 Y);

	GLOSSARYCORE_API UMaterialExpressionScalarParameter* AddScalarParam(UMaterial* Material, FName Name, float Default, int32 X = -600, int32 Y = 0);
	GLOSSARYCORE_API UMaterialExpressionVectorParameter* AddVectorParam(UMaterial* Material, FName Name, const FLinearColor& Default, int32 X = -600, int32 Y = 0);
	GLOSSARYCORE_API UMaterialExpressionTextureSampleParameter2D* AddTextureParam(UMaterial* Material, FName Name, UTexture* Default, int32 X = -600, int32 Y = 0);

	/**
	 * Custom 노드(HLSL). Inputs 는 입력 핀 이름 목록이고 순서대로 Connect(…, Custom, 이름) 으로 연결한다.
	 * OutputType: CMOT_Float1/2/3/4. 코드는 return 문을 포함한 함수 본문.
	 */
	GLOSSARYCORE_API UMaterialExpressionCustom* AddCustom(UMaterial* Material, const FString& Code, const TArray<FName>& Inputs,
		int32 OutputType /*ECustomMaterialOutputType*/, const FString& Description = TEXT("Custom"), int32 X = -400, int32 Y = 0);

	/** From 의 출력(FromOutput, 기본 "" = 첫 출력)을 To 의 입력(ToInput, ""=첫 입력)에 연결 */
	GLOSSARYCORE_API bool Connect(UMaterialExpression* From, const FString& FromOutput, UMaterialExpression* To, const FString& ToInput);
	/** From 을 머티리얼 핀(MP_BaseColor, MP_EmissiveColor, MP_OpacityMask …)에 연결 */
	GLOSSARYCORE_API bool ConnectToPin(UMaterialExpression* From, EMaterialProperty Property, const FString& FromOutput = FString());

	/** 컴파일 + 저장. 셰이더 컴파일 오류가 있으면 로그에 남기고 false */
	GLOSSARYCORE_API bool FinishMaterial(UMaterial* Material);

	/** Material Instance Constant 를 만들고 파라미터를 채운다 */
	GLOSSARYCORE_API UMaterialInstanceConstant* CreateMaterialInstance(const FString& PackagePath, UMaterialInterface* Parent,
		const TMap<FName, float>& Scalars = {}, const TMap<FName, FLinearColor>& Vectors = {}, const TMap<FName, UTexture*>& Textures = {});

	/** 빈 Material Function (있으면 노드를 지우고 재사용) */
	GLOSSARYCORE_API UMaterialFunction* CreateMaterialFunction(const FString& PackagePath, const FString& Description);

	// ---------------------------------------------------------------- 텍스처

	/** 픽셀 함수로 텍스처를 만든다. Pixel(x, y) 가 sRGB FColor 를 돌려준다. bSRGB=false 면 데이터(마스크·노이즈)용 */
	GLOSSARYCORE_API UTexture2D* CreateTexture(const FString& PackagePath, int32 Width, int32 Height,
		TFunctionRef<FColor(int32 X, int32 Y)> Pixel, bool bSRGB = true, bool bTile = true);

	/** 타일링되는 값 노이즈(0~1) 텍스처 — 디졸브·연기·왜곡용 */
	GLOSSARYCORE_API UTexture2D* CreateNoiseTexture(const FString& PackagePath, int32 Size = 256, int32 Cells = 8, int32 Octaves = 4, int32 Seed = 1);

	// ---------------------------------------------------------------- 사운드

	/** -1~1 모노 샘플로 USoundWave 를 만든다 (WAV 로 감싸 SoundFactory 로 임포트) */
	GLOSSARYCORE_API USoundWave* CreateSoundWave(const FString& PackagePath, const TArray<float>& Samples, int32 SampleRate = 44100, bool bLooping = false);

	/** 사인파 톤 (Attack/Release 엔벨로프) */
	GLOSSARYCORE_API TArray<float> MakeTone(float Frequency, float Duration, float Volume = 0.5f, int32 SampleRate = 44100, float Attack = 0.01f, float Release = 0.05f);
	/** 화이트 노이즈 버스트 (발소리·폭발·총소리 재료) */
	GLOSSARYCORE_API TArray<float> MakeNoise(float Duration, float Volume = 0.5f, int32 SampleRate = 44100, float Decay = 8.f, int32 Seed = 1);
	/** 음 이름 없이 반음 번호 배열로 단순한 멜로디 (0 = A4 440Hz, -100 = 쉼표) */
	GLOSSARYCORE_API TArray<float> MakeMelody(const TArray<int32>& Semitones, float NoteDuration, float Volume = 0.3f, int32 SampleRate = 44100);

	// ---------------------------------------------------------------- 공통

	/** 패키지를 만들거나 불러온다 */
	GLOSSARYCORE_API UPackage* PreparePackage(const FString& PackagePath);
	/** 에셋 등록 + 디스크에 저장 */
	GLOSSARYCORE_API bool SaveAsset(UObject* Asset);
	/** 새 UObject 에셋 (DataAsset, CurveFloat 등). 있으면 기존 것을 돌려준다 */
	GLOSSARYCORE_API UObject* CreateOrLoadAsset(UClass* Class, const FString& PackagePath);

	template <typename T>
	T* CreateOrLoad(const FString& PackagePath) { return Cast<T>(CreateOrLoadAsset(T::StaticClass(), PackagePath)); }

	template <typename T>
	T* AddExpression(UMaterial* Material, int32 X, int32 Y)
	{
		return Cast<T>(AddExpression(Material, T::StaticClass(), X, Y));
	}
}

#endif // WITH_EDITOR
