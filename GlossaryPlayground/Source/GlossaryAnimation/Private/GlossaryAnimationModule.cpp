#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "GlossaryAssetKit.h"
#include "D0305_AnimNotify_Hit.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "Animation/Skeleton.h"
#include "CineCameraActor.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Character.h"
#include "LevelSequence.h"
#include "MovieScene.h"
#include "Sections/MovieScene3DTransformSection.h"
#include "Sections/MovieSceneCameraCutSection.h"
#include "Sections/MovieSceneFadeSection.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "Tracks/MovieSceneCameraCutTrack.h"
#include "Tracks/MovieSceneFadeTrack.h"
#include "Tracks/MovieSceneSkeletalAnimationTrack.h"
#include "Channels/MovieSceneDoubleChannel.h"
#include "Sound/SoundWave.h"

DEFINE_LOG_CATEGORY_STATIC(LogGlossaryAnim, Log, All);

namespace D03Build
{
	const FString Folder = TEXT("/Game/Glossary/03_Animation/");

	UAnimSequence* Anim(const FString& Rel)
	{
		return LoadObject<UAnimSequence>(nullptr, *(TEXT("/Game/Characters/Mannequins/Anims/") + Rel));
	}

	/** 시퀀스의 Time 시점 뼈 위치(컴포넌트 공간) — 부모 사슬을 따라 로컬 트랜스폼을 곱한다 */
	FVector BoneCS(const UAnimSequence* Seq, const FReferenceSkeleton& Ref, int32 Bone, double Time)
	{
		FTransform T = FTransform::Identity;
		for (int32 B = Bone; B != INDEX_NONE; B = Ref.GetParentIndex(B))
		{
			FTransform Local;
			Seq->GetBoneTransform(Local, FSkeletonPoseBoneIndex(B), FAnimExtractContext(Time), true);
			T = T * Local;
		}
		return T.GetLocation();
	}

	/**
	 * 타격 프레임 찾기: 손·발이 골반보다 가장 멀리 앞으로(+Y) 뻗은 시점.
	 * 사람이 몽타주를 열어 '주먹이 다 뻗은 프레임' 에 Notify 를 놓는 것을 자동으로 흉내 낸다.
	 */
	float FindHitTime(const UAnimSequence* Seq)
	{
		const USkeleton* Skel = Seq->GetSkeleton();
		const FReferenceSkeleton& Ref = Skel->GetReferenceSkeleton();
		const int32 Pelvis = Ref.FindBoneIndex(TEXT("pelvis"));
		const int32 Limbs[] = { Ref.FindBoneIndex(TEXT("hand_r")), Ref.FindBoneIndex(TEXT("hand_l")), Ref.FindBoneIndex(TEXT("foot_r")), Ref.FindBoneIndex(TEXT("foot_l")) };
		const float Len = Seq->GetPlayLength();
		float BestT = Len * 0.4f, Best = -1e9f;
		for (float T = Len * 0.1f; T < Len * 0.85f; T += 1.f / 60.f)
		{
			const FVector P = BoneCS(Seq, Ref, Pelvis, T);
			for (int32 L : Limbs)
			{
				if (L == INDEX_NONE) { continue; }
				const FVector D = BoneCS(Seq, Ref, L, T) - P;
				if (D.Y > Best) { Best = D.Y; BestT = T; }
			}
		}
		return BestT;
	}

