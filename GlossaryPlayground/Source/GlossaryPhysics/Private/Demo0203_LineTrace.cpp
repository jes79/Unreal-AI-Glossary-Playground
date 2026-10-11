#include "Demo0203_LineTrace.h"

#include "Camera/PlayerCameraManager.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GlossaryCharacter.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"

using namespace GlossaryKit;

ADemo0203_LineTrace::ADemo0203_LineTrace()
{
	DemoId = TEXT("0203");
	Controls = TEXT("WASD 이동 · 마우스 시점 — 화면 가운데(카메라 정면)로 선을 쏜다\n")
		TEXT("[ / ] : 트레이스 길이 -250 / +250cm\n")
		TEXT("관찰: 1000cm 안에 맞으면 초록 선 + 이름, 멀면 빨간 선 + 'None'. 플레이어 자신은 무시");
	Showcase = TEXT("카메라 정면 LineTraceSingleByChannel 1000cm, 맞은 액터 이름·거리 표시 + DrawDebugLine");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-650.f, 0.f, 100.f));
}

void ADemo0203_LineTrace::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 이름(태그)이 붙은 대상들: 거리가 제각각
	auto Place = [&](EShape Shape, const FVector& Loc, const FVector& Size, const FLinearColor& Color, const TCHAR* Name) -> AActor*
	{
		AStaticMeshActor* A = SpawnShape(World, Shape, Loc, Size, Color);
		A->Tags.Add(Name);
		SpawnLabel(World, Name, Loc + FVector(0.f, 0.f, Size.Z * 0.5f + 60.f), FLinearColor::White, 16.f);
		return A;
	};
	Place(EShape::Cube, FVector(-150.f, 0.f, 100.f), FVector(120.f, 160.f, 200.f), Colors::Platform, TEXT("Crate_A"));
	Place(EShape::Cylinder, FVector(350.f, -450.f, 90.f), FVector(110.f, 110.f, 180.f), Colors::Enemy, TEXT("Barrel_B"));
	Place(EShape::Cube, FVector(1200.f, 350.f, 150.f), FVector(60.f, 300.f, 300.f), Colors::Ally, TEXT("FarWall_C (1000cm 밖)"));
	Mover = Place(EShape::Sphere, FVector(500.f, 0.f, 160.f), FVector(90.f), Colors::Item, TEXT("Drone_D"));

	HitLabel = SpawnLabel(World, TEXT(""), FVector::ZeroVector, Colors::Ally, 18.f);
}

void ADemo0203_LineTrace::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;

	if (WasKeyPressed(EKeys::LeftBracket))  { TraceDistance = FMath::Max(250.f, TraceDistance - 250.f); }
	if (WasKeyPressed(EKeys::RightBracket)) { TraceDistance += 250.f; }

	// 좌우로 오가는 드론 (트레이스가 움직이는 대상도 잡는 것을 보여 줌)
	if (Mover)
	{
		Mover->SetActorLocation(FVector(500.f, FMath::Sin(Time * 0.8f) * 500.f, 160.f));
	}

	// 어깨 너머 시점: 화면 가운데(조준점)가 캐릭터에 가리지 않게 카메라를 오른쪽 위로
	if (AGlossaryCharacter* Character = Cast<AGlossaryCharacter>(GetPlayerPawn()))
	{
		Character->GetCameraBoom()->SocketOffset = FVector(0.f, 90.f, 60.f);
	}

	APlayerController* PC = GetPC();
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}

	// 핵심: 카메라 위치에서 카메라 정면으로 TraceDistance 만큼
	const FVector Start = PC->PlayerCameraManager->GetCameraLocation();
	const FVector End = Start + PC->PlayerCameraManager->GetCameraRotation().Vector() * TraceDistance;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GlossaryLineTrace), false);
	Params.AddIgnoredActor(GetPlayerPawn());   // 내 캐릭터는 맞지 않게
	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params);

	// 카메라에서 쏜 선은 화면에서 점으로만 보이므로, 캐릭터 어깨에서 같은 끝점까지 그려 시각화
	const FVector Visual = GetPlayerPawn() ? GetPlayerPawn()->GetActorLocation() + FVector(0.f, 0.f, 50.f) : Start;
	const FVector Tip = bHit ? Hit.ImpactPoint : End;
	DrawDebugLine(GetWorld(), Visual, Tip, bHit ? FColor::Green : FColor::Red, false, -1.f, 0, 3.f);
	DrawDebugSphere(GetWorld(), Tip, 12.f, 12, bHit ? FColor::Green : FColor::Red);

	FString Name = TEXT("None");
	if (bHit)
	{
		AActor* HitActor = Hit.GetActor();
		Name = HitActor && HitActor->Tags.Num() > 0 ? HitActor->Tags[0].ToString() : GetNameSafe(HitActor);
		DrawDebugDirectionalArrow(GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + Hit.ImpactNormal * 80.f, 20.f, FColor::Cyan, false, -1.f, 0, 2.f);
		HitLabel->SetActorHiddenInGame(false);
		HitLabel->SetActorLocation(Hit.ImpactPoint + FVector(0.f, 0.f, 40.f));
		HitLabel->SetText(FString::Printf(TEXT("Hit: %s"), *Name));
	}
	else
	{
		HitLabel->SetActorHiddenInGame(true);
	}

	SetStatus(FString::Printf(TEXT("LineTraceSingleByChannel(Visibility, %.0fcm)\n%s"),
		TraceDistance,
		bHit ? *FString::Printf(TEXT("맞음: %s · 거리 %.0fcm · 표면 %s"), *Name, Hit.Distance, *Hit.ImpactNormal.ToCompactString())
		     : TEXT("맞은 것 없음 (None) — 거리 안에 아무것도 없다")));
}
