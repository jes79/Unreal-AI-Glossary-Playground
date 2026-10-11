#include "Demo0206_PhysicalMaterial.h"

#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryKit.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicsDemoUtil.h"

using namespace GlossaryKit;

namespace
{
	const float RampLength = 600.f;
	const float RampRowX[2] = { 350.f, -50.f };     // 0 기본(뒤), 1 얼음(앞)
	const float BallY[2] = { 950.f, 1200.f };      // 0 기본, 1 고무
	const float BallDropZ = 450.f;
}

ADemo0206_PhysicalMaterial::ADemo0206_PhysicalMaterial()
{
	DemoId = TEXT("0206");
	Controls = TEXT("R : 다시 시작 (7초마다 자동)\n")
		TEXT("관찰: 같은 20° 경사에서 기본 상자는 멈춰 있고 얼음(Friction 0.05)은 미끄러진다.\n")
		TEXT("같은 높이에서 떨어진 공: 기본은 거의 안 튀고 고무공(Restitution 0.9)은 높이 튄다");
	Showcase = TEXT("PM_Ice(Friction 0.05) 상자 미끄러짐 vs 기본, PM_Rubber(Restitution 0.9) 공 튀기 vs 기본");
	PlayerPawnClass = nullptr;
}

void ADemo0206_PhysicalMaterial::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	Camera = PhysicsDemoUtil::SpawnFixedCamera(World, FVector(-1150.f, 640.f, 520.f), FRotator(-17.f, 0.f, 0.f), 75.f);

	// 빌더가 만든 에셋(/Game/Glossary/02_Physics/PM_*) — 없으면 같은 값으로 런타임 생성
	if (!IceMaterial)     IceMaterial = PhysicsDemoUtil::LoadOrMakePhysMat(this, TEXT("/Game/Glossary/02_Physics/PM_Ice"), 0.05f, 0.1f, true, false);
	if (!RubberMaterial)  RubberMaterial = PhysicsDemoUtil::LoadOrMakePhysMat(this, TEXT("/Game/Glossary/02_Physics/PM_Rubber"), 0.8f, 0.9f, false, true);
	if (!DefaultMaterial) DefaultMaterial = PhysicsDemoUtil::LoadOrMakePhysMat(this, TEXT("/Game/Glossary/02_Physics/PM_Default"), 0.7f, 0.1f, false, false);

	const float Rad = FMath::DegreesToRadians(RampAngle);
	const FVector Down(0.f, FMath::Cos(Rad), -FMath::Sin(Rad));    // 경사 아래 방향
	const FVector Normal(0.f, FMath::Sin(Rad), FMath::Cos(Rad));    // 경사면 법선
	const FRotator RampRot(0.f, 0.f, -RampAngle);

	for (int32 i = 0; i < 2; ++i)
	{
		const bool bIce = i == 1;
		UPhysicalMaterial* PM = bIce ? IceMaterial.Get() : DefaultMaterial.Get();
		const FVector Top(RampRowX[i], 0.f, RampLength * FMath::Sin(Rad));
		AStaticMeshActor* Ramp = SpawnShape(World, EShape::Cube, Top + Down * RampLength * 0.5f - Normal * 10.f, FVector(220.f, RampLength, 20.f),
			bIce ? FLinearColor(0.6f, 0.85f, 1.f) : Colors::Platform, RampRot);
		Ramp->GetStaticMeshComponent()->SetPhysMaterialOverride(PM);
		SetColor(Ramp->GetStaticMeshComponent(), bIce ? FLinearColor(0.6f, 0.85f, 1.f) : Colors::Platform, bIce ? 0.05f : 0.8f, 0.f, 0.f);

		// 경사 위 상자
		const FTransform Start(RampRot, Top + Down * 90.f + Normal * 31.f);
		SliderStarts.Add(Start);
		AStaticMeshActor* Box = SpawnShape(World, EShape::Cube, Start.GetLocation(), FVector(60.f), bIce ? Colors::Cyan : Colors::Accent, RampRot, true);
		Box->GetStaticMeshComponent()->SetPhysMaterialOverride(PM);   // 핵심: 컴포넌트에 Physical Material 지정
		Sliders.Add(Box);
		SpawnLabel(World, bIce ? TEXT("얼음 PM_Ice (Friction 0.05)") : TEXT("기본 (Friction 0.7)"), Top + FVector(0.f, -60.f, 120.f),
			bIce ? Colors::Cyan : FLinearColor::White, 18.f);
	}

	for (int32 i = 0; i < 2; ++i)
	{
		const bool bRubber = i == 1;
		const FVector Start(150.f, BallY[i], BallDropZ);
		BallStarts.Add(Start);
		AStaticMeshActor* Ball = SpawnShape(World, EShape::Sphere, Start, FVector(70.f), bRubber ? Colors::Enemy : Colors::Neutral, FRotator::ZeroRotator, true);
		Ball->GetStaticMeshComponent()->SetPhysMaterialOverride(bRubber ? RubberMaterial.Get() : DefaultMaterial.Get());
		Balls.Add(Ball);
		// 높이 눈금 기둥
		SpawnShape(World, EShape::Cube, FVector(260.f, BallY[i], BallDropZ * 0.5f), FVector(8.f, 8.f, BallDropZ), Colors::Dark);
		SpawnLabel(World, bRubber ? TEXT("고무공 PM_Rubber (Restitution 0.9)") : TEXT("기본 공 (Restitution 0.1)"), Start + FVector(0.f, 0.f, 110.f),
			bRubber ? Colors::Enemy : FLinearColor::White, 18.f);
		MaxBounce.Add(0.f);
		BounceCount.Add(0);
		bWasFalling.Add(true);
	}
}

