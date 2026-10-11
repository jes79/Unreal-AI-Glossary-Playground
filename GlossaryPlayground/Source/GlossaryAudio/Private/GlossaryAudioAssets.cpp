// 09_Audio 분야 에셋 빌더 (에디터 전용).
// 소리 파일을 들여오지 않고 샘플을 코드로 합성해 USoundWave 를 만들고,
// 그 위에 실제 오디오 에셋(Sound Cue · Attenuation · Sound Class/Mix · Submix · Concurrency · Reverb · MetaSound)을 만든다.
#if WITH_EDITOR

#include "GlossaryAudioKit.h"
#include "GlossaryAssetKit.h"

#include "EditorAssetLibrary.h"
#include "MetasoundBuilderSubsystem.h"
#include "MetasoundEditorSubsystem.h"
#include "Editor.h"
#include "Modules/ModuleManager.h"
#include "Sound/ReverbEffect.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundConcurrency.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundNodeModulator.h"
#include "Sound/SoundNodeRandom.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "Sound/SoundSubmix.h"
#include "Sound/SoundWave.h"
#include "SubmixEffects/SubmixEffectFilter.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogGlossaryAudioAssets, Log, All);

using namespace GlossaryAssetKit;

namespace
{
	constexpr int32 SR = 44100;

	// ---------------------------------------------------------------- 간단한 합성 도구

	/** 1극 로우패스: Alpha 가 작을수록 먹먹하다 (발소리 재질 차이를 만든다) */
	TArray<float> LowPass(const TArray<float>& In, float Alpha)
	{
		TArray<float> Out;
		Out.SetNumUninitialized(In.Num());
		float Y = 0.f;
		for (int32 i = 0; i < In.Num(); ++i)
		{
			Y += Alpha * (In[i] - Y);
			Out[i] = Y;
		}
		return Out;
	}

	void MixInto(TArray<float>& Dst, const TArray<float>& Src, int32 Offset, float Gain = 1.f, bool bWrap = false)
	{
		for (int32 i = 0; i < Src.Num(); ++i)
		{
			int32 Index = Offset + i;
			if (bWrap)
			{
				Index %= Dst.Num();
			}
			else if (Index >= Dst.Num())
			{
				Dst.AddZeroed(Index - Dst.Num() + 1);
			}
			Dst[Index] += Src[i] * Gain;
		}
	}

	/** 주파수가 F0 → F1 로 떨어지는 사인 (킥 드럼·발 구르는 저음) */
	TArray<float> Sweep(float F0, float F1, float Duration, float Volume, float Decay)
	{
		TArray<float> Out;
		const int32 Count = FMath::RoundToInt(Duration * SR);
		Out.SetNumUninitialized(Count);
		float Phase = 0.f;
		for (int32 i = 0; i < Count; ++i)
		{
			const float T = static_cast<float>(i) / SR;
			const float F = FMath::Lerp(F1, F0, FMath::Exp(-T * 18.f));
			Phase += 2.f * PI * F / SR;
			Out[i] = FMath::Sin(Phase) * Volume * FMath::Exp(-Decay * T) * FMath::Min(1.f, T * 400.f);
		}
		return Out;
	}

	void Normalize(TArray<float>& Samples, float Peak)
	{
		float Max = 0.f;
		for (float S : Samples)
		{
			Max = FMath::Max(Max, FMath::Abs(S));
		}
		if (Max > KINDA_SMALL_NUMBER)
		{
			for (float& S : Samples)
			{
				S *= Peak / Max;
			}
		}
	}

	/** 화음 반주가 있는 멜로디: 멜로디 + 2옥타브 아래 베이스 */
	TArray<float> MakeSong(const TArray<int32>& Melody, const TArray<int32>& Bass, float NoteDuration, float BassDuration, float Volume)
	{
		TArray<float> Out = MakeMelody(Melody, NoteDuration, Volume, SR);
		TArray<float> Low = MakeMelody(Bass, BassDuration, Volume * 0.8f, SR);
		MixInto(Out, Low, 0, 1.f);
		return Out;
	}

