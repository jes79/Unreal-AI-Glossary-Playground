#include "GlossaryAssetKit.h"

#if WITH_EDITOR

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "Factories/MaterialFactoryNew.h"
#include "Factories/MaterialFunctionFactoryNew.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Factories/SoundFactory.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/PackageName.h"
#include "Sound/SoundWave.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogGlossaryAssets, Log, All);

namespace GlossaryAssetKit
{
	static TMap<FString, FBuilderFunc>& Builders()
	{
		static TMap<FString, FBuilderFunc> Map;
		return Map;
	}

	void RegisterBuilder(const FString& Name, FBuilderFunc Func)
	{
		Builders().Add(Name, MoveTemp(Func));
	}

	void UnregisterBuilder(const FString& Name)
	{
		Builders().Remove(Name);
	}

	TArray<FString> GetBuilderNames()
	{
		TArray<FString> Names;
		Builders().GetKeys(Names);
		Names.Sort();
		return Names;
	}

	void RunBuilders(const FString& Filter)
	{
		for (const FString& Name : GetBuilderNames())
		{
			if (!Filter.IsEmpty() && !Name.Contains(Filter))
			{
				continue;
			}
			UE_LOG(LogGlossaryAssets, Display, TEXT("[Glossary] 에셋 빌더 실행: %s"), *Name);
			Builders()[Name]();
		}
	}

	// ---------------------------------------------------------------- 공통

	UPackage* PreparePackage(const FString& PackagePath)
	{
		UPackage* Package = CreatePackage(*PackagePath);
		Package->FullyLoad();
		return Package;
	}

	bool SaveAsset(UObject* Asset)
	{
		if (!Asset)
		{
			return false;
		}
		UPackage* Package = Asset->GetOutermost();
		Package->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(Asset);
		const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		const bool bSaved = UPackage::SavePackage(Package, Asset, *Filename, Args);
		if (!bSaved)
		{
			UE_LOG(LogGlossaryAssets, Error, TEXT("[Glossary] 저장 실패: %s"), *Filename);
		}
		return bSaved;
	}

	UObject* CreateOrLoadAsset(UClass* Class, const FString& PackagePath)
	{
		UPackage* Package = PreparePackage(PackagePath);
		const FName Name(*FPackageName::GetShortName(PackagePath));
		if (UObject* Existing = StaticFindObject(Class, Package, *Name.ToString()))
		{
			return Existing;
		}
		return NewObject<UObject>(Package, Class, Name, RF_Public | RF_Standalone | RF_Transactional);
	}

	// ---------------------------------------------------------------- 머티리얼

	UMaterial* CreateMaterial(const FString& PackagePath)
	{
		UPackage* Package = PreparePackage(PackagePath);
		const FString Name = FPackageName::GetShortName(PackagePath);
		if (UMaterial* Existing = FindObject<UMaterial>(Package, *Name))
		{
			UMaterialEditingLibrary::DeleteAllMaterialExpressions(Existing);
			// 이전 실행의 블렌드 모드·셰이딩 모델이 남지 않게 기본값으로 되돌린다
			Existing->BlendMode = BLEND_Opaque;
			Existing->SetShadingModel(MSM_DefaultLit);
			Existing->TwoSided = false;
			Existing->MaterialDomain = MD_Surface;
			return Existing;
		}
		UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
		return Cast<UMaterial>(Factory->FactoryCreateNew(UMaterial::StaticClass(), Package, FName(*Name), RF_Public | RF_Standalone | RF_Transactional, nullptr, GWarn));
	}

	UMaterialExpression* AddExpression(UMaterial* Material, UClass* ExpressionClass, int32 X, int32 Y)
	{
		return UMaterialEditingLibrary::CreateMaterialExpression(Material, ExpressionClass, X, Y);
	}

	UMaterialExpressionScalarParameter* AddScalarParam(UMaterial* Material, FName Name, float Default, int32 X, int32 Y)
	{
		UMaterialExpressionScalarParameter* Param = AddExpression<UMaterialExpressionScalarParameter>(Material, X, Y);
		Param->ParameterName = Name;
		Param->DefaultValue = Default;
		return Param;
	}

	UMaterialExpressionVectorParameter* AddVectorParam(UMaterial* Material, FName Name, const FLinearColor& Default, int32 X, int32 Y)
	{
		UMaterialExpressionVectorParameter* Param = AddExpression<UMaterialExpressionVectorParameter>(Material, X, Y);
		Param->ParameterName = Name;
		Param->DefaultValue = Default;
		return Param;
	}

