#include "Demo0210_CCD.h"

#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryKit.h"
#include "PhysicsDemoUtil.h"

using namespace GlossaryKit;

namespace D0210Local
{
	const float WallX = 300.f;
	const float StartX = -700.f;
	const float LaneZ[2] = { 330.f, 120.f };   // 0 = CCD OFF(위), 1 = CCD ON(아래)
}
using namespace D0210Local;

ADemo0210_CCD::ADemo0210_CCD()
{
	DemoId = TEXT("0210");
	Controls = TEXT("자동 발사(1초마다) · E : 지금 발사 · [ / ] : 총알 속도 -5000 / +5000\n")
		TEXT("관찰: 위 레인(Use CCD OFF) 총알은 두께 4cm 벽을 뚫고 지나가고, 아래 레인(Use CCD ON)은 벽에 막힌다");
	Showcase = TEXT("초고속 물리 총알이 얇은 벽을 터널링(CCD OFF) vs bUseCCD 로 막힘(CCD ON), 통과/막힘 횟수 비교");
	PlayerPawnClass = nullptr;
}

void ADemo0210_CCD::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	// 옆에서 보는 카메라 (총알은 왼쪽 → 오른쪽)
	Camera = PhysicsDemoUtil::SpawnFixedCamera(World, FVector(-150.f, 1250.f, 330.f), FRotator(-6.f, -90.f, 0.f), 75.f);

	// 얇은 벽 (WorldStatic, 두께 WallThickness)
	AStaticMeshActor* Wall = SpawnShape(World, EShape::Cube, FVector(WallX, 0.f, 220.f), FVector(WallThickness, 300.f, 440.f), Colors::Wall);
	Wall->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
	// 레인 구분선
	SpawnShape(World, EShape::Cube, FVector(-200.f, 60.f, 225.f), FVector(1100.f, 10.f, 6.f), Colors::Dark);
	SpawnLabel(World, TEXT("Use CCD OFF → 뚫고 지나감"), FVector(-250.f, 0.f, LaneZ[0] + 70.f), Colors::Enemy, 20.f);
	SpawnLabel(World, TEXT("Use CCD ON → 막힘"), FVector(-250.f, 0.f, LaneZ[1] + 70.f), Colors::Ally, 20.f);
	SpawnLabel(World, FString::Printf(TEXT("두께 %.0fcm 벽"), WallThickness), FVector(WallX, 0.f, 480.f), FLinearColor::White, 18.f);
}

void ADemo0210_CCD::Fire()
{
	UWorld* World = GetWorld();
	for (int32 Lane = 0; Lane < 2; ++Lane)
	{
		AStaticMeshActor* Bullet = SpawnShape(World, EShape::Sphere, FVector(StartX, 0.f, LaneZ[Lane]), FVector(12.f),
			Lane == 0 ? Colors::Enemy : Colors::Ally, FRotator::ZeroRotator, true);
		UStaticMeshComponent* Mesh = Bullet->GetStaticMeshComponent();
		SetColor(Mesh, Lane == 0 ? Colors::Enemy : Colors::Ally, 0.4f, 0.f, 3.f);
		Mesh->SetEnableGravity(false);
		// 핵심: 연속 충돌 감지. 이전 위치→현재 위치 사이를 쓸어서 검사하므로 얇은 벽을 건너뛰지 않는다
		Mesh->SetUseCCD(Lane == 1);
		Mesh->SetPhysicsLinearVelocity(FVector(BulletSpeed, 0.f, 0.f));
		Bullet->SetLifeSpan(2.f);

		FShot Shot;
		Shot.Actor = Bullet;
		Shot.Lane = Lane;
		Shot.LastPos = Bullet->GetActorLocation();
		Shots.Add(Shot);
	}
}

void ADemo0210_CCD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PhysicsDemoUtil::KeepViewTarget(this, Camera);

	if (WasKeyPressed(EKeys::LeftBracket))  { BulletSpeed = FMath::Max(2000.f, BulletSpeed - 5000.f); }
	if (WasKeyPressed(EKeys::RightBracket)) { BulletSpeed += 5000.f; }
	Timer += DeltaSeconds;
	if (WasKeyPressed(EKeys::E) || Timer >= FireInterval)
	{
		Timer = 0.f;
		Fire();
	}

	for (FShot& Shot : Shots)
	{
		AStaticMeshActor* Actor = Shot.Actor.Get();
		if (!Actor)
		{
			continue;
		}
		Shot.Age += DeltaSeconds;
		const FVector Pos = Actor->GetActorLocation();
		// 한 프레임 동안의 이동을 선으로: CCD OFF 는 벽 앞뒤로 큰 점프가 보인다
		DrawDebugLine(GetWorld(), Shot.LastPos, Pos, Shot.Lane == 0 ? FColor::Red : FColor::Green, false, 0.6f, 0, 2.f);
		DrawDebugPoint(GetWorld(), Pos, 8.f, Shot.Lane == 0 ? FColor::Red : FColor::Green, false, 0.6f);
		if (!Shot.bCounted)
		{
			if (Pos.X > WallX + 20.f)
			{
				++Passed[Shot.Lane];
				Shot.bCounted = true;
			}
			// 벽 앞에서 속도를 잃었으면(멈추거나 튕김) 막힌 것
			else if (Pos.X < WallX && Actor->GetVelocity().X < BulletSpeed * 0.3f && Shot.Age > 0.05f)
			{
				++Blocked[Shot.Lane];
				Shot.bCounted = true;
			}
		}
		Shot.LastPos = Pos;
	}
	Shots.RemoveAll([](const FShot& S) { return !S.Actor.IsValid(); });

	SetStatus(FString::Printf(TEXT("총알 %.0fcm/s → 한 프레임에 약 %.0fcm 이동 (벽 두께 %.0fcm)\nCCD OFF: 통과 %d · 막힘 %d    CCD ON: 통과 %d · 막힘 %d"),
		BulletSpeed, BulletSpeed * DeltaSeconds, WallThickness, Passed[0], Blocked[0], Passed[1], Blocked[1]));
}