	/**
	 * 몽타주 에셋을 코드로 만든다: Slot 트랙 하나 + 시퀀스마다 세그먼트·섹션.
	 * bHitNotify 면 섹션마다 타격 프레임에 UD0305_AnimNotify_Hit 를 넣는다.
	 */
	UAnimMontage* BuildMontage(const FString& Name, FName Slot, const TArray<UAnimSequence*>& Seqs, const TArray<FName>& SectionNames, bool bHitNotify)
	{
		using namespace GlossaryAssetKit;
		UAnimMontage* M = CreateOrLoad<UAnimMontage>(Folder + Name);
		if (!M || Seqs.Num() == 0 || !Seqs[0])
		{
			return nullptr;
		}
		M->SetSkeleton(Seqs[0]->GetSkeleton());
		M->SlotAnimTracks.Reset();
		M->CompositeSections.Reset();
		M->Notifies.Reset();
		M->AnimNotifyTracks.Reset();
		M->AnimNotifyTracks.Add(FAnimNotifyTrack(TEXT("Hit"), FLinearColor::Red));

		FSlotAnimationTrack& Track = M->SlotAnimTracks.AddDefaulted_GetRef();
		Track.SlotName = Slot;
		float Pos = 0.f;
		TArray<float> Starts, HitTimes;
		for (UAnimSequence* Seq : Seqs)
		{
			FAnimSegment Seg;
			Seg.SetAnimReference(Seq, true);
			Seg.StartPos = Pos;
			Track.AnimTrack.AnimSegments.Add(Seg);
			Starts.Add(Pos);
			HitTimes.Add(bHitNotify ? Pos + FindHitTime(Seq) : -1.f);
			Pos += Seg.GetLength();
		}

		M->SetCompositeLength(Pos);   // 몽타주 전체 길이


		for (int32 i = 0; i < Seqs.Num(); ++i)
		{
			FCompositeSection Section;
			Section.SectionName = SectionNames.IsValidIndex(i) ? SectionNames[i] : FName(*FString::Printf(TEXT("Section%d"), i));
			Section.Link(M, Starts[i]);
			Section.NextSectionName = NAME_None;   // 섹션 끝에서 멈춘다 — 콤보 입력이 있을 때만 다음 섹션으로 잇는다
			M->CompositeSections.Add(Section);

			if (HitTimes[i] >= 0.f)
			{
				FAnimNotifyEvent& E = M->Notifies.AddDefaulted_GetRef();
				E.Notify = NewObject<UD0305_AnimNotify_Hit>(M, NAME_None, RF_Transactional);
				E.NotifyName = TEXT("Hit");
				E.Link(M, HitTimes[i]);
				E.TriggerTimeOffset = GetTriggerTimeOffsetForType(static_cast<UAnimSequenceBase*>(M)->CalculateOffsetForNotify(HitTimes[i]));
				E.TrackIndex = 0;
				UE_LOG(LogGlossaryAnim, Display, TEXT("[Glossary] %s 섹션 %s 타격 Notify %.2fs (섹션 시작 +%.2fs)"), *Name, *Section.SectionName.ToString(), HitTimes[i], HitTimes[i] - Starts[i]);
			}
		}
		M->BlendIn = FAlphaBlend(0.15f);
		M->BlendOut = FAlphaBlend(0.2f);
		M->UpdateLinkableElements();
		static_cast<UAnimSequenceBase*>(M)->RefreshCacheData();
		M->PostEditChange();
		SaveAsset(M);
		return M;
	}