	USoundWave* Wave(const TCHAR* Name, TArray<float> Samples, bool bLooping, float Peak = 0.8f)
	{
		Normalize(Samples, Peak);
		USoundWave* Sound = CreateSoundWave(GlossaryAudioKit::AssetPath(Name), Samples, SR, bLooping);
		if (!Sound)
		{
			UE_LOG(LogGlossaryAudioAssets, Error, TEXT("[Glossary] 사운드 생성 실패: %s"), Name);
		}
		return Sound;
	}

	/** 같은 이름의 기존 에셋을 지운다 (Sound Cue 그래프·MetaSound 를 매번 깨끗하게 다시 만들기 위해) */
	void DeleteIfExists(const FString& PackagePath)
	{
		if (UEditorAssetLibrary::DoesAssetExist(PackagePath))
		{
			UEditorAssetLibrary::DeleteAsset(PackagePath);
		}
	}

	// ---------------------------------------------------------------- 0901 Sound Cue

	USoundCue* BuildFootstepCue(const TArray<USoundWave*>& Footsteps)
	{
		// Sound Cue 그래프는 AudioEditor 모듈이 그린다 (커맨드렛에서도 로드돼 있어야 함)
		FModuleManager::Get().LoadModule(TEXT("AudioEditor"));
		if (!USoundCue::GetSoundCueAudioEditor().IsValid())
		{
			UE_LOG(LogGlossaryAudioAssets, Error, TEXT("[Glossary] SoundCue 그래프 에디터가 없어 SCue_Footsteps 를 만들 수 없음"));
			return nullptr;
		}

		const FString Path = GlossaryAudioKit::AssetPath(TEXT("SCue_Footsteps"));
		DeleteIfExists(Path);
		UPackage* Package = PreparePackage(Path);
		USoundCue* Cue = NewObject<USoundCue>(Package, TEXT("SCue_Footsteps"), RF_Public | RF_Standalone | RF_Transactional);

		// 출력 ← Modulator(피치 0.9~1.1) ← Random ← Wave Player ×4
		USoundNodeModulator* Modulator = Cue->ConstructSoundNode<USoundNodeModulator>();
		Modulator->PitchMin = 0.9f;
		Modulator->PitchMax = 1.1f;
		Modulator->VolumeMin = 0.9f;
		Modulator->VolumeMax = 1.0f;

		USoundNodeRandom* Random = Cue->ConstructSoundNode<USoundNodeRandom>();
		Random->bRandomizeWithoutReplacement = true; // 같은 발소리가 연속으로 나오지 않게

		for (USoundWave* Footstep : Footsteps)
		{
			USoundNodeWavePlayer* Player = Cue->ConstructSoundNode<USoundNodeWavePlayer>();
			Player->SetSoundWave(Footstep);
			const int32 Index = Random->ChildNodes.Num();
			Random->InsertChildNode(Index); // Weights 배열도 같이 늘어난다
			Random->ChildNodes[Index] = Player;
		}

		Modulator->InsertChildNode(0);
		Modulator->ChildNodes[0] = Random;
		Cue->FirstNode = Modulator;

		// 노드 연결(ChildNodes)을 에디터 그래프에도 반영하고 노드 위치를 정리
		Cue->LinkGraphNodesFromSoundNodes();
		auto Place = [](USoundNode* Node, int32 X, int32 Y)
		{
			if (Node && Node->GraphNode)
			{
				Node->GraphNode->NodePosX = X;
				Node->GraphNode->NodePosY = Y;
			}
		};
		Place(Modulator, -250, 0);
		Place(Random, -500, 0);
		for (int32 i = 0; i < Random->ChildNodes.Num(); ++i)
		{
			Place(Random->ChildNodes[i], -850, -180 + i * 120);
		}
		Cue->CacheAggregateValues();
		Cue->PostEditChange();
		SaveAsset(Cue);
		return Cue;
	}

