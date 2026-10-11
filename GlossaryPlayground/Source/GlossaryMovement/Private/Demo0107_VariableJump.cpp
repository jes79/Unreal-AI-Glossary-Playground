#include "Demo0107_VariableJump.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GlossaryCharacter.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"

using namespace GlossaryKit;

ADemo0107_VariableJump::ADemo0107_VariableJump()
{
	DemoId = TEXT("0107");
	Controls = TEXT("WASD 이동 · Space 점프 (짧게 톡 / 길게 꾹) · C : 가변 점프 ON/OFF\n")
		TEXT("왼쪽 마네킹은 0.05초, 오른쪽은 0.3초 동안 자동으로 누른다\n")
		TEXT("관찰: ON 이면 누른 시간만큼 높이 뛴다(최고점 구슬·눈금). OFF(Max Hold 0)면 항상 같은 높이");
	Showcase = TEXT("Jump Max Hold Time 0.3초 — 짧게/길게 누른 점프의 최고점을 눈금과 구슬로 비교, ON/OFF");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-500.f, 0.f, 100.f));
}

void ADemo0107_VariableJump::Apply(ACharacter* Character) const
{
	if (!Character)
	{
		return;
	}
	Character->GetCharacterMovement()->JumpZVelocity = JumpZVelocity;
	// 핵심: 이 시간 동안 점프 버튼을 누르고 있으면 상승 속도가 계속 JumpZVelocity 로 유지된다
	Character->JumpMaxHoldTime = bVariableJump ? JumpMaxHoldTime : 0.f;
}

void ADemo0107_VariableJump::AddJumper(ACharacter* Character, const FString& Name, const FLinearColor& Color, float AutoHold)
{
	if (!Character)
	{
		return;
	}
	Apply(Character);
	FJumper J;
	J.Character = Character;
	J.Name = Name;
	J.Color = Color;
	J.AutoHold = AutoHold;
	J.Timer = AutoHold > 0.2f ? 1.0f : 0.f;   // 두 마네킹이 엇갈려 뛰도록
	if (AutoHold >= 0.f)
	{
		J.Label = SpawnLabel(GetWorld(), Name, Character->GetActorLocation() + FVector(0.f, 0.f, 150.f), Color, 16.f);
	}
	Jumpers.Add(J);
}

void ADemo0107_VariableJump::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 높이 눈금 기둥: 50cm 마다 가로줄, 100cm 마다 숫자
	for (const float Y : { -130.f, 130.f })
	{
		SpawnBlock(World, FVector(150.f, Y, 0.f), FVector(10.f, 10.f, 320.f), Colors::Neutral);
		for (int32 H = 50; H <= 300; H += 50)
		{
			SpawnBlock(World, FVector(150.f, Y, H - 1.f), FVector(10.f, H % 100 == 0 ? 80.f : 40.f, 2.f), H % 100 == 0 ? Colors::Item : FLinearColor::White);
		}
	}
	for (int32 H = 100; H <= 300; H += 100)
	{
		SpawnLabel(World, FString::Printf(TEXT("%dcm"), H), FVector(150.f, 0.f, H), FLinearColor::White, 14.f);
	}

	// 자동 점프 마네킹 2개
	for (int32 i = 0; i < 2; ++i)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AGlossaryCharacter* Npc = World->SpawnActor<AGlossaryCharacter>(FVector(150.f, i == 0 ? -330.f : 330.f, 100.f), FRotator(0.f, 180.f, 0.f), Params);
		if (Npc)
		{
			Npc->AIControllerClass = AAIController::StaticClass();
			Npc->SpawnDefaultController();
			AddJumper(Npc, i == 0 ? TEXT("짧게 누름 0.05초") : TEXT("길게 누름 0.3초"), i == 0 ? Colors::Cyan : Colors::Accent, i == 0 ? 0.05f : 0.3f);
		}
	}
	AddJumper(GetPlayerCharacter(), TEXT("나"), Colors::Player, -1.f);
}

