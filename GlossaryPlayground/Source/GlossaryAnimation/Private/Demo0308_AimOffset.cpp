#include "Demo0308_AimOffset.h"

#include "Components/SkeletalMeshComponent.h"
#include "D03_AnimInstance.h"
#include "D03_GraphWidget.h"
#include "D03_Mannequin.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

namespace
{
	const FVector NpcFeet(0.f, 250.f, 0.f);
}

ADemo0308_AimOffset::ADemo0308_AimOffset()
{
	DemoId = TEXT("0308");
	Controls = TEXT("마우스 : 카메라(조준) 방향 · WASD 이동 · 1 : Aim Offset ON/OFF\n")
		TEXT("관찰: 멈춰 서서 카메라를 돌리면 플레이어 상체·머리가 카메라 방향을 따라 돈다 (±90° 까지)\n")
		TEXT("NPC 는 움직이는 노란 구를 조준 — 오른쪽 아래 Yaw×Pitch 격자의 점이 지금 조준값");
	Showcase = TEXT("Pitch·Yaw(-90~90) 조준값을 척추 5개에 나눠 회전, 카메라를 따라가는 플레이어 상체와 움직이는 표적을 조준하는 NPC");
	Implementation = EGlossaryImplementation::Alternative;
	AlternativeNote = TEXT("Aim Offset 에셋은 조준 포즈 9장(CU·CC·CD × L·C·R)이 필요한데 Unarmed 애니메이션 세트에 없다. ")
		TEXT("같은 원리(조준 Pitch·Yaw → 척추 회전, 다리는 그대로)를 C++ AnimInstance 의 컴포넌트 공간 회전으로 구현. ")
		TEXT("실제로는 AO 에셋을 만들고 AnimBP 에 Aim Offset 노드를 Pitch·Yaw 핀으로 연결한다(Rifle 폴더의 AO_Rifle 참고).");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-500.f, -150.f, 100.f));
}

void ADemo0308_AimOffset::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	Npc = D03::SpawnMannequin(World, NpcFeet, 180.f);
	if (Npc)
	{
		NpcAnim = Npc->UseCppAnimInstance();
		if (NpcAnim) { NpcAnim->bAimOffset = true; }
	}
	if (ACharacter* P = GetPlayerCharacter())
	{
		P->GetMesh()->SetAnimInstanceClass(UD03_AnimInstance::StaticClass());
		PlayerAnim = Cast<UD03_AnimInstance>(P->GetMesh()->GetAnimInstance());
		if (PlayerAnim) { PlayerAnim->bAimOffset = true; }
	}
	Target = SpawnShape(World, EShape::Sphere, NpcFeet + FVector(-300.f, 0.f, 150.f), FVector(30.f), Colors::Item);
	if (Target)
	{
		SetColor(Target->GetStaticMeshComponent(), Colors::Item, 0.4f, 0.f, 4.f);
		Target->SetActorEnableCollision(false);
	}
	Graph = UD03_GraphWidget::Create(GetPC(), TEXT("NPC 조준값 (Aim Offset 입력)"));
	if (Graph)
	{
		Graph->XMin = YawRange.X; Graph->XMax = YawRange.Y; Graph->YMin = PitchRange.X; Graph->YMax = PitchRange.Y;
		Graph->GridX = 4; Graph->GridY = 4;
		Graph->XLabel = TEXT("Yaw (°)");
		Graph->YLabel = TEXT("Pitch");
		Graph->PanelSize = FVector2D(360.f, 300.f);
	}
}

void ADemo0308_AimOffset::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	if (WasKeyPressed(EKeys::One))
	{
		bAimOffsetEnabled = !bAimOffsetEnabled;
	}

	// 표적: NPC 앞에서 8자 궤도 (Yaw ±80°, Pitch ±55° 정도를 훑는다)
	if (Target && Npc)
	{
		const float Yaw = 80.f * FMath::Sin(Time * 0.7f);
		const float Pitch = 55.f * FMath::Sin(Time * 1.4f);
		const FVector Head = Npc->GetActorLocation() + FVector(0, 0, 60.f);
		const FVector Dir = (Npc->GetActorRotation() + FRotator(Pitch, Yaw, 0.f)).Vector();
		Target->SetActorLocation(Head + Dir * 260.f);
	}

	// NPC: 표적 방향을 액터 기준 Yaw·Pitch 로 바꿔 Aim Offset 입력에 넣는다
	float NpcYaw = 0.f, NpcPitch = 0.f;
	if (NpcAnim && Npc && Target)
	{
		const FVector Head = Npc->GetActorLocation() + FVector(0, 0, 60.f);
		const FRotator Local = Npc->GetActorTransform().InverseTransformVectorNoScale(Target->GetActorLocation() - Head).Rotation();
		NpcYaw = FMath::Clamp(Local.Yaw, YawRange.X, YawRange.Y);
		NpcPitch = FMath::Clamp(Local.Pitch, PitchRange.X, PitchRange.Y);
		NpcAnim->bAimOffset = bAimOffsetEnabled;
		NpcAnim->AimYaw = NpcYaw;
		NpcAnim->AimPitch = NpcPitch;
	}

	// 플레이어: 카메라(Control Rotation) - 액터 회전 = 조준 오프셋 (AnimBP 에서 흔히 쓰는 Delta(Rotator) 계산)
	float PYaw = 0.f, PPitch = 0.f;
	if (PlayerAnim && GetPC() && GetPlayerCharacter())
	{
		const FRotator Delta = (GetPC()->GetControlRotation() - GetPlayerCharacter()->GetActorRotation()).GetNormalized();
		PYaw = FMath::Clamp(Delta.Yaw, YawRange.X, YawRange.Y);
		PPitch = FMath::Clamp(Delta.Pitch, PitchRange.X, PitchRange.Y);
		PlayerAnim->bAimOffset = bAimOffsetEnabled;
		PlayerAnim->AimYaw = PYaw;
		PlayerAnim->AimPitch = PPitch;
	}

	if (Graph)
	{
		Graph->ClearDynamic();
		for (float Y : { -90.f, 0.f, 90.f })
		{
			for (float P : { -90.f, 0.f, 90.f })
			{
				Graph->Points.Add({ FVector2D(Y, P), FLinearColor(0.5f, 0.6f, 0.8f), TEXT(""), 7.f });
			}
		}
		Graph->bShowMarker = true;
		Graph->Marker = FVector2D(NpcYaw, NpcPitch);
	}
	SetStatus(FString::Printf(TEXT("Aim Offset %s\nNPC     Yaw %+.0f° · Pitch %+.0f°\n플레이어 Yaw %+.0f° · Pitch %+.0f°  (카메라 - 몸 방향)"),
		bAimOffsetEnabled ? TEXT("ON") : TEXT("OFF"), NpcYaw, NpcPitch, PYaw, PPitch));
}