void ADemo0206_PhysicalMaterial::ResetObjects()
{
	for (int32 i = 0; i < Sliders.Num(); ++i)
	{
		UStaticMeshComponent* Mesh = Sliders[i]->GetStaticMeshComponent();
		Mesh->SetWorldLocationAndRotation(SliderStarts[i].GetLocation(), SliderStarts[i].GetRotation(), false, nullptr, ETeleportType::ResetPhysics);
		Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}
	for (int32 i = 0; i < Balls.Num(); ++i)
	{
		UStaticMeshComponent* Mesh = Balls[i]->GetStaticMeshComponent();
		Mesh->SetWorldLocation(BallStarts[i], false, nullptr, ETeleportType::ResetPhysics);
		Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		MaxBounce[i] = 0.f;
		BounceCount[i] = 0;
		bWasFalling[i] = true;
	}
}

void ADemo0206_PhysicalMaterial::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PhysicsDemoUtil::KeepViewTarget(this, Camera);

	Timer += DeltaSeconds;
	if (WasKeyPressed(EKeys::R) || Timer >= ResetInterval)
	{
		Timer = 0.f;
		ResetObjects();
	}

	// 공: 아래로 떨어지다 위로 바뀌는 순간 = 바운스. 그 뒤 최고 높이를 기록
	for (int32 i = 0; i < Balls.Num(); ++i)
	{
		const FVector Velocity = Balls[i]->GetStaticMeshComponent()->GetPhysicsLinearVelocity();
		const bool bFalling = Velocity.Z < -1.f;
		if (bWasFalling[i] && Velocity.Z > 1.f)
		{
			++BounceCount[i];
		}
		bWasFalling[i] = bFalling;
		if (BounceCount[i] > 0)
		{
			MaxBounce[i] = FMath::Max(MaxBounce[i], Balls[i]->GetActorLocation().Z - 35.f);
		}
	}
	const float SlideDefault = FVector::Dist(Sliders[0]->GetActorLocation(), SliderStarts[0].GetLocation());
	const float SlideIce = FVector::Dist(Sliders[1]->GetActorLocation(), SliderStarts[1].GetLocation());

	SetStatus(FString::Printf(TEXT("미끄러진 거리 — 기본 %.0fcm · 얼음 %.0fcm\n첫 바운스 후 최고 높이 (낙하 %.0fcm) — 기본 %.0fcm · 고무 %.0fcm"),
		SlideDefault, SlideIce, BallDropZ - 35.f, MaxBounce[0], MaxBounce[1]));
}