	// ---------------------------------------------------------------- 0902 MetaSound

	void BuildEngineMetaSound()
	{
		const FString Path = GlossaryAudioKit::AssetPath(TEXT("MS_EngineRPM"));
		FString Log;
		UMetaSoundSourceBuilder* Builder = GlossaryAudioKit::CreateEngineRPMBuilder(TEXT("GlossaryEngineRPM_Asset"), Log);
		UE_LOG(LogGlossaryAudioAssets, Display, TEXT("[Glossary] MetaSound 빌더: %s"), *Log);
		UMetaSoundEditorSubsystem* EditorSubsystem = GEditor ? GEditor->GetEditorSubsystem<UMetaSoundEditorSubsystem>() : nullptr;
		if (!Builder || !EditorSubsystem)
		{
			UE_LOG(LogGlossaryAudioAssets, Warning, TEXT("[Glossary] MS_EngineRPM 을 에셋으로 만들지 못함 (데모는 실행 중 빌더로 대체)"));
			return;
		}

		DeleteIfExists(Path);
		EMetaSoundBuilderResult Result = EMetaSoundBuilderResult::Failed;
		TScriptInterface<IMetaSoundDocumentInterface> Asset = EditorSubsystem->BuildToAsset(Builder, TEXT("Glossary"), TEXT("MS_EngineRPM"),
			TEXT("/Game/Glossary/09_Audio"), Result);
		if (Result == EMetaSoundBuilderResult::Succeeded && Asset.GetObject())
		{
			SaveAsset(Asset.GetObject());
			UE_LOG(LogGlossaryAudioAssets, Display, TEXT("[Glossary] MetaSound 에셋 생성: %s"), *Path);
		}
		else
		{
			UE_LOG(LogGlossaryAudioAssets, Warning, TEXT("[Glossary] BuildToAsset 실패: %s"), *Path);
		}
		if (UMetaSoundBuilderSubsystem* Subsystem = UMetaSoundBuilderSubsystem::Get())
		{
			Subsystem->UnregisterBuilder(TEXT("GlossaryEngineRPM_Asset"));
		}
	}
}

