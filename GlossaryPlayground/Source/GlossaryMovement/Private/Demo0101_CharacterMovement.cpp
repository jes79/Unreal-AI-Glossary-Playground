#include "Demo0101_CharacterMovement.h"

#include "Engine/StaticMeshActor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0101_CharacterMovement::ADemo0101_CharacterMovement()
{
	DemoId = TEXT("0101");
	Controls = TEXT("WASD 이동 · 마우스 시점 · Space 점프\n")
		TEXT("[ / ] : Walkable Floor Angle -5° / +5°   ,  . : Max Step Height -15 / +15\n")
		TEXT("관찰: 초록 램프·턱만 오른다. 값을 바꾸면 색이 바로 바뀌고 실제로 오를 수 있는지도 바뀐다");
	Showcase = TEXT("30°·45°·50° 램프와 30·45·60cm 턱, Walkable Floor Angle·Max Step Height 실시간 조절(색으로 오를 수 있는지 표시)");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-700.f, 0.f, 100.f));
}

void ADemo0101_CharacterMovement::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 왼쪽 줄: 경사 램프 3개 (각도만 다르고 길이·폭은 같다)
	SpawnRamp(30.f, FVector(0.f, -900.f, 0.f));
	SpawnRamp(45.f, FVector(0.f, -300.f, 0.f));
	SpawnRamp(50.f, FVector(0.f, 300.f, 0.f));

	// 오른쪽 줄: 턱 높이 3종
	SpawnStep(30.f, FVector(0.f, 900.f, 0.f));
	SpawnStep(45.f, FVector(0.f, 1300.f, 0.f));
	SpawnStep(60.f, FVector(0.f, 1700.f, 0.f));

	SpawnLabel(World, TEXT("경사 램프 → Walkable Floor Angle"), FVector(-100.f, -300.f, 450.f), Colors::Item, 20.f);
	SpawnLabel(World, TEXT("턱 높이 → Max Step Height"), FVector(-100.f, 1300.f, 300.f), Colors::Item, 20.f);

	ApplyMovementSettings();
}

void ADemo0101_CharacterMovement::SpawnRamp(float Angle, const FVector& Base)
{
	// 길이 L 의 판을 Angle 만큼 들어 올린다: 아래 끝이 Base 에 닿도록 중심을 (L/2)cos, (L/2)sin 만큼 옮긴다
	constexpr float Length = 700.f, Width = 400.f, Thickness = 20.f;
	const float Rad = FMath::DegreesToRadians(Angle);
	const FVector Center = Base + FVector(FMath::Cos(Rad) * Length * 0.5f, 0.f, FMath::Sin(Rad) * Length * 0.5f);
	AStaticMeshActor* Ramp = SpawnShape(GetWorld(), EShape::Cube, Center, FVector(Length, Width, Thickness), Colors::Neutral, FRotator(Angle, 0.f, 0.f));
	Ramps.Add({ Ramp, Angle });

	// 꼭대기 평판
	const FVector Top = Base + FVector(FMath::Cos(Rad) * Length, 0.f, FMath::Sin(Rad) * Length);
	SpawnBlock(GetWorld(), FVector(Top.X + 200.f, Top.Y, 0.f), FVector(400.f, Width, Top.Z), Colors::Platform);
	SpawnLabel(GetWorld(), FString::Printf(TEXT("%.0f°"), Angle), Base + FVector(-60.f, 0.f, 120.f), FLinearColor::White, 26.f);
}

void ADemo0101_CharacterMovement::SpawnStep(float Height, const FVector& Base)
{
	AStaticMeshActor* Step = SpawnBlock(GetWorld(), Base + FVector(150.f, 0.f, 0.f), FVector(300.f, 300.f, Height), Colors::Neutral);
	Steps.Add({ Step, Height });
	SpawnLabel(GetWorld(), FString::Printf(TEXT("턱 %.0fcm"), Height), Base + FVector(-60.f, 0.f, Height + 100.f), FLinearColor::White, 22.f);
}

void ADemo0101_CharacterMovement::ApplyMovementSettings()
{
	if (ACharacter* Character = GetPlayerCharacter())
	{
		UCharacterMovementComponent* Move = Character->GetCharacterMovement();
		// 핵심 두 줄: 경사 한계와 계단 높이 한계
		Move->SetWalkableFloorAngle(WalkableFloorAngle);
		Move->MaxStepHeight = MaxStepHeight;
	}
	RefreshColors();
}

void ADemo0101_CharacterMovement::RefreshColors()
{
	for (const FRamp& Ramp : Ramps)
	{
		if (AStaticMeshActor* Actor = Ramp.Actor.Get())
		{
			SetColor(Actor->GetStaticMeshComponent(), Ramp.Angle <= WalkableFloorAngle ? Colors::Ally : Colors::Enemy);
		}
	}
	for (const FStep& Step : Steps)
	{
		if (AStaticMeshActor* Actor = Step.Actor.Get())
		{
			SetColor(Actor->GetStaticMeshComponent(), Step.Height <= MaxStepHeight ? Colors::Ally : Colors::Enemy);
		}
	}
}

void ADemo0101_CharacterMovement::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	bool bChanged = false;
	if (WasKeyPressed(EKeys::LeftBracket))  { WalkableFloorAngle = FMath::Clamp(WalkableFloorAngle - 5.f, 0.f, 85.f); bChanged = true; }
	if (WasKeyPressed(EKeys::RightBracket)) { WalkableFloorAngle = FMath::Clamp(WalkableFloorAngle + 5.f, 0.f, 85.f); bChanged = true; }
	if (WasKeyPressed(EKeys::Comma))        { MaxStepHeight = FMath::Max(0.f, MaxStepHeight - 15.f); bChanged = true; }
	if (WasKeyPressed(EKeys::Period))       { MaxStepHeight += 15.f; bChanged = true; }
	if (bChanged)
	{
		ApplyMovementSettings();
	}

	ACharacter* Character = GetPlayerCharacter();
	if (!Character)
	{
		return;
	}
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	const FFindFloorResult& Floor = Move->CurrentFloor;
	const float FloorAngle = Floor.bBlockingHit ? FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Floor.HitResult.ImpactNormal.Z, -1.f, 1.f))) : 0.f;
	SetStatus(FString::Printf(TEXT("Walkable Floor Angle %.0f° · Max Step Height %.0fcm\nMovementMode %s · 속도 %.0f cm/s · 발밑 경사 %.0f°%s"),
		WalkableFloorAngle, MaxStepHeight,
		*Move->GetMovementName(), Move->Velocity.Size2D(), FloorAngle,
		Floor.bWalkableFloor ? TEXT("") : TEXT(" (걸을 수 없는 바닥 → 미끄러짐)")));
}
