#include "Demo0311_IKRetargeter.h"

#include "Components/SkeletalMeshComponent.h"
#include "D03_AnimInstance.h"
#include "D03_Mannequin.h"
#include "Engine/SkeletalMesh.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"

using namespace GlossaryKit;

ADemo0311_IKRetargeter::ADemo0311_IKRetargeter()
{
	DemoId = TEXT("0311");
	Controls = TEXT("1 : 내 캐릭터 다리 배율 1.35 ↔ 0.75   2 : 걷기 ↔ 조깅\n")
		TEXT("관찰: 같은 애니메이션을 그대로 복사하면(③) 다리 길이가 달라 발이 바닥을 뚫거나 뜬다.\n")
		TEXT("리타깃(④)은 골반(Retarget Root) 높이를 다리 비율만큼 옮겨 발이 바닥에 맞는다");
	Showcase = TEXT("소스 Manny·같은 스켈레톤 Quinn·다리 긴 캐릭터에 그대로 복사·골반 비율 보정 리타깃 4명 나란히 비교");
	Implementation = EGlossaryImplementation::Alternative;
	AlternativeNote = TEXT("IK Rig·IK Retargeter 에셋은 에디터에서 체인을 매핑해 만들고(Spine·Head·LeftArm·RightArm·LeftLeg·RightLeg + Retarget Root=pelvis), ")
		TEXT("다른 스켈레톤 메시가 필요하다. 이 프로젝트에는 Manny 스켈레톤만 있어 '다른 체형'을 다리 뼈 길이 배율로 만들고, ")
		TEXT("Retargeter 가 하는 핵심 보정(뼈 회전은 복사, 골반 이동은 다리 길이 비율로 스케일)을 C++ AnimInstance 로 재현했다. ")
		TEXT("실제 절차: 소스·타깃 IK Rig 생성 → Retarget Chain 지정 → IK Retargeter 에서 Auto Map Chains → Export Animations.");
	PlayerPawnClass = nullptr;
}

void ADemo0311_IKRetargeter::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	USkeletalMesh* Quinn = LoadAsset<USkeletalMesh>(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple"));

	struct FSetup { const TCHAR* Label; FLinearColor Color; bool bQuinn; float Leg; bool bPelvis; };
	const FSetup Setups[] = {
		{ TEXT("① 소스: Manny"), FLinearColor::White, false, 1.f, true },
		{ TEXT("② 같은 스켈레톤: Quinn\n(그대로 재생 가능)"), Colors::Cyan, true, 1.f, true },
		{ TEXT("③ 다른 체형에 그대로 복사"), Colors::Enemy, false, TargetLegScale, false },
		{ TEXT("④ 리타깃 (골반 높이 보정)"), Colors::Ally, false, TargetLegScale, true },
	};
	for (int32 i = 0; i < 4; ++i)
	{
		AD03_Mannequin* M = D03::SpawnMannequin(World, FVector(-360.f + i * 240.f, 0.f, 0.f), 0.f);
		if (!M) { continue; }
		if (Setups[i].bQuinn && Quinn)
		{
			M->GetMesh()->SetSkeletalMesh(Quinn);
		}
		UD03_AnimInstance* A = M->UseCppAnimInstance();
		if (A)
		{
			A->bAutoLocomotion = false;     // 제자리에서 같은 Speed 로 걷기 재생
			A->Speed = PlaySpeed;
			A->LegScale = Setups[i].Leg;
			A->bScalePelvisHeight = Setups[i].bPelvis;
		}
		M->SetHeadLabel(Setups[i].Label, Setups[i].Color, 15.f);
		if (M->HeadLabel) { M->HeadLabel->SetActorRelativeLocation(FVector(0, 0, 150.f)); }
		Actors.Add(M);
		Anims.Add(A);
	}
	// 바닥 기준선
	SpawnShape(World, EShape::Cube, FVector(0.f, 0.f, 1.f), FVector(1100.f, 6.f, 2.f), Colors::Item);
	D03::SpawnViewCamera(World, GetPC(), FVector(0.f, -760.f, 150.f), FVector(0.f, 0.f, 95.f), 70.f);
}

void ADemo0311_IKRetargeter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One))
	{
		TargetLegScale = TargetLegScale > 1.f ? 0.75f : 1.35f;
	}
	if (WasKeyPressed(EKeys::Two))
	{
		PlaySpeed = PlaySpeed < 400.f ? 500.f : 300.f;
	}
	for (int32 i = 0; i < Anims.Num(); ++i)
	{
		if (!Anims[i]) { continue; }
		Anims[i]->Speed = PlaySpeed;
		if (i >= 2) { Anims[i]->LegScale = TargetLegScale; }
	}
	auto FootZ = [this](int32 i) -> float
	{
		if (!Actors.IsValidIndex(i) || !Actors[i]) { return 0.f; }
		const USkeletalMeshComponent* Mesh = Actors[i]->GetMesh();
		return FMath::Min(Mesh->GetSocketLocation(TEXT("ball_l")).Z, Mesh->GetSocketLocation(TEXT("ball_r")).Z);
	};
	SetStatus(FString::Printf(TEXT("다리 배율 %.2f · Speed %.0f\n가장 낮은 발가락 높이: ① %.0f  ② %.0f  ③ %.0f  ④ %.0f cm (0 근처가 정상)"),
		TargetLegScale, PlaySpeed, FootZ(0), FootZ(1), FootZ(2), FootZ(3)));
}
