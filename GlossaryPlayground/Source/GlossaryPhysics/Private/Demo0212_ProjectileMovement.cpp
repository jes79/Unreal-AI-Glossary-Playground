#include "Demo0212_ProjectileMovement.h"

#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GlossaryKit.h"
#include "PhysicsDemoUtil.h"

using namespace GlossaryKit;

namespace
{
	const FVector Muzzle(-800.f, 0.f, 150.f);
	const float GrenadeLaneY = 160.f;
	const float TargetX = 1450.f;
	const float ThrowPitch = 35.f;
}

ADemo0212_ProjectileMovement::ADemo0212_ProjectileMovement()
{
	DemoId = TEXT("0212");
	Controls = TEXT("E : 총알 발사 · G : 수류탄 던지기 · 1 : 자동 발사 ON/OFF\n")
		TEXT("관찰: 총알(초속 3000)은 거의 직선으로 날아가 과녁에 박히고, 수류탄(Should Bounce)은 포물선으로 튀다가 3초 뒤 터진다");
	Showcase = TEXT("ProjectileMovementComponent 초속 3000 총알의 직선 궤적 vs Should Bounce 수류탄의 튀는 포물선 궤적");
	PlayerPawnClass = nullptr;
}

void ADemo0212_ProjectileMovement::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	Camera = PhysicsDemoUtil::SpawnFixedCamera(World, FVector(250.f, -1750.f, 380.f), FRotator(-6.f, 90.f, 0.f), 80.f);

	SpawnShape(World, EShape::Cube, Muzzle - FVector(60.f, 0.f, 75.f), FVector(120.f, 60.f, 150.f), Colors::Dark);
	SpawnShape(World, EShape::Cube, FVector(Muzzle.X - 60.f, GrenadeLaneY, 75.f), FVector(120.f, 60.f, 150.f), Colors::Dark);
	// 과녁 벽과 수류탄이 튈 낮은 턱
	SpawnShape(World, EShape::Cube, FVector(TargetX, 80.f, 200.f), FVector(40.f, 400.f, 400.f), Colors::Wall);
	SpawnShape(World, EShape::Cube, FVector(700.f, GrenadeLaneY, 30.f), FVector(80.f, 200.f, 60.f), Colors::Platform);
	SpawnLabel(World, TEXT("총알 3000cm/s (중력 0.1)"), Muzzle + FVector(0.f, 0.f, 120.f), Colors::Item, 18.f);
	SpawnLabel(World, TEXT("수류탄 Should Bounce"), FVector(Muzzle.X, GrenadeLaneY, 280.f), Colors::Ally, 18.f);
}

UProjectileMovementComponent* ADemo0212_ProjectileMovement::AddMovement(AStaticMeshActor* Actor, const FVector& Velocity)
{
	UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
	Mesh->SetCollisionObjectType(ECC_GameTraceChannel1);              // Projectile 채널
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);   // 발사체끼리는 무시
	UProjectileMovementComponent* Move = NewObject<UProjectileMovementComponent>(Actor);
	Move->SetUpdatedComponent(Mesh);
	Move->InitialSpeed = Velocity.Size();
	Move->MaxSpeed = Velocity.Size();
	Move->Velocity = Velocity;
	Move->bRotationFollowsVelocity = true;
	return Move;
}

void ADemo0212_ProjectileMovement::FireBullet()
{
	AStaticMeshActor* Bullet = SpawnShape(GetWorld(), EShape::Sphere, Muzzle + FVector(40.f, 0.f, 15.f), FVector(18.f), Colors::Item);
	SetColor(Bullet->GetStaticMeshComponent(), Colors::Item, 0.3f, 0.f, 6.f);
	// 핵심: 초속 3000, 튀지 않음 → 맞으면 멈춤(OnProjectileStop)
	UProjectileMovementComponent* Move = AddMovement(Bullet, FVector(BulletSpeed, 0.f, 0.f));
	Move->ProjectileGravityScale = BulletGravityScale;
	Move->bShouldBounce = false;
	Move->RegisterComponent();
	Bullet->SetLifeSpan(2.f);
	Tracked.Add({ Bullet, false, 0.f });
}

