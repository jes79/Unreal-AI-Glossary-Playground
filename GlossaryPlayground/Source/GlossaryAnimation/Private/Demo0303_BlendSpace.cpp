#include "Demo0303_BlendSpace.h"

#include "Animation/BlendSpace.h"
#include "D03_AnimInstance.h"
#include "D03_GraphWidget.h"
#include "D03_Mannequin.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GlossaryKit.h"
#include "KismetAnimationLibrary.h"

using namespace GlossaryKit;

namespace
{
	const FVector CircleCenter(300.f, 0.f, 0.f);
}

ADemo0303_BlendSpace::ADemo0303_BlendSpace()
{
	DemoId = TEXT("0303");
	Controls = TEXT("1 : 걷기 속도 200   2 : 조깅 속도 500   3 : 자동(200↔500)\n")
		TEXT("C : Blend Space 섞기 ON/OFF (OFF = 가장 가까운 샘플 하나로 뚝뚝 바뀜)\n")
		TEXT("관찰: 몸은 카메라를 본 채 원을 돌아서 Direction 이 -180~180 을 돈다. 노란 점 = 지금 입력 위치");
	Showcase = TEXT("코드로 만든 2D Blend Space(Direction×Speed, 8방향 걷기·조깅 17샘플)를 NPC 옆·뒤 걸음에 적용, 격자 위 입력 위치 표시, 스냅과 비교");
	PlayerPawnClass = nullptr;
}

void ADemo0303_BlendSpace::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	BlendSpace = LoadAsset<UBlendSpace>(D03::Folder + TEXT("BS_Locomotion8Dir"));
	Npc = D03::SpawnMannequin(World, CircleCenter + FVector(-CircleRadius, 0.f, 0.f), 180.f);
	if (Npc)
	{
		NpcAnim = Npc->UseCppAnimInstance();
		if (NpcAnim)
		{
			NpcAnim->LocomotionBlendSpace = BlendSpace;   // 2D: X=Direction, Y=Speed
			NpcAnim->bAutoLocomotion = false;             // 입력값은 아래 Tick 에서 직접 넣는다(스냅 비교 때문)
		}
		Npc->SetFixedFacing(true, 180.f);                 // 카메라 쪽을 본 채 이동 → 옆·뒤 걸음
	}
	// 원 경로 표시
	for (int32 i = 0; i < 24; ++i)
	{
		const float A = 2.f * PI * i / 24.f;
		SpawnShape(World, EShape::Cylinder, CircleCenter + FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * CircleRadius + FVector(0, 0, 1), FVector(14, 14, 2), Colors::Cyan);
	}
	D03::SpawnViewCamera(World, GetPC(), FVector(-760.f, 0.f, 480.f), CircleCenter + FVector(0, 0, 40.f), 62.f);

	Graph = UD03_GraphWidget::Create(GetPC(), TEXT("BS_Locomotion8Dir (2D Blend Space)"));
	if (Graph)
	{
		Graph->XMin = -180.f; Graph->XMax = 180.f; Graph->YMin = 0.f; Graph->YMax = 600.f;
		Graph->GridX = 8; Graph->GridY = 6;
		Graph->XLabel = TEXT("Direction (°)");
		Graph->YLabel = TEXT("Speed");
		Graph->PanelSize = FVector2D(520.f, 330.f);
	}
}

void ADemo0303_BlendSpace::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;

	if (WasKeyPressed(EKeys::One)) { SpeedMode = 1; }
	if (WasKeyPressed(EKeys::Two)) { SpeedMode = 2; }
	if (WasKeyPressed(EKeys::Three)) { SpeedMode = 3; }
	if (WasKeyPressed(EKeys::C)) { bBlend = !bBlend; }

	if (!Npc || !NpcAnim)
	{
		return;
	}
	// 원 둘레 접선 방향으로 이동 입력 (+ 반지름 보정)
	FVector ToNpc = Npc->GetActorLocation() - CircleCenter;
	ToNpc.Z = 0.f;
	const float R = ToNpc.Size();
	const FVector Radial = ToNpc.GetSafeNormal();
	const FVector Tangent(-Radial.Y, Radial.X, 0.f);
	Npc->AutoMoveDirection = (Tangent + Radial * (CircleRadius - R) * 0.01f).GetSafeNormal();
	const float TargetSpeed = SpeedMode == 1 ? 200.f : SpeedMode == 2 ? 500.f : 350.f + 150.f * FMath::Sin(Time * 0.6f);
	Npc->GetCharacterMovement()->MaxWalkSpeed = TargetSpeed;

	// AnimBP 에서 Blend Space Player 의 X·Y 핀에 넣는 값
	const FVector Velocity = Npc->GetVelocity();
	float Speed = Velocity.Size2D();
	float Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, Npc->GetActorRotation());
	if (!bBlend)
	{
		// 비교: 가장 가까운 샘플로 스냅 → 사이 값이 없어서 방향·속도가 바뀔 때 포즈가 툭툭 끊긴다
		Direction = FMath::RoundToFloat(Direction / 45.f) * 45.f;
		Speed = Speed < 100.f ? 0.f : (Speed < 350.f ? 200.f : 500.f);
	}
	NpcAnim->Speed = Speed;
	NpcAnim->Direction = Direction;

	if (Graph && BlendSpace)
	{
		Graph->ClearDynamic();
		BlendSpace->ForEachImmutableSample([this](const FBlendSample& S)
		{
			const bool bJog = S.SampleValue.Y > 300.f;
			const bool bIdle = S.SampleValue.Y < 1.f;
			const FLinearColor C = bIdle ? FLinearColor(0.7f, 0.7f, 0.7f) : bJog ? FLinearColor(1.f, 0.5f, 0.15f) : FLinearColor(0.3f, 0.9f, 0.4f);
			Graph->Points.Add({ FVector2D(S.SampleValue.X, S.SampleValue.Y), C, TEXT(""), 9.f });
		});
		Graph->Texts.Add({ FVector2D(-178.f, 250.f), TEXT("Walk 8방향"), FLinearColor(0.3f, 0.9f, 0.4f), 10.f });
		Graph->Texts.Add({ FVector2D(-178.f, 550.f), TEXT("Jog 8방향"), FLinearColor(1.f, 0.5f, 0.15f), 10.f });
		Graph->bShowMarker = true;
		Graph->Marker = FVector2D(Direction, Speed);
	}
	SetStatus(FString::Printf(TEXT("Blend Space 입력  Direction %.0f° · Speed %.0f  (%s)\n목표 속도 %.0f · 모드 %s"),
		Direction, Speed, bBlend ? TEXT("섞기 ON") : TEXT("스냅 — 섞지 않음"), TargetSpeed,
		SpeedMode == 1 ? TEXT("걷기") : SpeedMode == 2 ? TEXT("조깅") : TEXT("자동")));
}