void BuildGlossaryAudioAssets()
{
	// ---------------------------------------------------------------- 사운드 클래스 (0904)
	USoundClass* Master = CreateOrLoad<USoundClass>(GlossaryAudioKit::AssetPath(TEXT("SC_Master")));
	USoundClass* Music = CreateOrLoad<USoundClass>(GlossaryAudioKit::AssetPath(TEXT("SC_Music")));
	USoundClass* SFX = CreateOrLoad<USoundClass>(GlossaryAudioKit::AssetPath(TEXT("SC_SFX")));
	USoundClass* Voice = CreateOrLoad<USoundClass>(GlossaryAudioKit::AssetPath(TEXT("SC_Voice")));
	Master->ChildClasses = { Music, SFX, Voice };
	for (USoundClass* Child : { Music, SFX, Voice })
	{
		Child->ParentClass = Master;
		Child->Properties.Volume = 1.f;
	}

	// ---------------------------------------------------------------- Submix + Low Pass 이펙트 (0905)
	USubmixEffectFilterPreset* LowPassPreset = CreateOrLoad<USubmixEffectFilterPreset>(GlossaryAudioKit::AssetPath(TEXT("SSFX_LowPass")));
	FSubmixEffectFilterSettings FilterSettings;
	FilterSettings.FilterType = ESubmixFilterType::LowPass;
	FilterSettings.FilterAlgorithm = ESubmixFilterAlgorithm::StateVariable;
	FilterSettings.FilterFrequency = 20000.f; // 평소엔 활짝 열어 둔다 → 일시정지 때 실행 중 500Hz 로 내린다
	FilterSettings.FilterQ = 0.7f;
	LowPassPreset->Settings = FilterSettings;
	SaveAsset(LowPassPreset);

	USoundSubmix* SFXSubmix = CreateOrLoad<USoundSubmix>(GlossaryAudioKit::AssetPath(TEXT("SSub_SFX")));
	SFXSubmix->SubmixEffectChain = { LowPassPreset };
	SaveAsset(SFXSubmix);

	// ---------------------------------------------------------------- 합성 사운드
	TArray<USoundWave*> Footsteps;
	const float FootAlpha[4] = { 0.30f, 0.18f, 0.45f, 0.24f }; // 재질(흙·카펫·자갈·나무) 느낌을 필터로만 다르게
	for (int32 i = 0; i < 4; ++i)
	{
		TArray<float> Step = LowPass(MakeNoise(0.32f, 0.9f, SR, 22.f, 11 + i), FootAlpha[i]);
		MixInto(Step, Sweep(120.f + i * 12.f, 55.f, 0.25f, 0.5f, 18.f), 0, 1.f);
		Footsteps.Add(Wave(*FString::Printf(TEXT("S_Footstep_%02d"), i + 1), Step, false));
	}

	// 모닥불: 낮게 깔린 쉬익 소리 + 무작위 탁탁 소리 (4초 반복)
	TArray<float> Fire = LowPass(MakeNoise(4.f, 0.25f, SR, 0.f, 77), 0.08f);
	{
		FRandomStream Rand(5);
		for (int32 i = 0; i < 46; ++i)
		{
			const int32 Offset = Rand.RandRange(0, Fire.Num() - 1);
			MixInto(Fire, LowPass(MakeNoise(0.03f, Rand.FRandRange(0.3f, 1.f), SR, 160.f, 100 + i), 0.5f), Offset, 1.f, true);
		}
	}
	USoundWave* Campfire = Wave(TEXT("S_Campfire_Loop"), Fire, true, 0.7f);

	// 총소리: 긴 꼬리(1.2초)라 연사하면 여러 개가 겹친다 → 0907 Concurrency 가 필요해지는 상황
	TArray<float> Gun = MakeNoise(1.2f, 1.f, SR, 5.5f, 9);
	MixInto(Gun, LowPass(MakeNoise(1.2f, 1.f, SR, 3.f, 10), 0.04f), 0, 2.5f);
	MixInto(Gun, Sweep(90.f, 38.f, 0.8f, 0.9f, 6.f), 0, 1.f);
	USoundWave* Gunshot = Wave(TEXT("S_Gunshot"), Gun, false, 0.9f);

	// 타악기(킥 + 클릭) — 0908 Quartz, 0906 손뼉 대용
	TArray<float> Kick = Sweep(170.f, 48.f, 0.35f, 1.f, 9.f);
	MixInto(Kick, MakeNoise(0.03f, 0.6f, SR, 120.f, 3), 0, 1.f);
	USoundWave* Percussion = Wave(TEXT("S_Percussion"), Kick, false, 0.9f);

	TArray<float> ClapSamples = LowPass(MakeNoise(0.12f, 1.f, SR, 45.f, 21), 0.6f);
	USoundWave* Clap = Wave(TEXT("S_Clap"), ClapSamples, false, 0.9f);

	// 잔잔한 BGM (C 장조 아르페지오, 0.5초 음표 16개 = 8초) / 전투 BGM (A 단조 빠른 16분 음표, 4초)
	USoundWave* CalmBGM = Wave(TEXT("S_BGM_Calm_Loop"), MakeSong(
		{ 3, 7, 10, 15, 10, 7, 3, 7, 5, 8, 12, 17, 12, 8, 5, 8 },
		{ -21, -21, -16, -16 }, 0.5f, 2.f, 0.3f), true, 0.6f);
	TArray<float> Battle = MakeSong(
		{ 0, 3, 7, 12, 0, 3, 7, 12, -2, 2, 5, 10, -2, 2, 5, 10, -4, 0, 3, 8, -4, 0, 3, 8, -5, -1, 2, 7, -5, -1, 2, 7 },
		{ -24, -24, -24, -24, -26, -26, -26, -26, -28, -28, -28, -28, -29, -29, -29, -29 }, 0.125f, 0.25f, 0.3f);
	for (int32 Beat = 0; Beat < 8; ++Beat)
	{
		MixInto(Battle, Kick, Beat * SR / 2, 0.5f, true);
	}
	USoundWave* BattleBGM = Wave(TEXT("S_BGM_Battle_Loop"), Battle, true, 0.7f);

	// 효과음 반복(차임 + 쉬익): 높은 주파수가 많아서 로우패스 효과가 잘 보인다 (0904·0905)
	TArray<float> Chime = MakeMelody({ 27, -100, 31, -100, 34, 39, -100, -100, 27, 34, -100, 38, -100, 31, -100, -100 }, 0.25f, 0.35f, SR);
	MixInto(Chime, MakeNoise(4.f, 0.08f, SR, 0.f, 31), 0, 1.f, true);
	USoundWave* ChimeLoop = Wave(TEXT("S_Chime_Loop"), Chime, true, 0.6f);

	// 목소리 흉내: 기본음 + 배음 2·3 으로 된 짧은 음절을 높낮이를 바꿔 이어 붙인다 (3초 반복)
	TArray<float> Babble;
	{
		FRandomStream Rand(8);
		for (int32 Syllable = 0; Syllable < 16; ++Syllable)
		{
			const float F0 = Rand.FRandRange(150.f, 240.f);
			const float Len = Rand.FRandRange(0.1f, 0.2f);
			TArray<float> S = MakeTone(F0, Len, 0.5f, SR, 0.02f, 0.05f);
			MixInto(S, MakeTone(F0 * 2.f, Len, 0.3f, SR, 0.02f, 0.05f), 0, 1.f);
			MixInto(S, MakeTone(F0 * 3.f, Len, 0.15f, SR, 0.02f, 0.05f), 0, 1.f);
			Babble.Append(S);
			Babble.AddZeroed(FMath::RoundToInt(Rand.FRandRange(0.02f, 0.08f) * SR));
		}
		Babble.SetNumZeroed(3 * SR);
	}
	USoundWave* VoiceLoop = Wave(TEXT("S_Voice_Loop"), Babble, true, 0.6f);

	// 엔진 험(60Hz 톱니파 정확히 60주기 = 1초 반복) — 0902 MetaSound 가 실패할 때의 대체 소리
	TArray<float> Hum;
	Hum.SetNumUninitialized(SR);
	for (int32 i = 0; i < SR; ++i)
	{
		const float Phase = FMath::Fmod(60.f * i / SR, 1.f);
		Hum[i] = (Phase * 2.f - 1.f) * 0.6f + FMath::Sin(2.f * PI * 30.f * i / SR) * 0.3f;
	}
	Wave(TEXT("S_EngineHum_Loop"), Hum, true, 0.6f);

	// ---------------------------------------------------------------- 사운드 클래스·서브믹스 지정
	auto Route = [](USoundWave* Sound, USoundClass* Class, USoundSubmix* Submix)
	{
		if (Sound)
		{
			Sound->SoundClassObject = Class;
			Sound->SoundSubmixObject = Submix; // null 이면 기본(Master) 서브믹스
			SaveAsset(Sound);
		}
	};
	for (USoundWave* Footstep : Footsteps)
	{
		Route(Footstep, SFX, SFXSubmix);
	}
	Route(Campfire, SFX, SFXSubmix);
	Route(Gunshot, SFX, SFXSubmix);
	Route(Percussion, SFX, SFXSubmix);
	Route(Clap, SFX, SFXSubmix);
	Route(ChimeLoop, SFX, SFXSubmix);
	Route(CalmBGM, Music, nullptr);  // 음악은 SFX 서브믹스를 거치지 않는다 → 0905 에서 음악은 또렷하게 남는다
	Route(BattleBGM, Music, nullptr);
	Route(VoiceLoop, Voice, nullptr);
	SaveAsset(Master);
	SaveAsset(Music);
	SaveAsset(SFX);
	SaveAsset(Voice);

	// ---------------------------------------------------------------- Sound Mix (0904 옵션 슬라이더가 덮어쓸 믹스)
	USoundMix* OptionsMix = CreateOrLoad<USoundMix>(GlossaryAudioKit::AssetPath(TEXT("SMix_Options")));
	OptionsMix->SoundClassEffects.Reset();
	for (USoundClass* Class : { Music, SFX, Voice })
	{
		FSoundClassAdjuster Adjuster;
		Adjuster.SoundClassObject = Class;
		Adjuster.VolumeAdjuster = 1.f;
		Adjuster.PitchAdjuster = 1.f;
		Adjuster.bApplyToChildren = false;
		OptionsMix->SoundClassEffects.Add(Adjuster);
	}
	OptionsMix->bApplyEQ = false;
	SaveAsset(OptionsMix);

	// ---------------------------------------------------------------- Attenuation (0903 모닥불)
	USoundAttenuation* FireAttenuation = CreateOrLoad<USoundAttenuation>(GlossaryAudioKit::AssetPath(TEXT("ATT_Campfire")));
	FSoundAttenuationSettings& Att = FireAttenuation->Attenuation;
	Att.bAttenuate = true;
	Att.bSpatialize = true;                     // 좌우(방향) 위치감
	Att.AttenuationShape = EAttenuationShape::Sphere;
	Att.AttenuationShapeExtents = FVector(200.f, 0.f, 0.f); // Inner Radius 200: 이 안에서는 최대 볼륨
	Att.FalloffDistance = 1500.f;               // 200~1700cm 사이에서 0까지 줄어든다
	Att.DistanceAlgorithm = EAttenuationDistanceModel::Linear;
	SaveAsset(FireAttenuation);

	// ---------------------------------------------------------------- Concurrency (0907 총소리)
	USoundConcurrency* GunConcurrency = CreateOrLoad<USoundConcurrency>(GlossaryAudioKit::AssetPath(TEXT("SCon_Gunshot")));
	GunConcurrency->Concurrency.MaxCount = 4;
	GunConcurrency->Concurrency.ResolutionRule = EMaxConcurrentResolutionRule::StopOldest;
	GunConcurrency->Concurrency.bLimitToOwner = false;
	SaveAsset(GunConcurrency);

	// ---------------------------------------------------------------- Reverb Effect (0906 동굴)
	UReverbEffect* Cave = CreateOrLoad<UReverbEffect>(GlossaryAudioKit::AssetPath(TEXT("RVB_Cave")));
	Cave->Density = 1.f;
	Cave->Diffusion = 1.f;
	Cave->Gain = 0.6f;
	Cave->GainHF = 0.6f;
	Cave->DecayTime = 4.0f;       // 울림이 4초 동안 이어진다
	Cave->DecayHFRatio = 0.6f;
	Cave->ReflectionsGain = 1.2f;
	Cave->ReflectionsDelay = 0.03f;
	Cave->LateGain = 1.8f;
	Cave->LateDelay = 0.05f;
	Cave->AirAbsorptionGainHF = 0.99f;
	SaveAsset(Cave);

	// ---------------------------------------------------------------- Sound Cue · MetaSound
	BuildFootstepCue(Footsteps);
	BuildEngineMetaSound();

	UE_LOG(LogGlossaryAudioAssets, Display, TEXT("[Glossary] 09_Audio 에셋 생성 완료 (%s)"), Clap && Percussion && VoiceLoop && ChimeLoop && BattleBGM ? TEXT("OK") : TEXT("일부 실패"));
}

#endif // WITH_EDITOR
