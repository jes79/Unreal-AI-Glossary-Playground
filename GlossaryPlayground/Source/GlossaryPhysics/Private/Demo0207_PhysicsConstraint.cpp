#include "Demo0207_PhysicsConstraint.h"

#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryKit.h"
#include "PhysicsDemoUtil.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

using namespace GlossaryKit;

namespace D0207Local
{
	const FVector HingeBase(0.f, -100.f, 0.f);    // 문 경첩 아래 끝
	const float DoorWidth = 140.f;
	const float DoorHeight = 220.f;
	const FVector ChainAnchor(100.f, 650.f, 430.f);
	const float LinkSpacing = 50.f;
}
using namespace D0207Local;

ADemo0207_PhysicsConstraint::ADemo0207_PhysicsConstraint()
{
	DemoId = TEXT("0207");
	Controls = TEXT("E : 문 밀기 (3초마다 자동으로 번갈아 밀림) · 1 : 문 Swing 제한 ON/OFF\n")
		TEXT("관찰: 문은 경첩 축으로만 돌고 ±90°에서 멈춘다(제한 OFF 면 끝까지 돈다).\n")
		TEXT("오른쪽 체인은 고리마다 Constraint(회전 Free)로 이어져 출렁인다");
	Showcase = TEXT("Physics Constraint 문 경첩(Swing ±90° 제한, 제한 끄기 비교)과 고리 8개짜리 흔들리는 체인");
	PlayerPawnClass = nullptr;
}

UPhysicsConstraintComponent* ADemo0207_PhysicsConstraint::MakeConstraint(const FVector& Location, const FRotator& Rotation, UPrimitiveComponent* A, UPrimitiveComponent* B)
{
	UPhysicsConstraintComponent* C = NewObject<UPhysicsConstraintComponent>(this);
	C->SetupAttachment(GetRootComponent());
	C->RegisterComponent();
	// 조인트 위치·축은 컴포넌트의 월드 트랜스폼으로 정해진다 (X=Twist 축, Z 축 둘레 = Swing1)
	C->SetWorldLocationAndRotation(Location, Rotation);
	// 이동은 전부 잠금 → 두 물체가 이 점에서 붙어 있다
	C->SetLinearXLimit(LCM_Locked, 0.f);
	C->SetLinearYLimit(LCM_Locked, 0.f);
	C->SetLinearZLimit(LCM_Locked, 0.f);
	C->SetConstrainedComponents(A, NAME_None, B, NAME_None);
	return C;
}

void ADemo0207_PhysicsConstraint::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	Camera = PhysicsDemoUtil::SpawnFixedCamera(World, FVector(-950.f, 300.f, 330.f), FRotator(-8.f, 0.f, 0.f), 75.f);

	// ---------- 문: 고정된 경첩 기둥 + 물리 문짝 ----------
	AStaticMeshActor* Post = SpawnShape(World, EShape::Cylinder, HingeBase + FVector(0.f, -8.f, DoorHeight * 0.5f + 5.f), FVector(16.f, 16.f, DoorHeight + 10.f), Colors::Dark);
	SpawnShape(World, EShape::Cube, HingeBase + FVector(0.f, DoorWidth + 18.f, DoorHeight * 0.5f + 5.f), FVector(16.f, 16.f, DoorHeight + 10.f), Colors::Dark);
	SpawnShape(World, EShape::Cube, HingeBase + FVector(0.f, DoorWidth * 0.5f + 5.f, DoorHeight + 18.f), FVector(16.f, DoorWidth + 40.f, 16.f), Colors::Dark);

	Door = SpawnShape(World, EShape::Cube, HingeBase + FVector(0.f, DoorWidth * 0.5f + 2.f, DoorHeight * 0.5f + 6.f), FVector(10.f, DoorWidth, DoorHeight),
		FLinearColor(0.55f, 0.32f, 0.15f), FRotator::ZeroRotator, true);
	Door->GetStaticMeshComponent()->SetAngularDamping(0.6f);
	DoorBaseYaw = Door->GetActorRotation().Yaw;

	// 경첩: 기둥(고정)–문짝. 회전축 = 세로(Z) → Swing1 만 허용
	DoorHinge = MakeConstraint(HingeBase + FVector(0.f, 0.f, DoorHeight * 0.5f + 6.f), FRotator::ZeroRotator, Post->GetStaticMeshComponent(), Door->GetStaticMeshComponent());
	ApplyDoorLimit();
	SpawnLabel(World, TEXT("문 경첩 (Swing1 ±90°)"), HingeBase + FVector(0.f, DoorWidth * 0.5f, DoorHeight + 90.f), Colors::Item, 18.f);

	// ---------- 체인: 고정 앵커 + 고리 N 개 ----------
	AStaticMeshActor* Anchor = SpawnShape(World, EShape::Cube, ChainAnchor + FVector(0.f, 0.f, 30.f), FVector(40.f), Colors::Dark);
	UPrimitiveComponent* Prev = Anchor->GetStaticMeshComponent();
	FVector PrevLoc = ChainAnchor + FVector(0.f, 0.f, 30.f);
	for (int32 i = 0; i < ChainLinks; ++i)
	{
		const bool bLast = i == ChainLinks - 1;
		// 처음엔 옆으로 뻗어 있게 놓아 떨어지며 흔들리게 한다
		// 고리끼리 겹치지 않게 간격을 두고, 마지막 큰 추는 조금 더 떨어뜨린다 (겹치면 스폰 순간 튕겨 나감)
		const FVector Loc = ChainAnchor + FVector(0.f, (i + 1) * LinkSpacing + (bLast ? 25.f : 0.f), 30.f);
		AStaticMeshActor* Link = SpawnShape(World, EShape::Sphere, Loc, FVector(bLast ? 60.f : 28.f),
			bLast ? Colors::Enemy : Colors::Neutral, FRotator::ZeroRotator, true);
		Link->GetStaticMeshComponent()->SetLinearDamping(0.05f);
		Links.Add(Link);

		UPhysicsConstraintComponent* C = MakeConstraint((PrevLoc + Loc) * 0.5f, FRotator::ZeroRotator, Prev, Link->GetStaticMeshComponent());
		// 회전은 자유 → 볼 조인트 (체인 고리)
		C->SetAngularSwing1Limit(ACM_Free, 0.f);
		C->SetAngularSwing2Limit(ACM_Free, 0.f);
		C->SetAngularTwistLimit(ACM_Free, 0.f);
		Prev = Link->GetStaticMeshComponent();
		PrevLoc = Loc;
	}
	SpawnLabel(World, TEXT("체인 (고리마다 Constraint)"), ChainAnchor + FVector(0.f, 0.f, 130.f), Colors::Item, 18.f);
}

