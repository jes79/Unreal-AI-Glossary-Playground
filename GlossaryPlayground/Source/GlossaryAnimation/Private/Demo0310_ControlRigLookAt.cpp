#include "Demo0310_ControlRigLookAt.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "D03_AnimInstance.h"
#include "D03_Mannequin.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0310_ControlRigLookAt::ADemo0310_ControlRigLookAt()
{
	DemoId = TEXT("0310");
	Controls = TEXT("1 : Look At ON/OFF   2 : 회전 한계 60° ↔ 180°\n")
		TEXT("관찰: 머리가 가장 가까운 구(노란색으로 바뀜)를 본다. 구가 등 뒤로 돌아가면 60° 에서 멈추고(빨간 선 = 한계)\n")
		TEXT("한계를 180° 로 풀면 목이 비정상적으로 꺾인다");
	Showcase = TEXT("가장 가까운 타겟 선택 → 머리·목 Aim, 60° 회전 한계(콘 표시)와 한계 해제 비교");
	Implementation = EGlossaryImplementation::Alternative;
	AlternativeNote = TEXT("Control Rig 그래프(Aim 노드)는 에디터에서만 만들 수 있다. 같은 계산(타겟 방향 → 목 40%·머리 60% 회전, 60° Clamp, 보간)을 ")
		TEXT("C++ AnimInstance 의 컴포넌트 공간 후처리로 구현. 실제로는 Control Rig 에셋에 Aim 노드(Primary Axis=머리 앞축, Target=타겟 위치 변수)를 ")
		TEXT("놓고 AnimBP 의 Control Rig 노드에서 타겟 위치를 넘긴다.");
	PlayerPawnClass = nullptr;
}

void ADemo0310_ControlRigLookAt::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	Npc = D03::SpawnMannequin(World, FVector::ZeroVector, 180.f);
	if (Npc)
	{
		NpcAnim = Npc->UseCppAnimInstance();
		if (NpcAnim) { NpcAnim->bLookAt = true; }
	}
	for (int32 i = 0; i < 3; ++i)
	{
		AStaticMeshActor* T = SpawnShape(World, EShape::Sphere, FVector(-300.f, 0.f, 160.f), FVector(28.f), Colors::Neutral);
		if (T) { T->SetActorEnableCollision(false); }
		Targets.Add(T);
	}
	D03::SpawnViewCamera(World, GetPC(), FVector(-430.f, -330.f, 260.f), FVector(0.f, 0.f, 130.f), 60.f);
}

void ADemo0310_ControlRigLookAt::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	if (WasKeyPressed(EKeys::One)) { bEnabled = !bEnabled; }
	if (WasKeyPressed(EKeys::Two)) { bLimit = !bLimit; }
	if (!Npc || !NpcAnim)
	{
		return;
	}
	// 타겟 세 개가 서로 다른 반지름·속도로 캐릭터 주위를 돈다
	const FVector Center = Npc->GetActorLocation();
	const float Radii[] = { 250.f, 420.f, 600.f };
	const float Speeds[] = { 0.55f, -0.35f, 0.22f };
	const float Heights[] = { 70.f, 120.f, -20.f };
	int32 Nearest = INDEX_NONE;
	float NearestDist = LookRadius;
	for (int32 i = 0; i < Targets.Num(); ++i)
	{
		if (!Targets[i]) { continue; }
		const float A = Time * Speeds[i] + i * 2.1f;
		Targets[i]->SetActorLocation(Center + FVector(FMath::Cos(A) * Radii[i], FMath::Sin(A) * Radii[i], Heights[i]));
		const float D = FVector::Dist(Targets[i]->GetActorLocation(), Center);
		if (D < NearestDist) { NearestDist = D; Nearest = i; }
	}
	for (int32 i = 0; i < Targets.Num(); ++i)
	{
		if (Targets[i]) { SetColor(Targets[i]->GetStaticMeshComponent(), i == Nearest ? Colors::Item : Colors::Neutral, 0.4f, 0.f, i == Nearest ? 3.f : 0.f); }
	}

	// Aim 입력: 가장 가까운 타겟 위치, 회전 한계
	NpcAnim->bLookAt = bEnabled && Nearest != INDEX_NONE;
	NpcAnim->LookAtClamp = bLimit ? RotationLimit : 180.f;
	if (Nearest != INDEX_NONE)
	{
		NpcAnim->LookAtTarget = Targets[Nearest]->GetActorLocation();
	}

	// 한계 콘(빨강)과 현재 시선(초록) 표시
	const FVector Head = Npc->GetMesh()->GetSocketLocation(TEXT("head"));
	const FVector Fwd = Npc->GetActorForwardVector();
	// 수평면의 ±한계 각도 두 선 + 그 사이 호
	{
		const float Lim = FMath::Min(NpcAnim->LookAtClamp, 179.f);
		FVector Prev = FVector::ZeroVector;
		for (int32 k = 0; k <= 12; ++k)
		{
			const float A = FMath::Lerp(-Lim, Lim, k / 12.f);
			const FVector P = Head + FRotator(0.f, A, 0.f).RotateVector(Fwd) * 110.f;
			if (k == 0 || k == 12) { DrawDebugLine(GetWorld(), Head, P, FColor(255, 60, 40), false, -1.f, 0, 2.f); }
			if (k > 0) { DrawDebugLine(GetWorld(), Prev, P, FColor(255, 60, 40), false, -1.f, 0, 1.f); }
			Prev = P;
		}
	}
	if (NpcAnim->bLookAt)
	{
		const FVector Look = NpcAnim->LookDeltaCS.IsIdentity() ? Fwd : Npc->GetMesh()->GetComponentQuat().RotateVector(NpcAnim->LookDeltaCS.RotateVector(NpcAnim->ForwardCS));
		DrawDebugLine(GetWorld(), Head, Head + Look * 220.f, FColor(40, 255, 80), false, -1.f, 0, 2.5f);
		DrawDebugLine(GetWorld(), Head, NpcAnim->LookAtTarget, FColor(255, 220, 40), false, -1.f, 0, 0.8f);
	}
	const bool bClamped = NpcAnim->LookAtDesiredAngle > NpcAnim->LookAtClamp;
	Npc->SetHeadLabel(bClamped ? TEXT("한계 도달!") : TEXT("바라보는 중"), bClamped ? Colors::Enemy : Colors::Ally, 16.f);
	SetStatus(FString::Printf(TEXT("Look At %s · 회전 한계 %.0f°\n가장 가까운 타겟 #%d (%.0f cm) · 필요한 각도 %.0f° → 실제 회전 %.0f°%s"),
		bEnabled ? TEXT("ON") : TEXT("OFF"), NpcAnim->LookAtClamp, Nearest + 1, NearestDist, NpcAnim->LookAtDesiredAngle, NpcAnim->LookAtAppliedAngle,
		bClamped ? TEXT(" (Clamp)") : TEXT("")));
}
