#include "Demo0208_RadialForce.h"

#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryKit.h"
#include "PhysicsDemoUtil.h"
#include "PhysicsEngine/RadialForceComponent.h"

using namespace GlossaryKit;

namespace D0208Local
{
	const FVector GridCenter(300.f, 350.f, 0.f);
}
using namespace D0208Local;

ADemo0208_RadialForce::ADemo0208_RadialForce()
{
	DemoId = TEXT("0208");
	Controls = TEXT("좌클릭 : 클릭한 곳에서 폭발 · E : 화면 가운데에서 폭발 · R : 상자 다시 놓기\n")
		TEXT("관찰: 노란 원(반경 500) 안의 상자만 중심에서 바깥으로 날아가고, 밖의 상자는 그대로");
	Showcase = TEXT("클릭 위치 RadialForceComponent 반경 500 FireImpulse, 원 안 상자만 사방으로 날아감");
	PlayerPawnClass = nullptr;
	bShowMouseCursor = true;

	RadialForce = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForce"));
	RadialForce->SetupAttachment(Root);
	RadialForce->bAutoActivate = false;          // 계속 미는 Force 는 끄고, 순간 Impulse 만 쓴다
	RadialForce->ForceStrength = 0.f;
}

void ADemo0208_RadialForce::BeginPlay()
{
	Super::BeginPlay();
	// 화면 가운데가 상자 격자 중심을 보도록
	Camera = PhysicsDemoUtil::SpawnFixedCamera(GetWorld(), GridCenter + FVector(-1300.f, -150.f, 1000.f), FRotator(-35.f, 6.f, 0.f), 75.f);
	RadialForce->Radius = Radius;
	RadialForce->ImpulseStrength = ImpulseStrength;
	RadialForce->bImpulseVelChange = true;       // 질량 무시: 큰 상자·작은 상자 같은 속도 변화
	RadialForce->Falloff = RIF_Linear;          // 중심에서 멀수록 약하게
	SpawnBoxes();
}

void ADemo0208_RadialForce::SpawnBoxes()
{
	for (AStaticMeshActor* Box : Boxes)
	{
		if (Box)
		{
			Box->Destroy();
		}
	}
	Boxes.Reset();
	// 9×9 격자 (간격 120) — 반경 500 원보다 넓게 깔아 밖의 상자가 남는 것을 보여 준다
	for (int32 X = -4; X <= 4; ++X)
	{
		for (int32 Y = -4; Y <= 4; ++Y)
		{
			const FVector Loc = GridCenter + FVector(X * 120.f, Y * 120.f, 31.f);
			const float Dist = FVector2D(X * 120.f, Y * 120.f).Size();
			const FLinearColor Color = FLinearColor::LerpUsingHSV(Colors::Accent, Colors::Player, FMath::Clamp(Dist / 700.f, 0.f, 1.f));
			AStaticMeshActor* Box = SpawnShape(GetWorld(), EShape::Cube, Loc, FVector(60.f), Color, FRotator::ZeroRotator, true);
			// RadialForce 는 ObjectTypesToAffect(기본: WorldDynamic·PhysicsBody 등)에 든 타입만 민다 → PhysicsActor 프리셋(PhysicsBody)
			Box->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("PhysicsActor"));
			Boxes.Add(Box);
		}
	}
}

void ADemo0208_RadialForce::Explode(const FVector& Location)
{
	// 핵심: 컴포넌트를 폭발 지점으로 옮기고 FireImpulse — 반경 안 물리 바디에 바깥 방향 Impulse
	RadialForce->SetWorldLocation(Location - FVector(0.f, 0.f, 60.f));   // 지면보다 조금 아래 → 바깥+위로 튀어 오름
	RadialForce->Radius = Radius;
	RadialForce->ImpulseStrength = ImpulseStrength;
	RadialForce->FireImpulse();
	LastBlast = Location;
	BlastTime = 1.5f;
	++Explosions;
}

void ADemo0208_RadialForce::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PhysicsDemoUtil::KeepViewTarget(this, Camera);

	if (WasKeyPressed(EKeys::R))
	{
		SpawnBoxes();
	}
	FHitResult Hit;
	if (WasKeyPressed(EKeys::LeftMouseButton) && PhysicsDemoUtil::TraceCursor(this, Hit))
	{
		Explode(Hit.ImpactPoint);
	}
	if (WasKeyPressed(EKeys::E) && PhysicsDemoUtil::TraceScreenCenter(this, Hit))
	{
		Explode(Hit.ImpactPoint);
	}

	// 반경 미리보기: 폭발 직후엔 그 자리, 평소엔 커서 위치
	FVector Preview = LastBlast;
	if (BlastTime <= 0.f && PhysicsDemoUtil::TraceCursor(this, Hit))
	{
		Preview = Hit.ImpactPoint;
	}
	BlastTime -= DeltaSeconds;
	const FColor CircleColor = BlastTime > 0.f ? FColor::Orange : FColor::Yellow;
	DrawDebugCircle(GetWorld(), Preview + FVector(0.f, 0.f, 3.f), Radius, 64, CircleColor, false, -1.f, 0, BlastTime > 0.f ? 6.f : 2.f,
		FVector(0.f, 1.f, 0.f), FVector(1.f, 0.f, 0.f), false);
	if (BlastTime > 0.f)
	{
		DrawDebugSphere(GetWorld(), LastBlast, Radius * (1.f - BlastTime / 1.5f), 24, FColor::Orange);
	}

	int32 Flying = 0;
	for (AStaticMeshActor* Box : Boxes)
	{
		if (Box && Box->GetVelocity().Size() > 50.f)
		{
			++Flying;
		}
	}
	SetStatus(FString::Printf(TEXT("RadialForce 반경 %.0f · ImpulseStrength %.0f (VelChange, Linear 감쇠)\n폭발 %d회 · 날아가는 상자 %d개"),
		Radius, ImpulseStrength, Explosions, Flying));
}
