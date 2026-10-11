#include "Demo0213_PhysicsHandle.h"

#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/SpringArmComponent.h"
#include "GlossaryCharacter.h"
#include "GlossaryKit.h"
#include "PhysicsDemoUtil.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"

using namespace GlossaryKit;

ADemo0213_PhysicsHandle::ADemo0213_PhysicsHandle()
{
	DemoId = TEXT("0213");
	Controls = TEXT("WASD 이동 · 마우스 시점 · 좌클릭 누르고 있기 : 잡기 → 떼면 던지기\n")
		TEXT("E : 잡기/던지기 전환 (한 번 누르면 잡고, 다시 누르면 던짐)\n")
		TEXT("관찰: 잡힌 물체는 물리를 유지한 채 스프링처럼 따라오고(다른 물체를 밀어냄), 놓으면 시선 방향으로 날아간다");
	Showcase = TEXT("PhysicsHandleComponent 로 화면 가운데 물리 물체를 잡아 들고 이동, 놓으면 시선 방향으로 던지기(그래비티 건)");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-650.f, 0.f, 100.f));

	PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));
	PhysicsHandle->LinearStiffness = 750.f;    // 들고 있는 물체가 목표점에 붙는 강도
	PhysicsHandle->InterpolationSpeed = 50.f;
}

void ADemo0213_PhysicsHandle::BeginPlay()
{
	Super::BeginPlay();
	SpawnProps();
}

void ADemo0213_PhysicsHandle::SpawnProps()
{
	UWorld* World = GetWorld();
	// 받침대 위 상자: 시작하자마자 화면 가운데에 오도록
	SpawnShape(World, EShape::Cube, FVector(-250.f, 90.f, 60.f), FVector(70.f, 70.f, 120.f), Colors::Dark);
	SpawnShape(World, EShape::Cube, FVector(-250.f, 90.f, 152.f), FVector(60.f), Colors::Item, FRotator::ZeroRotator, true);

	// 주변 물리 물체들
	const FLinearColor Palette[] = { Colors::Accent, Colors::Cyan, Colors::Ally, Colors::Purple, Colors::Enemy };
	int32 Index = 0;
	for (int32 X = 0; X < 3; ++X)
	{
		for (int32 Y = 0; Y < 3; ++Y)
		{
			const EShape Shape = (X + Y) % 3 == 0 ? EShape::Sphere : ((X + Y) % 3 == 1 ? EShape::Cube : EShape::Cylinder);
			SpawnShape(World, Shape, FVector(150.f + X * 150.f, -300.f + Y * 300.f, 40.f), FVector(70.f), Palette[Index++ % 5], FRotator::ZeroRotator, true);
		}
	}
	// 던져서 맞힐 벽
	SpawnShape(World, EShape::Cube, FVector(900.f, 0.f, 200.f), FVector(40.f, 1200.f, 400.f), Colors::Wall);
}

void ADemo0213_PhysicsHandle::Grab()
{
	FHitResult Hit;
	if (!PhysicsDemoUtil::TraceScreenCenter(this, Hit, GrabRange))
	{
		LastAction = TEXT("잡을 물체 없음");
		return;
	}
	UPrimitiveComponent* Comp = Hit.GetComponent();
	if (!Comp || !Comp->IsSimulatingPhysics())
	{
		LastAction = FString::Printf(TEXT("%s 는 Simulate Physics 가 아니라 못 잡음"), *GetNameSafe(Hit.GetActor()));
		return;
	}
	// 핵심: 물리 바디를 핸들에 연결. 이후 SetTargetLocation 으로 끌고 다닌다
	PhysicsHandle->GrabComponentAtLocationWithRotation(Comp, NAME_None, Comp->GetComponentLocation(), Comp->GetComponentRotation());
	Comp->WakeRigidBody();
	Held = Comp;
	LastAction = FString::Printf(TEXT("잡음: %s (질량 %.0fkg)"), *GetNameSafe(Hit.GetActor()), Comp->GetMass());
}

void ADemo0213_PhysicsHandle::Throw()
{
	UPrimitiveComponent* Comp = Held.Get();
	PhysicsHandle->ReleaseComponent();
	Held = nullptr;
	FVector Start, Dir;
	if (Comp && PhysicsDemoUtil::GetViewRay(this, Start, Dir))
	{
		// 놓는 순간 시선 방향으로 속도 변화(bVelChange=true → 질량과 무관)
		Comp->AddImpulse(Dir * ThrowSpeed, NAME_None, true);
		++Throws;
		LastAction = FString::Printf(TEXT("던짐: %.0f cm/s"), ThrowSpeed);
	}
}

void ADemo0213_PhysicsHandle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 어깨 너머 시점 (화면 가운데 조준점이 캐릭터에 가리지 않게)
	if (AGlossaryCharacter* Character = Cast<AGlossaryCharacter>(GetPlayerPawn()))
	{
		Character->GetCameraBoom()->SocketOffset = FVector(0.f, 90.f, 60.f);
	}

	const bool bHolding = Held.IsValid();
	if (WasKeyPressed(EKeys::E))
	{
		bHolding ? Throw() : Grab();
	}
	else if (WasKeyPressed(EKeys::LeftMouseButton) && !bHolding)
	{
		Grab();
	}
	else if (WasKeyReleased(EKeys::LeftMouseButton) && bHolding)
	{
		Throw();
	}

	FVector Start, Dir;
	if (PhysicsDemoUtil::GetViewRay(this, Start, Dir))
	{
		const FVector Target = Start + Dir * (HoldDistance + 400.f);   // 카메라는 캐릭터 400 뒤 → 캐릭터 앞 약 300
		if (Held.IsValid())
		{
			PhysicsHandle->SetTargetLocation(Target);
			DrawDebugLine(GetWorld(), GetPlayerPawn() ? GetPlayerPawn()->GetActorLocation() + FVector(0.f, 0.f, 40.f) : Start,
				Held->GetComponentLocation(), FColor::Cyan, false, -1.f, 0, 2.f);
			DrawDebugSphere(GetWorld(), Target, 10.f, 8, FColor::Cyan);
		}
		else
		{
			// 조준점
			FHitResult Hit;
			if (PhysicsDemoUtil::TraceScreenCenter(this, Hit, GrabRange))
			{
				const bool bGrabbable = Hit.GetComponent() && Hit.GetComponent()->IsSimulatingPhysics();
				DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 8.f, 8, bGrabbable ? FColor::Green : FColor::White);
			}
		}
	}

	SetStatus(FString::Printf(TEXT("%s · 던진 횟수 %d\n%s"),
		Held.IsValid() ? TEXT("들고 있음 (PhysicsHandle 연결)") : TEXT("빈손"), Throws,
		LastAction.IsEmpty() ? TEXT("노란 상자를 조준하고 좌클릭(또는 E)") : *LastAction));
}
