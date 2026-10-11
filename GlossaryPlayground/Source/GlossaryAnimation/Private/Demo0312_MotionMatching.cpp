#include "Demo0312_MotionMatching.h"

#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "Animation/Skeleton.h"
#include "D03_AnimInstance.h"
#include "D03_GraphWidget.h"
#include "D03_Mannequin.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0312_MotionMatching::ADemo0312_MotionMatching()
{
	DemoId = TEXT("0312");
	Controls = TEXT("1 : Motion Matching ↔ 2D Blend Space 비교\n")
		TEXT("관찰: NPC 가 무작위 지점으로 걷기·조깅하며 몸 방향은 거의 고정 → 앞·옆·뒤 클립이 골고루 필요하다.\n")
		TEXT("0.1초마다 '원하는 속도 + 지금 발 위치·속도' 가 가장 비슷한 프레임을 데이터베이스에서 골라 넘어간다");
	Showcase = TEXT("Idle·8방향 걷기·조깅 17클립을 프레임 단위 포즈 DB 로 만들고 0.1초마다 속도+발 특징 비용 최소 프레임 검색, 후보·비용 표시, Blend Space 와 비교");
	Implementation = EGlossaryImplementation::Alternative;
	AlternativeNote = TEXT("PoseSearch 플러그인은 켜져 있지만 Pose Search Schema·Database 에셋과 Motion Matching 노드는 AnimBP·에디터 인덱싱이 필요하다. ")
		TEXT("같은 원리(특징 벡터 = 궤적 속도 + 발 위치·속도, 가중 거리 최소 프레임 검색, 전환 시 블렌드, 현재 클립 유지 보너스)를 C++ 로 직접 구현했다. ")
		TEXT("실제 프로젝트: Game Animation Sample 처럼 PSS_ 스키마(Trajectory·Pose 채널) + PSD_ 데이터베이스 + AnimBP 의 Motion Matching 노드 + Character Trajectory 컴포넌트.");
	PlayerPawnClass = nullptr;
}

FVector ADemo0312_MotionMatching::BoneCS(const UAnimSequence* Seq, int32 Bone, float T) const
{
	const FReferenceSkeleton& Ref = Seq->GetSkeleton()->GetReferenceSkeleton();
	FTransform Result = FTransform::Identity;
	for (int32 B = Bone; B != INDEX_NONE; B = Ref.GetParentIndex(B))
	{
		FTransform Local;
		Seq->GetBoneTransform(Local, FSkeletonPoseBoneIndex(B), FAnimExtractContext(static_cast<double>(T)), false);
		Result = Result * Local;
	}
	return Result.GetLocation();
}