void ADemo0207_PhysicsConstraint::ApplyDoorLimit()
{
	// 핵심: Swing1(세로축 회전)만 Limited, 나머지 회전은 Locked
	DoorHinge->SetAngularSwing1Limit(bLimitOn ? ACM_Limited : ACM_Free, DoorSwingLimit);
	DoorHinge->SetAngularSwing2Limit(ACM_Locked, 0.f);
	DoorHinge->SetAngularTwistLimit(ACM_Locked, 0.f);
	MaxAngleSeen = 0.f;
}

void ADemo0207_PhysicsConstraint::PushDoor()
{
	UStaticMeshComponent* Mesh = Door->GetStaticMeshComponent();
	// 문 바깥 끝을 문 면(법선) 방향으로 민다
	const FVector Edge = Mesh->GetComponentLocation() + Door->GetActorRightVector() * DoorWidth * 0.45f;
	const FVector Dir = Door->GetActorForwardVector() * PushSign;
	Mesh->AddImpulseAtLocation(Dir * DoorPush * Mesh->GetMass(), Edge);
	PushSign = -PushSign;
}

void ADemo0207_PhysicsConstraint::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PhysicsDemoUtil::KeepViewTarget(this, Camera);

	if (WasKeyPressed(EKeys::One))
	{
		bLimitOn = !bLimitOn;
		ApplyDoorLimit();
	}
	PushTimer += DeltaSeconds;
	if (WasKeyPressed(EKeys::E) || PushTimer >= 3.f)
	{
		PushTimer = 0.f;
		PushDoor();
	}

	const float Angle = FRotator::NormalizeAxis(Door->GetActorRotation().Yaw - DoorBaseYaw);
	MaxAngleSeen = FMath::Max(MaxAngleSeen, FMath::Abs(Angle));

	// 제한 범위 시각화: 경첩에서 ±90° 방향 선
	const FVector Hinge = HingeBase + FVector(0.f, 0.f, 8.f);
	if (bLimitOn)
	{
		DrawDebugLine(GetWorld(), Hinge, Hinge + FRotator(0.f, DoorSwingLimit, 0.f).RotateVector(FVector(0.f, DoorWidth, 0.f)), FColor::Yellow, false, -1.f, 0, 2.f);
		DrawDebugLine(GetWorld(), Hinge, Hinge + FRotator(0.f, -DoorSwingLimit, 0.f).RotateVector(FVector(0.f, DoorWidth, 0.f)), FColor::Yellow, false, -1.f, 0, 2.f);
	}
	for (int32 i = 1; i < Links.Num(); ++i)
	{
		DrawDebugLine(GetWorld(), Links[i - 1]->GetActorLocation(), Links[i]->GetActorLocation(), FColor(200, 200, 200), false, -1.f, 0, 3.f);
	}
	if (Links.Num() > 0)
	{
		DrawDebugLine(GetWorld(), ChainAnchor + FVector(0.f, 0.f, 30.f), Links[0]->GetActorLocation(), FColor(200, 200, 200), false, -1.f, 0, 3.f);
	}

	SetStatus(FString::Printf(TEXT("문 Swing 제한: %s · 현재 각도 %.0f° · 지금까지 최대 %.0f°\n체인 끝 속도 %.0f cm/s"),
		bLimitOn ? *FString::Printf(TEXT("Limited ±%.0f°"), DoorSwingLimit) : TEXT("Free (제한 없음)"),
		Angle, MaxAngleSeen, Links.Num() > 0 ? Links.Last()->GetVelocity().Size() : 0.f));
}