	/** 0303: Direction(-180~180) × Speed(0~600) 2D Blend Space — 8방향 걷기(200)·조깅(500) + Idle */
	void BuildBlendSpace()
	{
		using namespace GlossaryAssetKit;
		UAnimSequence* Idle = Anim(TEXT("Unarmed/MM_Idle"));
		if (!Idle)
		{
			UE_LOG(LogGlossaryAnim, Error, TEXT("[Glossary] MM_Idle 를 찾지 못함"));
			return;
		}
		UBlendSpace* BS = CreateOrLoad<UBlendSpace>(Folder + TEXT("BS_Locomotion8Dir"));
		while (BS->GetNumberOfBlendSamples() > 0)
		{
			BS->DeleteSample(0);
		}
		BS->SetSkeleton(Idle->GetSkeleton());

		// BlendParameters 는 protected 라서 리플렉션으로 채운다 (에디터의 Axis Settings 패널과 같은 값)
		FStructProperty* Prop = FindFProperty<FStructProperty>(UBlendSpace::StaticClass(), TEXT("BlendParameters"));
		FBlendParameter* X = Prop->ContainerPtrToValuePtr<FBlendParameter>(BS, 0);
		FBlendParameter* Y = Prop->ContainerPtrToValuePtr<FBlendParameter>(BS, 1);
		X->DisplayName = TEXT("Direction"); X->Min = -180.f; X->Max = 180.f; X->GridNum = 8; X->bSnapToGrid = true;
		Y->DisplayName = TEXT("Speed");     Y->Min = 0.f;    Y->Max = 600.f; Y->GridNum = 12; Y->bSnapToGrid = true;

		struct FDir { const TCHAR* Suffix; float Angle; };
		const FDir Dirs[] = {
			{ TEXT("Fwd"), 0.f }, { TEXT("Fwd_Right"), 45.f }, { TEXT("Right"), 90.f }, { TEXT("Bwd_Right"), 135.f },
			{ TEXT("Bwd"), 180.f }, { TEXT("Bwd_Left"), -135.f }, { TEXT("Left"), -90.f }, { TEXT("Fwd_Left"), -45.f },
		};
		for (float A : { -180.f, -90.f, 0.f, 90.f, 180.f })
		{
			BS->AddSample(Idle, FVector(A, 0.f, 0.f));
		}
		for (const FDir& D : Dirs)
		{
			UAnimSequence* Walk = Anim(FString::Printf(TEXT("Unarmed/Walk/MF_Unarmed_Walk_%s"), D.Suffix));
			UAnimSequence* Jog = Anim(FString::Printf(TEXT("Unarmed/Jog/MF_Unarmed_Jog_%s"), D.Suffix));
			if (Walk) { BS->AddSample(Walk, FVector(D.Angle, 200.f, 0.f)); }
			if (Jog)  { BS->AddSample(Jog, FVector(D.Angle, 500.f, 0.f)); }
			if (D.Angle == 180.f)
			{
				// 뒤(±180)는 축 양 끝 모두에 둔다
				if (Walk) { BS->AddSample(Walk, FVector(-180.f, 200.f, 0.f)); }
				if (Jog)  { BS->AddSample(Jog, FVector(-180.f, 500.f, 0.f)); }
			}
		}
		BS->ValidateSampleData();
		BS->ResampleData();
		BS->PostEditChange();
		SaveAsset(BS);
		UE_LOG(LogGlossaryAnim, Display, TEXT("[Glossary] BS_Locomotion8Dir 샘플 %d개"), BS->GetNumberOfBlendSamples());
	}

	/** 0314: 1초 Ease In/Out 커브 + 비교용 Linear */
	void BuildCurves()
	{
		using namespace GlossaryAssetKit;
		auto Make = [](const FString& Name, ERichCurveInterpMode Mode)
		{
			UCurveFloat* C = CreateOrLoad<UCurveFloat>(Folder + Name);
			C->FloatCurve.Reset();
			for (float T : { 0.f, 1.f })
			{
				const FKeyHandle H = C->FloatCurve.AddKey(T, T);
				C->FloatCurve.SetKeyInterpMode(H, Mode);
				if (Mode == RCIM_Cubic)
				{
					// 양 끝 접선을 0 으로 → 천천히 출발해서 천천히 멈춘다 (Ease In/Out)
					C->FloatCurve.SetKeyTangentMode(H, RCTM_User);
					FRichCurveKey& K = C->FloatCurve.GetKey(H);
					K.ArriveTangent = 0.f;
					K.LeaveTangent = 0.f;
				}
			}
			SaveAsset(C);
		};
		Make(TEXT("C_DoorEaseInOut"), RCIM_Cubic);
		Make(TEXT("C_DoorLinear"), RCIM_Linear);
	}

	void BuildSounds()
	{
		using namespace GlossaryAssetKit;
		// 짧은 '퍽' 소리: 노이즈 버스트 + 낮은 톤
		TArray<float> Noise = MakeNoise(0.18f, 0.7f, 44100, 22.f, 3);
		const TArray<float> Tone = MakeTone(95.f, 0.18f, 0.6f, 44100, 0.002f, 0.12f);
		for (int32 i = 0; i < Noise.Num() && i < Tone.Num(); ++i)
		{
			Noise[i] = FMath::Clamp(Noise[i] + Tone[i], -1.f, 1.f);
		}
		CreateSoundWave(Folder + TEXT("S_Hit"), Noise);
	}