void ADemo0312_MotionMatching::BuildDatabase()
{
	// 클립과 그 클립이 나타내는 이동 속도(캐릭터 기준). 애니메이션 이름의 방향과 걷기 200 · 조깅 500
	struct FDef { const TCHAR* Path; float Angle; float Speed; };
	const FDef Defs[] = {
		{ TEXT("Unarmed/MM_Idle"), 0.f, 0.f },
		{ TEXT("Unarmed/Walk/MF_Unarmed_Walk_Fwd"), 0.f, 200.f }, { TEXT("Unarmed/Walk/MF_Unarmed_Walk_Fwd_Right"), 45.f, 200.f },
		{ TEXT("Unarmed/Walk/MF_Unarmed_Walk_Right"), 90.f, 200.f }, { TEXT("Unarmed/Walk/MF_Unarmed_Walk_Bwd_Right"), 135.f, 200.f },
		{ TEXT("Unarmed/Walk/MF_Unarmed_Walk_Bwd"), 180.f, 200.f }, { TEXT("Unarmed/Walk/MF_Unarmed_Walk_Bwd_Left"), -135.f, 200.f },
		{ TEXT("Unarmed/Walk/MF_Unarmed_Walk_Left"), -90.f, 200.f }, { TEXT("Unarmed/Walk/MF_Unarmed_Walk_Fwd_Left"), -45.f, 200.f },
		{ TEXT("Unarmed/Jog/MF_Unarmed_Jog_Fwd"), 0.f, 500.f }, { TEXT("Unarmed/Jog/MF_Unarmed_Jog_Fwd_Right"), 45.f, 500.f },
		{ TEXT("Unarmed/Jog/MF_Unarmed_Jog_Right"), 90.f, 500.f }, { TEXT("Unarmed/Jog/MF_Unarmed_Jog_Bwd_Right"), 135.f, 500.f },
		{ TEXT("Unarmed/Jog/MF_Unarmed_Jog_Bwd"), 180.f, 500.f }, { TEXT("Unarmed/Jog/MF_Unarmed_Jog_Bwd_Left"), -135.f, 500.f },
		{ TEXT("Unarmed/Jog/MF_Unarmed_Jog_Left"), -90.f, 500.f }, { TEXT("Unarmed/Jog/MF_Unarmed_Jog_Fwd_Left"), -45.f, 500.f },
	};
	const float Step = 1.f / 15.f;
	for (const FDef& D : Defs)
	{
		UAnimSequence* Seq = D03::LoadAnim(D.Path);
		if (!Seq || !Seq->GetSkeleton())
		{
			continue;
		}
		const int32 ClipIndex = Clips.Num();
		FString Name = FPaths::GetBaseFilename(D.Path).Replace(TEXT("MF_Unarmed_"), TEXT(""));
		const float Rad = FMath::DegreesToRadians(D.Angle);
		Clips.Add({ Seq, Name, FVector2D(FMath::Cos(Rad), FMath::Sin(Rad)) * D.Speed });
		ClipAssets.Add(Seq);

		const FReferenceSkeleton& Ref = Seq->GetSkeleton()->GetReferenceSkeleton();
		const int32 FL = Ref.FindBoneIndex(TEXT("foot_l"));
		const int32 FR = Ref.FindBoneIndex(TEXT("foot_r"));
		const float Len = Seq->GetPlayLength();
		for (float T = 0.f; T < Len; T += Step)
		{
			FFrame F;
			F.Clip = ClipIndex;
			F.Time = T;
			F.Velocity = Clips[ClipIndex].Velocity;
			F.FootL = BoneCS(Seq, FL, T);
			F.FootR = BoneCS(Seq, FR, T);
			const float T2 = FMath::Fmod(T + Step, Len);
			F.FootVelL = (BoneCS(Seq, FL, T2) - F.FootL) / Step;
			F.FootVelR = (BoneCS(Seq, FR, T2) - F.FootR) / Step;
			Database.Add(F);
		}
	}
}

int32 ADemo0312_MotionMatching::FindFrame(int32 Clip, float T) const
{
	int32 Best = INDEX_NONE;
	float BestD = FLT_MAX;
	for (int32 i = 0; i < Database.Num(); ++i)
	{
		if (Database[i].Clip == Clip)
		{
			const float D = FMath::Abs(Database[i].Time - T);
			if (D < BestD) { BestD = D; Best = i; }
		}
	}
	return Best;
}

