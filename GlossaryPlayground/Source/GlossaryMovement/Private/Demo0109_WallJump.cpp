#include "Demo0109_WallJump.h"

#include "D0109_WallJumpCharacter.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"

using namespace GlossaryKit;

ADemo0109_WallJump::ADemo0109_WallJump()
{
	DemoId = TEXT("0109");
	Controls = TEXT("WASD 이동 · Space 점프 · C : 벽 슬라이드/벽 점프 ON/OFF\n")
		TEXT("두 벽 사이로 뛰어 벽에 붙은 뒤 Space — 반대쪽 벽으로 튕겨 오르기를 반복해 꼭대기(초록)까지\n")
		TEXT("관찰: 벽에 붙으면 낙하 속도가 150 cm/s 로 줄고(슬라이드), 점프는 벽 반대쪽으로 발사된다");
	Showcase = TEXT("수직 통로에서 벽에 붙으면 낙하 속도 제한(벽 슬라이드), 점프하면 벽 반대쪽으로 LaunchCharacter, 노란 화살표 = 벽 법선");
	PlayerPawnClass = AD0109_WallJumpCharacter::StaticClass();
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-600.f, 0.f, 100.f));
}

void ADemo0109_WallJump::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 수직 통로: Y 방향으로 마주 보는 두 벽 (사이 간격 450)
	constexpr float Gap = 450.f;
	SpawnBlock(World, FVector(300.f, -Gap * 0.5f - 50.f, 0.f), FVector(600.f, 100.f, 1100.f), Colors::Wall);
	SpawnBlock(World, FVector(300.f, Gap * 0.5f + 50.f, 0.f), FVector(600.f, 100.f, 800.f), Colors::Wall);
	// 목표: 낮은 쪽 벽 위의 초록 발판
	SpawnBlock(World, FVector(300.f, Gap * 0.5f + 250.f, 790.f), FVector(600.f, 400.f, 20.f), Colors::Ally);
	SpawnLabel(World, TEXT("목표 (높이 8m)"), FVector(300.f, Gap * 0.5f + 250.f, 900.f), Colors::Ally, 18.f);
	SpawnLabel(World, TEXT("벽 사이로 들어가 점프 → 벽에 붙으면 Space"), FVector(0.f, 0.f, 300.f), Colors::Item, 18.f);

	// 높이 눈금 (1m 간격, 왼쪽 벽 면)
	for (int32 H = 100; H <= 1000; H += 100)
	{
		SpawnBlock(World, FVector(0.f, -Gap * 0.5f + 2.f, H), FVector(60.f, 4.f, 4.f), FLinearColor::White);
		if (H % 200 == 0)
		{
			SpawnLabel(World, FString::Printf(TEXT("%dm"), H / 100), FVector(-20.f, -Gap * 0.5f + 30.f, H), FLinearColor::White, 13.f);
		}
	}

	// 단일 벽 연습용
	SpawnBlock(World, FVector(-200.f, 900.f, 0.f), FVector(500.f, 100.f, 600.f), Colors::Wall);
	SpawnLabel(World, TEXT("한쪽 벽: 슬라이드만 연습"), FVector(-200.f, 800.f, 680.f), FLinearColor::White, 16.f);

	HeadLabel = SpawnLabel(World, TEXT(""), FVector::ZeroVector, Colors::Cyan, 15.f);
	AStaticMeshActor* Arrow = SpawnShape(World, EShape::Cone, FVector::ZeroVector, FVector(30.f, 30.f, 60.f), Colors::Item);
	Arrow->SetActorEnableCollision(false);
	NormalArrow = Arrow;
}

void ADemo0109_WallJump::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AD0109_WallJumpCharacter* Character = Cast<AD0109_WallJumpCharacter>(GetPlayerPawn());
	if (!Character)
	{
		return;
	}
	if (WasKeyPressed(EKeys::C))
	{
		Character->bWallJumpEnabled = !Character->bWallJumpEnabled;
	}

	const FVector Loc = Character->GetActorLocation();
	BestHeight = FMath::Max(BestHeight, Loc.Z - 96.f);

	// 벽 법선 화살표: 벽 점프가 날아갈 방향
	if (AActor* Arrow = NormalArrow.Get())
	{
		Arrow->SetActorHiddenInGame(!Character->IsOnWall());
		if (Character->IsOnWall())
		{
			const FVector N = Character->GetWallNormal();
			Arrow->SetActorLocation(Loc + N * 90.f + FVector(0.f, 0.f, 40.f));
			// 원뿔의 뾰족한 쪽(+Z)이 법선 방향을 향하도록
			Arrow->SetActorRotation(FRotationMatrix::MakeFromZ(N).Rotator());
		}
	}
	if (AGlossaryLabel* L = HeadLabel.Get())
	{
		L->SetActorLocation(Loc + FVector(0.f, 0.f, 130.f));
		L->SetActorHiddenInGame(!Character->IsOnWall());
		L->SetText(Character->IsWallSliding() ? TEXT("벽 슬라이드") : TEXT("벽에 붙음"));
	}

	const UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	SetStatus(FString::Printf(TEXT("벽 기능 %s · Wall Slide Speed %.0f · 벽 점프 %d회 · 최고 높이 %.1fm\n%s · 낙하 속도 %.0f cm/s%s"),
		Character->bWallJumpEnabled ? TEXT("ON") : TEXT("OFF"), Character->WallSlideSpeed, Character->GetWallJumpCount(), BestHeight / 100.f,
		*Move->GetMovementName(), -Move->Velocity.Z,
		Character->IsWallSliding() ? TEXT(" (벽 슬라이드로 제한 중)") : (Character->IsOnWall() ? TEXT(" (벽에 붙음)") : TEXT(""))));
}
