#include "Demo0110_SpringArm.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GlossaryCharacter.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0110_SpringArm::ADemo0110_SpringArm()
{
	DemoId = TEXT("0110");
	Controls = TEXT("WASD 이동 · 마우스로 카메라 회전(Control Rotation) · C : Do Collision Test ON/OFF · [ / ] 팔 길이 -100/+100\n")
		TEXT("관찰: 등 뒤에 벽이 있어도 ON 이면 카메라가 앞으로 당겨지고, OFF 면 벽 너머로 가서 화면이 벽에 가린다");
	Showcase = TEXT("Spring Arm 길이 400 + 마우스 Control Rotation, 좁은 골목에서 Do Collision Test 로 카메라가 당겨지는 것과 끈 경우 비교");
	// 등 뒤 1.5m 에 벽이 있는 골목 안에서 시작
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-600.f, 0.f, 100.f));
}

void ADemo0110_SpringArm::Apply()
{
	if (AGlossaryCharacter* Character = Cast<AGlossaryCharacter>(GetPlayerPawn()))
	{
		USpringArmComponent* Arm = Character->GetCameraBoom();
		Arm->TargetArmLength = TargetArmLength;
		Arm->bUsePawnControlRotation = true;    // 마우스가 돌리는 Control Rotation 을 따라 팔이 회전
		Arm->bDoCollisionTest = bDoCollisionTest; // 팔과 카메라 사이에 벽이 있으면 그 앞까지 당긴다
		Arm->ProbeSize = ProbeSize;
		Arm->ProbeChannel = ECC_Camera;
	}
}

void ADemo0110_SpringArm::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 캐릭터 등 뒤 벽 + 좌우 골목 벽
	SpawnBlock(World, FVector(-800.f, 0.f, 0.f), FVector(60.f, 900.f, 500.f), Colors::Wall);
	SpawnBlock(World, FVector(-200.f, -260.f, 0.f), FVector(1200.f, 40.f, 400.f), Colors::Platform);
	SpawnBlock(World, FVector(-200.f, 260.f, 0.f), FVector(1200.f, 40.f, 400.f), Colors::Platform);
	SpawnLabel(World, TEXT("좁은 골목 — 마우스로 카메라를 돌려 벽에 대 보세요"), FVector(0.f, 0.f, 330.f), Colors::Item, 18.f);

	// 골목 밖 기둥 숲
	for (int32 i = 0; i < 12; ++i)
	{
		const FVector P(600.f + (i % 4) * 350.f, -500.f + (i / 4) * 500.f, 0.f);
		SpawnShape(World, EShape::Cylinder, P + FVector(0.f, 0.f, 200.f), FVector(80.f, 80.f, 400.f), i % 2 ? Colors::Accent : Colors::Cyan);
	}
	SpawnLabel(World, TEXT("기둥 사이 — 기둥이 카메라를 가리면 팔이 짧아진다"), FVector(1100.f, 0.f, 480.f), FLinearColor::White, 16.f);
	Apply();
}

void ADemo0110_SpringArm::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	bool bChanged = false;
	if (WasKeyPressed(EKeys::C)) { bDoCollisionTest = !bDoCollisionTest; bChanged = true; }
	if (WasKeyPressed(EKeys::LeftBracket)) { TargetArmLength = FMath::Max(100.f, TargetArmLength - 100.f); bChanged = true; }
	if (WasKeyPressed(EKeys::RightBracket)) { TargetArmLength = FMath::Min(1000.f, TargetArmLength + 100.f); bChanged = true; }
	if (bChanged)
	{
		Apply();
	}

	AGlossaryCharacter* Character = Cast<AGlossaryCharacter>(GetPlayerPawn());
	if (!Character)
	{
		return;
	}
	// 실제 팔 길이 = 팔 시작점(캐릭터) ~ 카메라 거리
	const USpringArmComponent* Arm = Character->GetCameraBoom();
	const float Actual = FVector::Dist(Arm->GetComponentLocation(), Character->GetFollowCamera()->GetComponentLocation());
	const bool bPulled = Actual < TargetArmLength - 5.f;
	SetStatus(FString::Printf(TEXT("Do Collision Test %s · Target Arm Length %.0f · 실제 길이 %.0f%s\nControl Rotation Pitch %.0f° Yaw %.0f°"),
		bDoCollisionTest ? TEXT("ON") : TEXT("OFF"), TargetArmLength, Actual,
		bPulled ? *FString::Printf(TEXT(" ← 벽에 막혀 %.0fcm 당겨짐"), TargetArmLength - Actual) : TEXT(""),
		Character->GetControlRotation().Pitch, Character->GetControlRotation().Yaw));
}