	UMaterialExpressionTextureSampleParameter2D* AddTextureParam(UMaterial* Material, FName Name, UTexture* Default, int32 X, int32 Y)
	{
		UMaterialExpressionTextureSampleParameter2D* Param = AddExpression<UMaterialExpressionTextureSampleParameter2D>(Material, X, Y);
		Param->ParameterName = Name;
		Param->Texture = Default;
		Param->AutoSetSampleType();
		return Param;
	}

	UMaterialExpressionCustom* AddCustom(UMaterial* Material, const FString& Code, const TArray<FName>& Inputs, int32 OutputType,
		const FString& Description, int32 X, int32 Y)
	{
		UMaterialExpressionCustom* Custom = AddExpression<UMaterialExpressionCustom>(Material, X, Y);
		Custom->Code = Code;
		Custom->Description = Description;
		Custom->OutputType = static_cast<ECustomMaterialOutputType>(OutputType);
		Custom->Inputs.Reset();
		for (const FName& Input : Inputs)
		{
			FCustomInput& NewInput = Custom->Inputs.AddDefaulted_GetRef();
			NewInput.InputName = Input;
		}
		Custom->PostEditChange();
		return Custom;
	}

	bool Connect(UMaterialExpression* From, const FString& FromOutput, UMaterialExpression* To, const FString& ToInput)
	{
		const bool bOk = UMaterialEditingLibrary::ConnectMaterialExpressions(From, FromOutput, To, ToInput);
		if (!bOk)
		{
			UE_LOG(LogGlossaryAssets, Error, TEXT("[Glossary] 노드 연결 실패: %s(%s) -> %s(%s)"),
				*GetNameSafe(From), *FromOutput, *GetNameSafe(To), *ToInput);
		}
		return bOk;
	}

	bool ConnectToPin(UMaterialExpression* From, EMaterialProperty Property, const FString& FromOutput)
	{
		const bool bOk = UMaterialEditingLibrary::ConnectMaterialProperty(From, FromOutput, Property);
		if (!bOk)
		{
			UE_LOG(LogGlossaryAssets, Error, TEXT("[Glossary] 머티리얼 핀 연결 실패: %s -> %d"), *GetNameSafe(From), static_cast<int32>(Property));
		}
		return bOk;
	}

	bool FinishMaterial(UMaterial* Material)
	{
		UMaterialEditingLibrary::LayoutMaterialExpressions(Material);
		Material->PreEditChange(nullptr);
		Material->PostEditChange();
		const TArray<FString> Errors = UMaterialEditingLibrary::RecompileMaterial(Material);
		for (const FString& Error : Errors)
		{
			UE_LOG(LogGlossaryAssets, Error, TEXT("[Glossary] %s 컴파일 오류: %s"), *Material->GetName(), *Error);
		}
		SaveAsset(Material);
		return Errors.Num() == 0;
	}