void ADemo0312_MotionMatching::Search(const FVector2D& Desired)
{
	const int32 CurIndex = FindFrame(CurClip, CurTime);
	if (CurIndex == INDEX_NONE)
	{
		return;
	}
	const FFrame& Cur = Database[CurIndex];

	// 모든 프레임 비용 = 속도 차이 + 포즈(발 위치·발 속도) 차이 — Pose Search 의 Trajectory 채널 + Pose 채널
	struct FCand { int32 Index; float Cost; };
	TArray<FCand> Cands;
	Cands.Reserve(Database.Num());
	for (int32 i = 0; i < Database.Num(); ++i)
	{
		const FFrame& F = Database[i];
		const float CostV = (F.Velocity - Desired).SizeSquared() / FMath::Square(200.f);
		const float CostP = ((F.FootL - Cur.FootL).SizeSquared() + (F.FootR - Cur.FootR).SizeSquared()) / FMath::Square(25.f)
			+ ((F.FootVelL - Cur.FootVelL).SizeSquared() + (F.FootVelR - Cur.FootVelR).SizeSquared()) / FMath::Square(300.f);
		float Cost = CostV * VelocityWeight + CostP * PoseWeight;
		if (F.Clip == CurClip && FMath::Abs(F.Time - CurTime) < 0.15f)
		{
			Cost -= ContinuingBias;   // 지금 재생 중인 흐름을 이어 가면 보너스
		}
		Cands.Add({ i, Cost });
	}
	Cands.Sort([](const FCand& A, const FCand& B) { return A.Cost < B.Cost; });

	TopCandidates.Reset();
	TSet<int32> Shown;
	for (const FCand& C : Cands)
	{
		const FFrame& F = Database[C.Index];
		if (Shown.Contains(F.Clip)) { continue; }
		Shown.Add(F.Clip);
		TopCandidates += FString::Printf(TEXT("%s %.2fs (비용 %.2f)   "), *Clips[F.Clip].Name, F.Time, C.Cost);
		if (Shown.Num() >= 3) { break; }
	}

	const FFrame& Best = Database[Cands[0].Index];
	LastCost = Cands[0].Cost;
	if (Best.Clip == CurClip && FMath::Abs(Best.Time - CurTime) < 0.2f)
	{
		return;   // 지금 프레임이 가장 좋다 — 계속 재생
	}
	// 새 프레임으로 넘어간다: 지금 포즈는 B 로 밀고 A 를 새 프레임으로, 0 → 1 블렌드
	PrevClip = CurClip;
	PrevTime = CurTime;
	CurClip = Best.Clip;
	CurTime = Best.Time;
	BlendAlpha = 0.f;
	++Switches;
}

void ADemo0312_MotionMatching::PickWaypoint()
{
	Waypoint = FVector(FMath::FRandRange(-250.f, 450.f), FMath::FRandRange(-450.f, 450.f), 0.f);
	WaypointSpeed = FMath::RandBool() ? 200.f : 500.f;
	FacingYaw = 180.f + FMath::FRandRange(-40.f, 40.f);
}

void ADemo0312_MotionMatching::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	FMath::RandInit(312);
	BuildDatabase();
	CompareBlendSpace = LoadAsset<UBlendSpace>(D03::Folder + TEXT("BS_Locomotion8Dir"));

	Npc = D03::SpawnMannequin(World, FVector(100.f, 0.f, 0.f), 180.f);
	if (Npc)
	{
		NpcAnim = Npc->UseCppAnimInstance();
		Npc->AutoMoveScale = 1.f;
	}
	PickWaypoint();
	D03::SpawnViewCamera(World, GetPC(), FVector(-850.f, 0.f, 520.f), FVector(150.f, 0.f, 40.f), 65.f);

	Graph = UD03_GraphWidget::Create(GetPC(), TEXT("데이터베이스 클립(속도 특징) · 노란 점 = 원하는 속도"));
	if (Graph)
	{
		Graph->XMin = -600.f; Graph->XMax = 600.f; Graph->YMin = -600.f; Graph->YMax = 600.f;
		Graph->GridX = 4; Graph->GridY = 4;
		Graph->XLabel = TEXT("오른쪽 속도");
		Graph->YLabel = TEXT("앞 속도");
		Graph->PanelSize = FVector2D(360.f, 340.f);
	}
}