	FFrameNumber Frame(UMovieScene* MS, double Seconds)
	{
		return (Seconds * MS->GetTickResolution()).RoundToFrame();
	}

	/** 0313: 카메라 컷 3개 + 페이드 + 주인공 애니메이션이 있는 5초 Level Sequence */
	void BuildSequence()
	{
		using namespace GlossaryAssetKit;
		ULevelSequence* Seq = CreateOrLoad<ULevelSequence>(Folder + TEXT("LS_Cutscene"));
		Seq->Initialize();   // 빈 MovieScene 으로 새로 시작 (다시 빌드해도 같은 결과)
		UMovieScene* MS = Seq->GetMovieScene();
		MS->SetDisplayRate(FFrameRate(30, 1));
		const FFrameNumber End = Frame(MS, 5.0);
		MS->SetPlaybackRange(0, End.Value);

		// 주인공 위치(데모 BeginPlay 와 같은 값): (600, 0) 에서 -X 쪽을 본다
		const FVector Hero(600.f, 0.f, 0.f);
		struct FCamKey { FVector From, To; FVector LookFrom, LookTo; };
		struct FCam { const TCHAR* Name; FCamKey Key; double Start, Stop; };
		const FCam Cams[] = {
			{ TEXT("Cam_Wide"),  { FVector(-250, -650, 320), FVector(-100, -420, 260), Hero + FVector(0, 0, 100), Hero + FVector(0, 0, 100) }, 0.0, 1.7 },
			{ TEXT("Cam_Close"), { FVector(420, -60, 175),   FVector(470, -40, 170),   Hero + FVector(0, 0, 160), Hero + FVector(0, 0, 160) }, 1.7, 3.4 },
			{ TEXT("Cam_Orbit"), { FVector(600, 650, 520),   FVector(980, 380, 420),   Hero + FVector(0, 0, 90),  Hero + FVector(0, 0, 90) },  3.4, 5.0 },
		};

		UMovieSceneCameraCutTrack* CutTrack = Cast<UMovieSceneCameraCutTrack>(MS->AddCameraCutTrack(UMovieSceneCameraCutTrack::StaticClass()));
		for (const FCam& C : Cams)
		{
			// Possessable: 실행 중 데모가 스폰한 CineCameraActor 를 SetBinding 으로 묶는다
			const FGuid Guid = MS->AddPossessable(C.Name, ACineCameraActor::StaticClass());

			// 카메라가 컷 동안 천천히 움직이도록 Transform 트랙 키 2개
			UMovieScene3DTransformTrack* TT = MS->AddTrack<UMovieScene3DTransformTrack>(Guid);
			UMovieScene3DTransformSection* TS = Cast<UMovieScene3DTransformSection>(TT->CreateNewSection());
			TT->AddSection(*TS);
			TS->SetRange(TRange<FFrameNumber>(0, End));
			TArrayView<FMovieSceneDoubleChannel*> Ch = TS->GetChannelProxy().GetChannels<FMovieSceneDoubleChannel>();
			for (int32 k = 0; k < 2; ++k)
			{
				const FFrameNumber F = Frame(MS, k == 0 ? C.Start : C.Stop);
				const FVector P = k == 0 ? C.Key.From : C.Key.To;
				const FRotator R = ((k == 0 ? C.Key.LookFrom : C.Key.LookTo) - P).Rotation();
				Ch[0]->AddCubicKey(F, P.X); Ch[1]->AddCubicKey(F, P.Y); Ch[2]->AddCubicKey(F, P.Z);
				Ch[3]->AddCubicKey(F, R.Roll); Ch[4]->AddCubicKey(F, R.Pitch); Ch[5]->AddCubicKey(F, R.Yaw);
				Ch[6]->AddCubicKey(F, 1.0); Ch[7]->AddCubicKey(F, 1.0); Ch[8]->AddCubicKey(F, 1.0);
			}

			// 카메라 컷: 이 구간 동안 이 카메라로 본다
			UMovieSceneCameraCutSection* Cut = CutTrack->AddNewCameraCut(UE::MovieScene::FRelativeObjectBindingID(Guid), Frame(MS, C.Start));
			Cut->SetRange(TRange<FFrameNumber>(Frame(MS, C.Start), Frame(MS, C.Stop)));
		}

		// 주인공: Skeletal Animation 트랙 (공격 두 번)
		const FGuid HeroGuid = MS->AddPossessable(TEXT("Hero"), ACharacter::StaticClass());
		UMovieSceneSkeletalAnimationTrack* AnimTrack = MS->AddTrack<UMovieSceneSkeletalAnimationTrack>(HeroGuid);
		if (UAnimSequence* A1 = Anim(TEXT("Unarmed/Attack/MM_Attack_01"))) { AnimTrack->AddNewAnimation(Frame(MS, 1.8), A1); }
		if (UAnimSequence* A2 = Anim(TEXT("Unarmed/Attack/MM_Attack_03"))) { AnimTrack->AddNewAnimation(Frame(MS, 3.5), A2); }

		// 페이드: 0~0.5초 검은 화면에서 밝아지고, 4.4~5초 다시 어두워진다
		UMovieSceneTrack* FadeTrack = MS->AddTrack(UMovieSceneFadeTrack::StaticClass());
		UMovieSceneFadeSection* Fade = Cast<UMovieSceneFadeSection>(FadeTrack->CreateNewSection());
		FadeTrack->AddSection(*Fade);
		Fade->SetRange(TRange<FFrameNumber>(0, End));
		Fade->FloatCurve.AddLinearKey(Frame(MS, 0.0), 1.f);
		Fade->FloatCurve.AddLinearKey(Frame(MS, 0.5), 0.f);
		Fade->FloatCurve.AddLinearKey(Frame(MS, 4.4), 0.f);
		Fade->FloatCurve.AddLinearKey(Frame(MS, 5.0), 1.f);

		Seq->MarkPackageDirty();
		SaveAsset(Seq);
		UE_LOG(LogGlossaryAnim, Display, TEXT("[Glossary] LS_Cutscene: 카메라 3 · 컷 %d · 페이드 · 애니메이션 트랙"), CutTrack->GetAllSections().Num());
	}
}

