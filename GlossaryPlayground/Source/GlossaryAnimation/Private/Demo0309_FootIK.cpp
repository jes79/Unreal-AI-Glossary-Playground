#include "Demo0309_FootIK.h"

#include "Components/SkeletalMeshComponent.h"
#include "D03_AnimInstance.h"
#include "D03_Mannequin.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0309_FootIK::ADemo0309_FootIK()
{
	DemoId = TEXT("0309");
	Controls = TEXT("1 : 가운데 캐릭터·뒤쪽 걷는 캐릭터의 Foot IK ON/OFF\n")
		TEXT("관찰: 왼쪽(IK 없음)은 오른발이 계단 높이만큼 공중에 뜬다. 가운데는 골반을 내리고 무릎을 굽혀\n")
		TEXT("두 발이 각각 자기 바닥에 닿는다. 오른쪽은 템플릿 ABP_Unarmed 의 Control Rig Foot IK");
	Showcase = TEXT("계단에 걸친 Manny 3명(IK 없음 / Line Trace+Two Bone IK / ABP_Unarmed Control Rig) 발 높이 비교, 계단을 오르내리는 IK 캐릭터");
	PlayerPawnClass = nullptr;
}

void ADemo0309_FootIK::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 스테이션 3개: 캐릭터는 -X(카메라)를 보고, 왼발 쪽(+Y)에 StepHeight 높이 계단
	const float Ys[] = { -110.f, 110.f, 330.f };
	AD03_Mannequin* Made[3] = {};
	for (int32 i = 0; i < 3; ++i)
	{
		SpawnBlock(World, FVector(0.f, Ys[i] + 50.f, 0.f), FVector(160.f, 100.f, StepHeight), Colors::Platform);
		Made[i] = D03::SpawnMannequin(World, FVector(0.f, Ys[i] + 10.f, StepHeight), 180.f);
		if (Made[i])
		{
			Made[i]->GetCharacterMovement()->PerchRadiusThreshold = 0.f;
		}
	}
	NoIK = Made[0];
	CppIK = Made[1];
	RigIK = Made[2];
	if (NoIK)
	{
		if (UD03_AnimInstance* A = NoIK->UseCppAnimInstance()) { A->bFootIK = false; }
		NoIK->SetHeadLabel(TEXT("① IK 없음"), Colors::Enemy, 17.f);
	}
	if (CppIK)
	{
		CppAnim = CppIK->UseCppAnimInstance();
		if (CppAnim)
		{
			CppAnim->bFootIK = true;
			CppAnim->FootTraceUp = TraceUp;
			CppAnim->FootTraceDown = TraceDown;
		}
	}
	if (RigIK)
	{
		RigIK->SetHeadLabel(TEXT("③ ABP_Unarmed\n(CR_Mannequin_FootIK)"), Colors::Cyan, 15.f);
	}

	// 뒤쪽 계단 (한 칸 15cm 피라미드) 을 오르내리는 IK 캐릭터
	const float Rise = 15.f;
	for (int32 k = -3; k <= 3; ++k)
	{
		const float H = Rise * (4 - FMath::Abs(k));
		SpawnBlock(World, FVector(380.f, k * 60.f, 0.f), FVector(200.f, 60.f, H), Colors::Wall);
	}
	Walker = D03::SpawnMannequin(World, FVector(380.f, -420.f, 0.f), 90.f);
	if (Walker)
	{
		WalkerAnim = Walker->UseCppAnimInstance();
		if (WalkerAnim) { WalkerAnim->bFootIK = true; }
		Walker->SetPatrol(FVector(380.f, -420.f, 0.f), FVector(380.f, 420.f, 0.f));
		Walker->AutoMoveScale = 0.35f;
	}

	D03::SpawnViewCamera(World, GetPC(), FVector(-700.f, 60.f, 240.f), FVector(80.f, 60.f, 50.f), 62.f);
}

void ADemo0309_FootIK::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One))
	{
		bIK = !bIK;
	}
	if (CppAnim) { CppAnim->bFootIK = bIK; }
	if (WalkerAnim) { WalkerAnim->bFootIK = bIK; }
	if (CppIK)
	{
		CppIK->SetHeadLabel(FString::Printf(TEXT("② Line Trace + Two Bone IK %s"), bIK ? TEXT("ON") : TEXT("OFF")), Colors::Ally, 16.f);
	}
	if (Walker)
	{
		Walker->SetHeadLabel(FString::Printf(TEXT("계단 걷기 · IK %s"), bIK ? TEXT("ON") : TEXT("OFF")), FLinearColor::White, 14.f);
	}
	// 발 높이 확인: 각 캐릭터의 왼발·오른발 뼈 높이
	auto Feet = [](AD03_Mannequin* M)
	{
		if (!M) { return FString(TEXT("-")); }
		const float L = M->GetMesh()->GetSocketLocation(TEXT("foot_l")).Z;
		const float R = M->GetMesh()->GetSocketLocation(TEXT("foot_r")).Z;
		return FString::Printf(TEXT("왼발 %.0f · 오른발 %.0f cm"), L, R);
	};
	SetStatus(FString::Printf(TEXT("① IK 없음   %s\n② C++ IK %s  %s · 골반 %.1f · 발 오프셋 L %.1f R %.1f\n③ ABP_Unarmed %s"),
		*Feet(NoIK), bIK ? TEXT("ON ") : TEXT("OFF"), *Feet(CppIK),
		CppAnim ? CppAnim->PelvisOffset : 0.f, CppAnim ? CppAnim->FootOffsetL : 0.f, CppAnim ? CppAnim->FootOffsetR : 0.f, *Feet(RigIK)));
}