	UMaterialInstanceConstant* CreateMaterialInstance(const FString& PackagePath, UMaterialInterface* Parent,
		const TMap<FName, float>& Scalars, const TMap<FName, FLinearColor>& Vectors, const TMap<FName, UTexture*>& Textures)
	{
		UPackage* Package = PreparePackage(PackagePath);
		const FString Name = FPackageName::GetShortName(PackagePath);
		UMaterialInstanceConstant* Instance = FindObject<UMaterialInstanceConstant>(Package, *Name);
		if (!Instance)
		{
			UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
			Factory->InitialParent = Parent;
			Instance = Cast<UMaterialInstanceConstant>(Factory->FactoryCreateNew(UMaterialInstanceConstant::StaticClass(), Package, FName(*Name),
				RF_Public | RF_Standalone | RF_Transactional, nullptr, GWarn));
		}
		Instance->SetParentEditorOnly(Parent);
		Instance->ClearParameterValuesEditorOnly();
		for (const TPair<FName, float>& Pair : Scalars)
		{
			Instance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(Pair.Key), Pair.Value);
		}
		for (const TPair<FName, FLinearColor>& Pair : Vectors)
		{
			Instance->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(Pair.Key), Pair.Value);
		}
		for (const TPair<FName, UTexture*>& Pair : Textures)
		{
			Instance->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(Pair.Key), Pair.Value);
		}
		Instance->PostEditChange();
		SaveAsset(Instance);
		return Instance;
	}

	UMaterialFunction* CreateMaterialFunction(const FString& PackagePath, const FString& Description)
	{
		UPackage* Package = PreparePackage(PackagePath);
		const FString Name = FPackageName::GetShortName(PackagePath);
		UMaterialFunction* Function = FindObject<UMaterialFunction>(Package, *Name);
		if (Function)
		{
			UMaterialEditingLibrary::DeleteAllMaterialExpressionsInFunction(Function);
		}
		else
		{
			UMaterialFunctionFactoryNew* Factory = NewObject<UMaterialFunctionFactoryNew>();
			Function = Cast<UMaterialFunction>(Factory->FactoryCreateNew(UMaterialFunction::StaticClass(), Package, FName(*Name),
				RF_Public | RF_Standalone | RF_Transactional, nullptr, GWarn));
		}
		Function->Description = Description;
		Function->bExposeToLibrary = true;
		return Function;
	}

	// ---------------------------------------------------------------- 텍스처

	UTexture2D* CreateTexture(const FString& PackagePath, int32 Width, int32 Height, TFunctionRef<FColor(int32, int32)> Pixel, bool bSRGB, bool bTile)
	{
		UPackage* Package = PreparePackage(PackagePath);
		const FString Name = FPackageName::GetShortName(PackagePath);
		UTexture2D* Texture = FindObject<UTexture2D>(Package, *Name);
		if (!Texture)
		{
			Texture = NewObject<UTexture2D>(Package, FName(*Name), RF_Public | RF_Standalone | RF_Transactional);
		}

		TArray<FColor> Pixels;
		Pixels.SetNumUninitialized(Width * Height);
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				Pixels[Y * Width + X] = Pixel(X, Y); // FColor 메모리 순서가 BGRA 라서 TSF_BGRA8 과 그대로 맞는다
			}
		}

		Texture->PreEditChange(nullptr);
		Texture->Source.Init(Width, Height, 1, 1, TSF_BGRA8, reinterpret_cast<const uint8*>(Pixels.GetData()));
		Texture->SRGB = bSRGB;
		Texture->CompressionSettings = bSRGB ? TC_Default : TC_Masks;
		Texture->AddressX = bTile ? TA_Wrap : TA_Clamp;
		Texture->AddressY = bTile ? TA_Wrap : TA_Clamp;
		Texture->MipGenSettings = TMGS_FromTextureGroup;
		Texture->PostEditChange();
		SaveAsset(Texture);
		return Texture;
	}

	static float Hash2D(int32 X, int32 Y, int32 Seed)
	{
		uint32 H = static_cast<uint32>(X) * 374761393u + static_cast<uint32>(Y) * 668265263u + static_cast<uint32>(Seed) * 2147483647u;
		H = (H ^ (H >> 13)) * 1274126177u;
		return static_cast<float>((H ^ (H >> 16)) & 0xFFFFFF) / static_cast<float>(0xFFFFFF);
	}

	UTexture2D* CreateNoiseTexture(const FString& PackagePath, int32 Size, int32 Cells, int32 Octaves, int32 Seed)
	{
		auto ValueNoise = [Seed](float U, float V, int32 Period, int32 Octave)
		{
			const float X = U * Period, Y = V * Period;
			const int32 X0 = FMath::FloorToInt(X), Y0 = FMath::FloorToInt(Y);
			const float FX = X - X0, FY = Y - Y0;
			auto At = [&](int32 IX, int32 IY) { return Hash2D(((IX % Period) + Period) % Period, ((IY % Period) + Period) % Period, Seed + Octave * 31); };
			const float SX = FX * FX * (3.f - 2.f * FX), SY = FY * FY * (3.f - 2.f * FY);
			return FMath::Lerp(FMath::Lerp(At(X0, Y0), At(X0 + 1, Y0), SX), FMath::Lerp(At(X0, Y0 + 1), At(X0 + 1, Y0 + 1), SX), SY);
		};
		return CreateTexture(PackagePath, Size, Size, [&](int32 X, int32 Y)
		{
			float Sum = 0.f, Amp = 0.5f, Total = 0.f;
			for (int32 O = 0; O < Octaves; ++O)
			{
				Sum += ValueNoise(static_cast<float>(X) / Size, static_cast<float>(Y) / Size, Cells << O, O) * Amp;
				Total += Amp;
				Amp *= 0.5f;
			}
			const uint8 Value = static_cast<uint8>(FMath::Clamp(Sum / Total, 0.f, 1.f) * 255.f);
			return FColor(Value, Value, Value, 255);
		}, false, true);
	}

	// ---------------------------------------------------------------- 사운드

	static TArray<uint8> EncodeWav(const TArray<float>& Samples, int32 SampleRate)
	{
		const int32 DataBytes = Samples.Num() * 2;
		TArray<uint8> Wav;
		Wav.Reserve(44 + DataBytes);
		auto Write32 = [&Wav](uint32 V) { for (int32 I = 0; I < 4; ++I) Wav.Add(static_cast<uint8>((V >> (I * 8)) & 0xFF)); };
		auto Write16 = [&Wav](uint16 V) { Wav.Add(static_cast<uint8>(V & 0xFF)); Wav.Add(static_cast<uint8>(V >> 8)); };
		auto WriteTag = [&Wav](const char* Tag) { for (int32 I = 0; I < 4; ++I) Wav.Add(static_cast<uint8>(Tag[I])); };
		WriteTag("RIFF"); Write32(36 + DataBytes); WriteTag("WAVE");
		WriteTag("fmt "); Write32(16); Write16(1); Write16(1); Write32(SampleRate); Write32(SampleRate * 2); Write16(2); Write16(16);
		WriteTag("data"); Write32(DataBytes);
		for (float Sample : Samples)
		{
			Write16(static_cast<uint16>(static_cast<int16>(FMath::Clamp(Sample, -1.f, 1.f) * 32767.f)));
		}
		return Wav;
	}

	USoundWave* CreateSoundWave(const FString& PackagePath, const TArray<float>& Samples, int32 SampleRate, bool bLooping)
	{
		UPackage* Package = PreparePackage(PackagePath);
		const FString Name = FPackageName::GetShortName(PackagePath);
		const TArray<uint8> Wav = EncodeWav(Samples, SampleRate);

		USoundFactory* Factory = NewObject<USoundFactory>();
		Factory->SuppressImportDialogs();
		const uint8* Buffer = Wav.GetData();
		USoundWave* Sound = Cast<USoundWave>(Factory->FactoryCreateBinary(USoundWave::StaticClass(), Package, FName(*Name),
			RF_Public | RF_Standalone | RF_Transactional, nullptr, TEXT("wav"), Buffer, Buffer + Wav.Num(), GWarn));
		if (Sound)
		{
			Sound->bLooping = bLooping;
			SaveAsset(Sound);
		}
		return Sound;
	}

	TArray<float> MakeTone(float Frequency, float Duration, float Volume, int32 SampleRate, float Attack, float Release)
	{
		const int32 Count = FMath::Max(1, FMath::RoundToInt(Duration * SampleRate));
		TArray<float> Samples;
		Samples.SetNumUninitialized(Count);
		for (int32 I = 0; I < Count; ++I)
		{
			const float T = static_cast<float>(I) / SampleRate;
			const float Env = FMath::Min(1.f, T / FMath::Max(Attack, 1e-4f)) * FMath::Min(1.f, (Duration - T) / FMath::Max(Release, 1e-4f));
			Samples[I] = FMath::Sin(2.f * PI * Frequency * T) * Volume * FMath::Max(Env, 0.f);
		}
		return Samples;
	}

	TArray<float> MakeNoise(float Duration, float Volume, int32 SampleRate, float Decay, int32 Seed)
	{
		FRandomStream Random(Seed);
		const int32 Count = FMath::Max(1, FMath::RoundToInt(Duration * SampleRate));
		TArray<float> Samples;
		Samples.SetNumUninitialized(Count);
		for (int32 I = 0; I < Count; ++I)
		{
			const float T = static_cast<float>(I) / SampleRate;
			Samples[I] = Random.FRandRange(-1.f, 1.f) * Volume * FMath::Exp(-Decay * T);
		}
		return Samples;
	}

	TArray<float> MakeMelody(const TArray<int32>& Semitones, float NoteDuration, float Volume, int32 SampleRate)
	{
		TArray<float> Samples;
		for (int32 Semitone : Semitones)
		{
			if (Semitone <= -100)
			{
				Samples.AddZeroed(FMath::RoundToInt(NoteDuration * SampleRate));
				continue;
			}
			const float Frequency = 440.f * FMath::Pow(2.f, Semitone / 12.f);
			Samples.Append(MakeTone(Frequency, NoteDuration, Volume, SampleRate, 0.01f, NoteDuration * 0.4f));
		}
		return Samples;
	}
}

#endif // WITH_EDITOR