void ADemo0212_ProjectileMovement::ThrowGrenade()
{
	const FVector Start(Muzzle.X + 50.f, GrenadeLaneY, Muzzle.Z + 30.f);
	AStaticMeshActor* Grenade = SpawnShape(GetWorld(), EShape::Sphere, Start, FVector(30.f), Colors::Ally);
	const FVector Dir = FRotator(ThrowPitch, 0.f, 0.f).Vector();
	// 핵심: Should Bounce + Bounciness(반발) + Friction(구를 때 감속)
	UProjectileMovementComponent* Move = AddMovement(Grenade, Dir * GrenadeSpeed);
	Move->ProjectileGravityScale = 1.f;
	Move->bShouldBounce = true;
	Move->Bounciness = GrenadeBounciness;
	Move->Friction = 0.3f;
	Move->OnProjectileBounce.AddDynamic(this, &ADemo0212_ProjectileMovement::OnGrenadeBounce);
	Move->RegisterComponent();
	Tracked.Add({ Grenade, true, 0.f });
}

void ADemo0212_ProjectileMovement::OnGrenadeBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	++Bounces;
	DrawDebugCircle(GetWorld(), ImpactResult.ImpactPoint + FVector(0.f, 0.f, 2.f), 25.f, 16, FColor::Green, false, 2.f, 0, 2.f,
		FVector(0.f, 1.f, 0.f), FVector(1.f, 0.f, 0.f), false);
}

void ADemo0212_ProjectileMovement::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PhysicsDemoUtil::KeepViewTarget(this, Camera);

	if (WasKeyPressed(EKeys::One)) { bAutoFire = !bAutoFire; }
	if (WasKeyPressed(EKeys::E)) { FireBullet(); }
	if (WasKeyPressed(EKeys::G)) { ThrowGrenade(); }
	if (bAutoFire)
	{
		AutoTimer += DeltaSeconds;
		if (AutoTimer >= 1.5f)
		{
			AutoTimer = 0.f;
			(AutoIndex++ % 2 == 0) ? ThrowGrenade() : FireBullet();
		}
	}

	for (FTracked& T : Tracked)
	{
		AStaticMeshActor* Actor = T.Actor.Get();
		if (!Actor)
		{
			continue;
		}
		T.Age += DeltaSeconds;
		// 궤적: 프레임마다 점을 남긴다 (3초 유지)
		DrawDebugPoint(GetWorld(), Actor->GetActorLocation(), 6.f, T.bGrenade ? FColor::Green : FColor::Yellow, false, 3.f);
		if (T.bGrenade && T.Age >= GrenadeFuse)
		{
			LastExplosion = Actor->GetActorLocation();
			ExplosionTime = 0.6f;
			Actor->Destroy();
		}
		else if (!T.bGrenade)
		{
			UProjectileMovementComponent* Move = Actor->FindComponentByClass<UProjectileMovementComponent>();
			if (Move && Move->UpdatedComponent == nullptr)
			{
				// OnProjectileStop 뒤에는 UpdatedComponent 가 비워진다 → 과녁에 박힘
				++BulletHits;
				Actor->SetLifeSpan(0.5f);
				T.Actor = nullptr;
			}
		}
	}
	Tracked.RemoveAll([](const FTracked& T) { return !T.Actor.IsValid(); });

	if (ExplosionTime > 0.f)
	{
		ExplosionTime -= DeltaSeconds;
		DrawDebugSphere(GetWorld(), LastExplosion, 250.f * (1.f - ExplosionTime / 0.6f) + 20.f, 20, FColor::Orange);
	}

	SetStatus(FString::Printf(TEXT("총알: InitialSpeed %.0f · 과녁 명중 %d\n수류탄: InitialSpeed %.0f · Bounciness %.1f · 튕김 %d회 · 자동 발사 %s"),
		BulletSpeed, BulletHits, GrenadeSpeed, GrenadeBounciness, Bounces, bAutoFire ? TEXT("ON") : TEXT("OFF")));
}
