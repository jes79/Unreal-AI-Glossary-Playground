#include "Demo0510_Beam.h"

#include "EffectsDemoKit.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryKit.h"
#include "NiagaraComponent.h"

using namespace GlossaryKit;

ADemo0510_Beam::ADemo0510_Beam()
{
	DemoId = TEXT("0510");
	Controls = TEXT("C : 끝점 움직임 멈춤/재개\n")
		TEXT("관찰: 빔 이미터는 시작·끝 위치를 User.BeamStart / User.BeamEnd 파라미터로 받는다.\n")
		TEXT("C++ 가 매 프레임 SetVariablePosition 으로 두 구슬 위치를 넘기므로 빔이 구슬을 따라간다");
	Showcase = TEXT("두 구슬 사이를 잇는 번개 빔, 끝점을 User Parameter(BeamStart/BeamEnd)로 매 프레임 갱신");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Night;
	Environment.ExposureEV100 = 3.f;
}

void ADemo0510_Beam::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	StartOrb = SpawnShape(World, EShape::Sphere, FVector(0.f, -400.f, 200.f), FVector(50.f), Colors::Cyan);
	EndOrb = SpawnShape(World, EShape::Sphere, FVector(0.f, 400.f, 200.f), FVector(50.f), Colors::Purple);
	if (AStaticMeshActor* A = Cast<AStaticMeshActor>(StartOrb.Get())) SetColor(A->GetStaticMeshComponent(), Colors::Cyan, 0.3f, 0.f, 6.f);
	if (AStaticMeshActor* B = Cast<AStaticMeshActor>(EndOrb.Get())) SetColor(B->GetStaticMeshComponent(), Colors::Purple, 0.3f, 0.f, 6.f);
	SpawnShape(World, EShape::Cylinder, FVector(0.f, -400.f, 85.f), FVector(20.f, 20.f, 170.f), Colors::Wall);

	Beam = EffectsKit::AddNiagara(this, TEXT("NS_Beam"), FVector::ZeroVector);
	SpawnLabel(World, TEXT("User.BeamStart"), FVector(0.f, -400.f, 300.f), Colors::Cyan, 18.f);
	EffectsKit::SetupFixedCamera(this, FVector(-1100.f, 0.f, 350.f), FVector(0.f, 0.f, 220.f), 65.f);
}

void ADemo0510_Beam::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::C)) { bMoving = !bMoving; }
	if (bMoving) { Angle += OrbitSpeed * DeltaSeconds; }

	// 끝점 구슬은 원을 그리며 돈다
	const float Rad = FMath::DegreesToRadians(Angle);
	const FVector EndPos(FMath::Sin(Rad) * 250.f, 350.f + FMath::Cos(Rad) * 150.f, 220.f + FMath::Sin(Rad * 2.f) * 150.f);
	if (EndOrb) EndOrb->SetActorLocation(EndPos);
	const FVector StartPos = StartOrb ? StartOrb->GetActorLocation() : FVector::ZeroVector;

	if (Beam)
	{
		// 핵심: 빔 끝점 = User 파라미터 (Position 타입)
		Beam->SetVariablePosition(TEXT("BeamStart"), StartPos);
		Beam->SetVariablePosition(TEXT("BeamEnd"), EndPos);
	}
	SetStatus(FString::Printf(TEXT("BeamStart (%.0f, %.0f, %.0f) → BeamEnd (%.0f, %.0f, %.0f) · 길이 %.0f cm"),
		StartPos.X, StartPos.Y, StartPos.Z, EndPos.X, EndPos.Y, EndPos.Z, FVector::Dist(StartPos, EndPos)));
}
