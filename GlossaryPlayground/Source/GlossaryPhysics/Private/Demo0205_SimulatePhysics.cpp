#include "Demo0205_SimulatePhysics.h"

#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "PhysicsDemoUtil.h"

using namespace GlossaryKit;

namespace
{
	const float LeftY = -150.f;   // Simulate Physics ON 더미
	const float RightY = 550.f;   // OFF 더미
}

ADemo0205_SimulatePhysics::ADemo0205_SimulatePhysics()
{
	DemoId = TEXT("0205");
	Controls = TEXT("좌클릭 : 클릭한 지점에 AddImpulseAtLocation · E : 화면 가운데에 충격\n")
		TEXT("R : 상자 다시 쌓기 · F3 : 마우스 커서 켜기/끄기\n")
		TEXT("관찰: 왼쪽(Simulate Physics ON)은 맞은 자리에서 밀리고 돌며 무너지고, 오른쪽(OFF)은 꿈쩍 않는다");
	Showcase = TEXT("Simulate Physics 켠 상자 더미를 클릭 지점 AddImpulseAtLocation 으로 밀기, 꺼진 더미와 비교");
	PlayerPawnClass = nullptr;
	bShowMouseCursor = true;
}

void ADemo0205_SimulatePhysics::BeginPlay()
{
	Super::BeginPlay();
	// 화면 가운데가 왼쪽 더미를 향하도록 카메라를 둔다 (E 키 = 가운데 클릭)
	Camera = PhysicsDemoUtil::SpawnFixedCamera(GetWorld(), FVector(-900.f, 120.f, 380.f), FRotator(-14.f, -12.f, 0.f), 75.f);
	SpawnStacks();
	SpawnLabel(GetWorld(), TEXT("Simulate Physics ON"), FVector(0.f, LeftY, 330.f), Colors::Ally, 20.f);
	SpawnLabel(GetWorld(), TEXT("Simulate Physics OFF"), FVector(0.f, RightY, 330.f), Colors::Enemy, 20.f);
}

void ADemo0205_SimulatePhysics::SpawnStacks()
{
	for (AStaticMeshActor* Box : Boxes)
	{
		if (Box)
		{
			Box->Destroy();
		}
	}
	Boxes.Reset();

	UWorld* World = GetWorld();
	const float Size = 60.f;
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const bool bPhysics = Side == 0;
		const float BaseY = bPhysics ? LeftY : RightY;
		// 피라미드: 아래 4 · 3 · 2 · 1
		for (int32 Row = 0; Row < 4; ++Row)
		{
			const int32 Count = 4 - Row;
			for (int32 i = 0; i < Count; ++i)
			{
				const float Y = BaseY + (i - (Count - 1) * 0.5f) * (Size + 2.f);
				const FVector Loc(0.f, Y, Size * 0.5f + Row * (Size + 1.f) + 1.f);
				const FLinearColor Color = bPhysics ? FLinearColor::LerpUsingHSV(Colors::Ally, Colors::Cyan, Row / 3.f) : Colors::Neutral;
				// 핵심: bPhysics=true 면 SetSimulatePhysics(true) — 이제 위치는 물리 엔진이 정한다
				AStaticMeshActor* Box = SpawnShape(World, EShape::Cube, Loc, FVector(Size), Color, FRotator::ZeroRotator, bPhysics);
				Boxes.Add(Box);
			}
		}
	}
}

void ADemo0205_SimulatePhysics::PushAt(const FHitResult& Hit, const FVector& Direction)
{
	LastPoint = Hit.ImpactPoint;
	LastDir = Direction;
	ArrowTime = 1.5f;
	UPrimitiveComponent* Comp = Hit.GetComponent();
	if (Comp && Comp->IsSimulatingPhysics())
	{
		// 핵심: 맞은 위치에 충격량. 중심에서 벗어난 곳을 치면 토크가 생겨 회전한다
		const FVector Impulse = Direction * ImpulseStrength * Comp->GetMass();
		Comp->AddImpulseAtLocation(Impulse, Hit.ImpactPoint);
		LastResult = FString::Printf(TEXT("AddImpulseAtLocation → %s (질량 %.0fkg, 충격 %.0f)"), *GetNameSafe(Hit.GetActor()), Comp->GetMass(), Impulse.Size());
	}
	else if (Comp)
	{
		LastResult = FString::Printf(TEXT("%s : Simulate Physics 꺼짐 → 충격 무시"), Hit.GetActor() && Boxes.Contains(Hit.GetActor()) ? TEXT("OFF 상자") : TEXT("바닥/고정 물체"));
	}
}

void ADemo0205_SimulatePhysics::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PhysicsDemoUtil::KeepViewTarget(this, Camera);

	if (WasKeyPressed(EKeys::R))
	{
		SpawnStacks();
	}
	FHitResult Hit;
	FVector Start, Dir;
	if (WasKeyPressed(EKeys::LeftMouseButton) && PhysicsDemoUtil::TraceCursor(this, Hit))
	{
		PushAt(Hit, (Hit.ImpactPoint - Hit.TraceStart).GetSafeNormal());
	}
	if (WasKeyPressed(EKeys::E) && PhysicsDemoUtil::TraceScreenCenter(this, Hit) && PhysicsDemoUtil::GetViewRay(this, Start, Dir))
	{
		PushAt(Hit, Dir);
	}

	if (ArrowTime > 0.f)
	{
		ArrowTime -= DeltaSeconds;
		DrawDebugDirectionalArrow(GetWorld(), LastPoint - LastDir * 150.f, LastPoint, 30.f, FColor::Orange, false, -1.f, 0, 4.f);
		DrawDebugSphere(GetWorld(), LastPoint, 10.f, 8, FColor::Orange);
	}

	int32 Moving = 0;
	for (AStaticMeshActor* Box : Boxes)
	{
		if (Box && Box->GetStaticMeshComponent()->IsSimulatingPhysics() && Box->GetVelocity().Size() > 5.f)
		{
			++Moving;
		}
	}
	SetStatus(FString::Printf(TEXT("움직이는 물리 상자 %d개\n%s"), Moving, LastResult.IsEmpty() ? TEXT("상자를 클릭하거나 E") : *LastResult));
}