void ADemo0312_MotionMatching::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	if (WasKeyPressed(EKeys::One))
	{
		bMotionMatching = !bMotionMatching;
	}
	if (!Npc || !NpcAnim || Clips.Num() == 0)
	{
		return;
	}

	// 이동: 지점에 도착하면 새 지점·속도·몸 방향
	FVector To = Waypoint - Npc->GetActorLocation();
	To.Z = 0.f;
	if (To.Size() < 60.f)
	{
		PickWaypoint();
	}
	Npc->AutoMoveDirection = To.GetSafeNormal();
	Npc->GetCharacterMovement()->MaxWalkSpeed = WaypointSpeed;
	Npc->SetFixedFacing(true, FacingYaw);

	// 원하는 속도(캐릭터 기준 x=앞, y=오른쪽): 입력 방향 × 목표 속도 — 미래 궤적의 가장 단순한 형태
	const FVector LocalDir = Npc->GetActorTransform().InverseTransformVectorNoScale(To.GetSafeNormal());
	const FVector2D Desired(LocalDir.X * WaypointSpeed, LocalDir.Y * WaypointSpeed);
	LastQuery = Desired;

	if (bMotionMatching)
	{
		// 재생 시간 진행 (루프)
		auto Advance = [&](int32 Clip, float& T)
		{
			if (Clips.IsValidIndex(Clip)) { T = FMath::Fmod(T + DeltaSeconds, FMath::Max(Clips[Clip].Seq->GetPlayLength(), 0.01f)); }
		};
		Advance(CurClip, CurTime);
		Advance(PrevClip, PrevTime);
		BlendAlpha = FMath::Min(1.f, BlendAlpha + DeltaSeconds / FMath::Max(BlendTime, 0.01f));

		SearchTimer += DeltaSeconds;
		if (SearchTimer >= SearchInterval)
		{
			SearchTimer = 0.f;
			Search(Desired);
		}
		NpcAnim->bPoseOverride = true;
		NpcAnim->PoseSeqA = Clips[CurClip].Seq;
		NpcAnim->PoseTimeA = CurTime;
		NpcAnim->PoseSeqB = Clips.IsValidIndex(PrevClip) ? Clips[PrevClip].Seq : nullptr;
		NpcAnim->PoseTimeB = PrevTime;
		NpcAnim->PoseAlphaA = BlendAlpha;
	}
	else
	{
		NpcAnim->bPoseOverride = false;
		NpcAnim->LocomotionBlendSpace = CompareBlendSpace;
	}

	if (Graph)
	{
		Graph->ClearDynamic();
		for (int32 i = 0; i < Clips.Num(); ++i)
		{
			const bool bCur = bMotionMatching && i == CurClip;
			Graph->Points.Add({ FVector2D(Clips[i].Velocity.Y, Clips[i].Velocity.X), bCur ? FLinearColor(0.2f, 1.f, 0.3f) : FLinearColor(0.55f, 0.65f, 0.85f),
				bCur ? Clips[i].Name : TEXT(""), bCur ? 12.f : 8.f });
		}
		Graph->bShowMarker = true;
		Graph->Marker = FVector2D(Desired.Y, Desired.X);
	}
	Npc->SetHeadLabel(bMotionMatching ? Clips[CurClip].Name : TEXT("Blend Space"), Colors::Item, 15.f);
	if (bMotionMatching)
	{
		SetStatus(FString::Printf(TEXT("Motion Matching · DB %d 프레임 / %d 클립 · 전환 %d회\n재생: %s %.2fs (비용 %.2f, 블렌드 %.0f%%) · 원하는 속도 앞 %.0f 오른쪽 %.0f\n후보: %s"),
			Database.Num(), Clips.Num(), Switches, *Clips[CurClip].Name, CurTime, LastCost, BlendAlpha * 100.f, Desired.X, Desired.Y, *TopCandidates));
	}
	else
	{
		SetStatus(FString::Printf(TEXT("비교: 2D Blend Space (BS_Locomotion8Dir) — 샘플을 섞어서 만든 포즈\n원하는 속도 앞 %.0f 오른쪽 %.0f"), Desired.X, Desired.Y));
	}
}
