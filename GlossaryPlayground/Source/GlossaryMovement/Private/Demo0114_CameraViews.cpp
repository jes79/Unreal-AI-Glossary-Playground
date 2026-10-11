#include "Demo0114_CameraViews.h"

#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryGameMode.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0114_CameraViews::ADemo0114_CameraViews()
{
	DemoId = TEXT("0114");
	Controls = TEXT("1 : TPS · 2 : FPS · 3 : 탑다운(마우스 좌클릭 이동) · 4 : 횡스크롤(A/D·Space)\n")
		TEXT("WASD 이동 · Space 점프 · TPS/FPS 는 마우스 시점\n")
		TEXT("관찰: 같은 캐릭터에서 Spring Arm 길이·각도·Control Rotation 사용 여부만 바꿔 네 시점이 된다");
	Showcase = TEXT("1~4 키로 TPS / FPS / 탑다운(클릭 이동, SimpleMoveToLocation) / 횡스크롤(평면 제한) 카메라 전환");
	PlayerPawnClass = AD0114_ViewCharacter::StaticClass();
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-600.f, 0.f, 100.f));
	bNeedsNavMesh = true;   // 탑다운 클릭 이동용
	NavBoundsExtent = FVector(3000.f, 3000.f, 800.f);
}

void ADemo0114_CameraViews::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 횡스크롤 코스: Y=0 줄 위에 높이가 다른 발판 (카메라가 있는 +Y 쪽은 비워 둔다)
	const float Heights[] = { 60.f, 130.f, 200.f, 130.f, 260.f, 330.f, 200.f };
	for (int32 i = 0; i < UE_ARRAY_COUNT(Heights); ++i)
	{
		SpawnBlock(World, FVector(200.f + i * 330.f, 0.f, 0.f), FVector(220.f, 300.f, Heights[i]), i % 2 ? Colors::Platform : Colors::Neutral);
	}
	SpawnLabel(World, TEXT("횡스크롤 코스 (4)"), FVector(1200.f, 0.f, 520.f), Colors::Item, 18.f);

	// 탑다운용 장애물 (-Y 쪽)
	for (int32 i = 0; i < 10; ++i)
	{
		const FVector P(-400.f + (i % 5) * 500.f, -700.f - (i / 5) * 600.f, 0.f);
		SpawnBlock(World, P, FVector(200.f, 200.f, 150.f + (i % 3) * 80.f), i % 2 ? Colors::Accent : Colors::Cyan);
	}
	SpawnLabel(World, TEXT("탑다운 장애물 — 클릭하면 길찾기로 돌아간다 (3)"), FVector(600.f, -1000.f, 400.f), Colors::Item, 18.f);

	AStaticMeshActor* Marker = SpawnShape(World, EShape::Cylinder, FVector::ZeroVector, FVector(80.f, 80.f, 6.f), Colors::Item);
	Marker->SetActorEnableCollision(false);
	SetColor(Marker->GetStaticMeshComponent(), Colors::Item, 0.4f, 0.f, 3.f);
	Marker->SetActorHiddenInGame(true);
	ClickMarker = Marker;

	SwitchView(ED0114View::TPS);
}

void ADemo0114_CameraViews::SwitchView(ED0114View View)
{
	AD0114_ViewCharacter* Character = Cast<AD0114_ViewCharacter>(GetPlayerPawn());
	APlayerController* PC = GetPC();
	if (!Character || !PC)
	{
		return;
	}
	PC->StopMovement();
	Character->SetView(View);

	// 탑다운만 마우스 커서를 보이고 클릭을 받는다
	if (AGlossaryPlayerController* GPC = Cast<AGlossaryPlayerController>(PC))
	{
		GPC->SetCursorMode(View == ED0114View::TopDown);
	}
	if (View == ED0114View::SideScroller)
	{
		// 횡스크롤 코스 줄(Y=0)로 옮기고 오른쪽(+X)을 보게
		FVector Loc = Character->GetActorLocation();
		Loc.Y = 0.f;
		Loc.X = FMath::Min(Loc.X, 0.f);
		Character->SetActorLocationAndRotation(Loc + FVector(0.f, 0.f, 20.f), FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
	}
	if (View == ED0114View::FPS || View == ED0114View::TPS)
	{
		PC->SetControlRotation(FRotator(-10.f, Character->GetActorRotation().Yaw, 0.f));
	}
	if (AActor* M = ClickMarker.Get())
	{
		M->SetActorHiddenInGame(true);
	}
}

void ADemo0114_CameraViews::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One)) SwitchView(ED0114View::TPS);
	if (WasKeyPressed(EKeys::Two)) SwitchView(ED0114View::FPS);
	if (WasKeyPressed(EKeys::Three)) SwitchView(ED0114View::TopDown);
	if (WasKeyPressed(EKeys::Four)) SwitchView(ED0114View::SideScroller);

	AD0114_ViewCharacter* Character = Cast<AD0114_ViewCharacter>(GetPlayerPawn());
	if (!Character)
	{
		return;
	}

	// 탑다운 클릭 이동: 커서 아래 바닥을 찾아 NavMesh 길찾기로 이동
	if (Character->GetView() == ED0114View::TopDown && WasKeyPressed(EKeys::LeftMouseButton))
	{
		FHitResult Hit;
		if (GetMouseWorldHit(Hit))
		{
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(GetPC(), Hit.Location);
			if (AActor* M = ClickMarker.Get())
			{
				M->SetActorHiddenInGame(false);
				M->SetActorLocation(Hit.Location + FVector(0.f, 0.f, 4.f));
			}
		}
	}

	static const TCHAR* Names[] = { TEXT("1 TPS (3인칭)"), TEXT("2 FPS (1인칭)"), TEXT("3 탑다운 (클릭 이동)"), TEXT("4 횡스크롤") };
	static const TCHAR* Notes[] = {
		TEXT("Spring Arm 400 · Use Pawn Control Rotation · 이동 방향으로 몸 회전"),
		TEXT("Arm 0 · 눈 높이 · Use Controller Rotation Yaw · 내 몸 Owner No See"),
		TEXT("Absolute Rotation (-60°) · Arm 1400 · 커서 클릭 → SimpleMoveToLocation"),
		TEXT("Absolute Rotation (옆) · Arm 1000 · Plane Constraint (Y 고정)"),
	};
	const int32 Index = static_cast<int32>(Character->GetView());
	SetStatus(FString::Printf(TEXT("시점: %s\n%s"), Names[Index], Notes[Index]));
}
