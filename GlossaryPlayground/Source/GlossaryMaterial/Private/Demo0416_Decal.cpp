#include "Demo0416_Decal.h"

#include "Camera/CameraActor.h"
#include "Components/DecalComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

using namespace GlossaryKit;

ADemo0416_Decal::ADemo0416_Decal()
{
	DemoId = TEXT("0416");
	Controls = TEXT("왼쪽 클릭 : 커서 위치에 사격 → 총알 자국 데칼    A : 자동 사격 ON/OFF\n")
		TEXT("관찰: 자국은 벽·상자·바닥 어디든 표면에 붙고(메시를 바꾸지 않음), 10초 뒤 1초 동안 서서히 사라진다");
	Showcase = TEXT("맞은 지점에 Deferred Decal 총알 자국을 SpawnDecalAtLocation, 10초 뒤 페이드");
	bShowMouseCursor = true;
}

void ADemo0416_Decal::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	SetupFixedCamera(FVector(-700.f, -200.f, 260.f), FVector(200.f, -100.f, 150.f), 60.f);
	DecalMaterial = LoadMaterial(TEXT("M_BulletHoleDecal"));

	// 표적: 벽 + 상자 + 원기둥 (데칼은 모양과 상관없이 표면에 투사된다)
	SpawnBlock(World, FVector(300.f, 0.f, 0.f), FVector(40.f, 1200.f, 450.f), Colors::Wall);
	SpawnBlock(World, FVector(150.f, -250.f, 0.f), FVector(120.f, 120.f, 120.f), Colors::Platform, FRotator(0.f, 25.f, 0.f));
	SpawnShape(World, EShape::Cylinder, FVector(120.f, 250.f, 100.f), FVector(110.f, 110.f, 200.f), Colors::Accent);
	SpawnLabel(World, TEXT("M_BulletHoleDecal (Deferred Decal)"), FVector(280.f, 0.f, 490.f), Colors::Item, 16.f);
}

void ADemo0416_Decal::FireAt(const FVector& Start, const FVector& End)
{
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
	{
		return;
	}
	// 데칼은 자기 X 축 방향으로 투사된다 → 맞은 면의 노멀을 X 축으로, 회전(Roll)은 무작위
	FRotator Rotation = Hit.ImpactNormal.Rotation();
	Rotation.Roll = Random.FRandRange(0.f, 360.f);
	UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(this, DecalMaterial, DecalSize, Hit.ImpactPoint, Rotation);
	if (Decal)
	{
		Decal->SetFadeScreenSize(0.f);                 // 멀리서도 사라지지 않게
		Decal->SetFadeOut(FadeStartDelay, FadeDuration, false);  // 10초 뒤 1초 페이드 → 끝나면 컴포넌트 제거 (소유자는 WorldSettings 라 지우지 않는다)
		Decals.Add(Decal);
		++ShotCount;
	}
}

void ADemo0416_Decal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::A)) { bAutoFire = !bAutoFire; }
	if (WasKeyPressed(EKeys::LeftMouseButton))
	{
		FHitResult Hit;
		if (GetMouseWorldHit(Hit) && DemoCamera)
		{
			const FVector From = DemoCamera->GetActorLocation();
			FireAt(From, From + (Hit.ImpactPoint - From) * 1.1f);
		}
	}
	if (bAutoFire)
	{
		AutoFireTimer += DeltaSeconds;
		if (AutoFireTimer >= AutoFireInterval && DemoCamera)
		{
			AutoFireTimer = 0.f;
			// 카메라에서 벽·상자 쪽 무작위 지점으로
			const FVector Target(300.f, Random.FRandRange(-550.f, 450.f), Random.FRandRange(20.f, 420.f));
			const FVector From = DemoCamera->GetActorLocation();
			FireAt(From, From + (Target - From) * 1.2f);
		}
	}

	Decals.RemoveAll([](const TWeakObjectPtr<UDecalComponent>& Decal) { return !Decal.IsValid(); });
	SetStatus(FString::Printf(TEXT("사격 %d발 · 붙어 있는 데칼 %d개 (각자 %.0f초 뒤 %.0f초 페이드)%s"),
		ShotCount, Decals.Num(), FadeStartDelay, FadeDuration, bAutoFire ? TEXT(" · 자동 사격") : TEXT("")));
}