/** 03_Animation 분야 데모가 쓰는 생성 에셋 (/Game/Glossary/03_Animation) */
static void BuildGlossaryAnimationAssets()
{
	using namespace D03Build;
	BuildBlendSpace();

	// 0304·0305·0315: 3단 콤보 몽타주 (DefaultSlot, 섹션 Attack1/2/3, 타격 Notify)
	BuildMontage(TEXT("AM_Attack_Combo"), TEXT("DefaultSlot"),
		{ Anim(TEXT("Unarmed/Attack/MM_Attack_01")), Anim(TEXT("Unarmed/Attack/MM_Attack_02")), Anim(TEXT("Unarmed/Attack/MM_Attack_03")) },
		{ TEXT("Attack1"), TEXT("Attack2"), TEXT("Attack3") }, true);

	// 0306: 구르기(대시) 몽타주 — MM_Dash 는 Root Motion 이 켜진 시퀀스
	if (UAnimSequence* Dash = Anim(TEXT("Unarmed/Jump/MM_Dash")))
	{
		BuildMontage(TEXT("AM_Dash"), TEXT("DefaultSlot"), { Dash }, { TEXT("Default") }, false);
		const FTransform RM = Dash->ExtractRootMotionFromRange(0.0, Dash->GetPlayLength(), FAnimExtractContext());
		UE_LOG(LogGlossaryAnim, Display, TEXT("[Glossary] MM_Dash bEnableRootMotion=%d 길이 %.2fs 루트 이동 %s"),
			Dash->bEnableRootMotion ? 1 : 0, Dash->GetPlayLength(), *RM.GetTranslation().ToString());
	}

	BuildCurves();
	BuildSounds();
	BuildSequence();
}
#endif

class FGlossaryAnimationModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::RegisterBuilder(TEXT("03_Animation"), &BuildGlossaryAnimationAssets);
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::UnregisterBuilder(TEXT("03_Animation"));
#endif
	}
};

IMPLEMENT_MODULE(FGlossaryAnimationModule, GlossaryAnimation);