void ADemo0107_VariableJump::TrackJump(FJumper& J, float DeltaSeconds, bool bKeyDown)
{
	ACharacter* C = J.Character.Get();
	if (!C)
	{
		return;
	}
	const UCharacterMovementComponent* Move = C->GetCharacterMovement();
	const float FootZ = C->GetActorLocation().Z - C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	if (bKeyDown && !J.bHolding && J.AutoHold < 0.f)
	{
		J.PressTime = 0.f;   // 플레이어: 새로 누르기 시작
	}
	if (J.AutoHold < 0.f)
	{
		J.bHolding = bKeyDown;
	}
	if (bKeyDown)
	{
		J.PressTime += DeltaSeconds;
	}

	if (!J.bInAir && Move->IsFalling())
	{
		J.bInAir = true;
		J.StartZ = FootZ;
		J.ApexZ = FootZ;
	}
	else if (J.bInAir)
	{
		J.ApexZ = FMath::Max(J.ApexZ, FootZ);
		if (Move->IsMovingOnGround())
		{
			J.bInAir = false;
			const float Height = J.ApexZ - J.StartZ;
			// 최고점 구슬 + 라벨 (각자 최근 3개만)
			const FVector At(C->GetActorLocation().X, C->GetActorLocation().Y, J.ApexZ);
			AStaticMeshActor* Ball = SpawnShape(GetWorld(), EShape::Sphere, At, FVector(24.f), J.Color);
			Ball->SetActorEnableCollision(false);
			SetColor(Ball->GetStaticMeshComponent(), J.Color, 0.4f, 0.f, 2.f);
			AGlossaryLabel* L = SpawnLabel(GetWorld(), FString::Printf(TEXT("+%.0fcm (누름 %.2fs)"), Height, J.PressTime), At + FVector(0.f, 0.f, 30.f), J.Color, 13.f);
			J.Markers.Add(Ball);
			J.Markers.Add(L);
			while (J.Markers.Num() > 6)
			{
				if (AActor* Old = J.Markers[0].Get()) Old->Destroy();
				J.Markers.RemoveAt(0);
			}
		}
	}
}

void ADemo0107_VariableJump::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (WasKeyPressed(EKeys::C))
	{
		bVariableJump = !bVariableJump;
		for (FJumper& J : Jumpers)
		{
			Apply(J.Character.Get());
		}
	}

	for (FJumper& J : Jumpers)
	{
		ACharacter* C = J.Character.Get();
		if (!C)
		{
			continue;
		}
		if (J.AutoHold >= 0.f)
		{
			// 자동 점프: 2초마다 Jump(), AutoHold 초 뒤 StopJumping() — 플레이어가 버튼을 누르고 떼는 것과 같다
			J.Timer += DeltaSeconds;
			if (!J.bHolding && J.Timer >= 2.f && C->GetCharacterMovement()->IsMovingOnGround())
			{
				C->Jump();
				J.bHolding = true;
				J.PressTime = 0.f;
				J.Timer = 0.f;
			}
			else if (J.bHolding && J.Timer >= J.AutoHold)
			{
				C->StopJumping();
				J.bHolding = false;
			}
			TrackJump(J, DeltaSeconds, J.bHolding);
			if (AGlossaryLabel* L = J.Label.Get())
			{
				L->SetActorLocation(C->GetActorLocation() + FVector(0.f, 0.f, 140.f));
			}
		}
		else
		{
			TrackJump(J, DeltaSeconds, IsKeyDown(EKeys::SpaceBar) || IsKeyDown(EKeys::Gamepad_FaceButton_Bottom));
		}
	}

	if (ACharacter* Player = GetPlayerCharacter())
	{
		SetStatus(FString::Printf(TEXT("가변 점프 %s — Jump Max Hold Time %.2f초 · JumpZVelocity %.0f\nJumpKeyHoldTime %.2f · 상승 유지 남은 시간 %.2f · 속도 Z %.0f"),
			bVariableJump ? TEXT("ON") : TEXT("OFF"), Player->JumpMaxHoldTime, JumpZVelocity,
			Player->JumpKeyHoldTime, Player->JumpForceTimeRemaining, Player->GetVelocity().Z));
	}
}
