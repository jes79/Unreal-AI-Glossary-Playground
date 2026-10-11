#include "Demo0201_CollisionPreset.h"

#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"
#include "PhysicsDemoUtil.h"

using namespace GlossaryKit;

namespace D0201Local
{
	const float LaneY[3] = { 100.f, 500.f, 900.f };
	const float TargetX = 600.f;
	const float WallX = 1300.f;
	const float BulletZ = 120.f;
	const FLinearColor TargetColors[3] = { Colors::Player, Colors::Neutral, Colors::Accent };
	const TCHAR* TargetNames[3] = { TEXT("Pawn"), TEXT("WorldDynamic"), TEXT("Projectile") };
}
using namespace D0201Local;

ADemo0201_CollisionPreset::ADemo0201_CollisionPreset()
{
	DemoId = TEXT("0201");
	Controls = TEXT("자동 발사(1.2초마다) · E : 지금 발사\n")
		TEXT("1 : Projectile 프리셋 ↔ BlockAll 프리셋 전환\n")
		TEXT("관찰: Projectile 프리셋 총알은 Pawn(파랑)은 Overlap 으로 통과, 상자는 Block, 다른 Projectile(주황)은 Ignore");
	Showcase = TEXT("Projectile 오브젝트 채널 총알이 Pawn=Overlap·WorldDynamic=Block·Projectile=Ignore 로 다르게 반응, BlockAll 프리셋과 비교");
	PlayerPawnClass = nullptr;
}

void ADemo0201_CollisionPreset::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	Camera = PhysicsDemoUtil::SpawnFixedCamera(World, FVector(-750.f, 330.f, 1150.f), FRotator(-45.f, 0.f, 0.f), 80.f);

	for (int32 Lane = 0; Lane < 3; ++Lane)
	{
		const float Y = LaneY[Lane];
		// 발사대
		SpawnShape(World, EShape::Cylinder, FVector(0.f, Y, BulletZ * 0.5f), FVector(80.f, 80.f, BulletZ), Colors::Dark);

		// 레인 가운데 대상: Pawn / WorldDynamic / Projectile 오브젝트 타입
		AStaticMeshActor* Target = nullptr;
		if (Lane == 0)
		{
			Target = SpawnShape(World, EShape::Cylinder, FVector(TargetX, Y, 100.f), FVector(90.f, 90.f, 200.f), TargetColors[Lane]);
			Target->GetStaticMeshComponent()->SetCollisionObjectType(ECC_Pawn);
		}
		else if (Lane == 1)
		{
			Target = SpawnShape(World, EShape::Cube, FVector(TargetX, Y, 75.f), FVector(150.f, 150.f, 150.f), TargetColors[Lane]);
			Target->GetStaticMeshComponent()->SetCollisionObjectType(ECC_WorldDynamic);
		}
		else
		{
			Target = SpawnShape(World, EShape::Sphere, FVector(TargetX, Y, BulletZ), FVector(110.f), TargetColors[Lane]);
			Target->GetStaticMeshComponent()->SetCollisionObjectType(ProjectileChannel);
		}
		UStaticMeshComponent* TargetMesh = Target->GetStaticMeshComponent();
		TargetMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		TargetMesh->SetCollisionResponseToAllChannels(ECR_Block);
		TargetMesh->SetGenerateOverlapEvents(true);
		Targets.Add(Target);
		TargetLabels.Add(SpawnLabel(World, TEXT(""), FVector(TargetX, Y, 300.f), FLinearColor::White, 18.f));

		// 레인 끝 벽 (WorldStatic)
		AStaticMeshActor* Wall = SpawnShape(World, EShape::Cube, FVector(WallX, Y, 150.f), FVector(40.f, 300.f, 300.f), Colors::Wall);
		Wall->GetStaticMeshComponent()->SetCollisionObjectType(ECC_WorldStatic);
		Wall->Tags.Add(TEXT("Wall"));
	}
	SpawnLabel(World, TEXT("벽 = WorldStatic → Block"), FVector(WallX, LaneY[1], 380.f), Colors::Item, 18.f);
	RefreshLabels();
	FireVolley();
}

void ADemo0201_CollisionPreset::ApplyBulletPreset(UPrimitiveComponent* Bullet) const
{
	// 핵심: 총알의 Object Type 을 Projectile 채널로, 채널별 응답을 프리셋처럼 지정
	Bullet->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Bullet->SetCollisionObjectType(ProjectileChannel);
	Bullet->SetCollisionResponseToAllChannels(ECR_Block);
	if (bUseProjectilePreset)
	{
		Bullet->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);        // 캐릭터는 통과하며 '맞았다'만 감지
		Bullet->SetCollisionResponseToChannel(ProjectileChannel, ECR_Ignore); // 총알끼리는 완전히 무시
		Bullet->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		Bullet->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}
	Bullet->SetGenerateOverlapEvents(true);
}

void ADemo0201_CollisionPreset::FireVolley()
{
	UWorld* World = GetWorld();
	for (int32 Lane = 0; Lane < 3; ++Lane)
	{
		AStaticMeshActor* Bullet = SpawnShape(World, EShape::Sphere, FVector(60.f, LaneY[Lane], BulletZ), FVector(30.f), Colors::Item,
			FRotator::ZeroRotator, false, true);
		SetColor(Bullet->GetStaticMeshComponent(), Colors::Item, 0.4f, 0.f, 4.f);
		UStaticMeshComponent* Mesh = Bullet->GetStaticMeshComponent();
		ApplyBulletPreset(Mesh);
		Mesh->OnComponentHit.AddDynamic(this, &ADemo0201_CollisionPreset::OnBulletHit);
		Mesh->OnComponentBeginOverlap.AddDynamic(this, &ADemo0201_CollisionPreset::OnBulletOverlap);

		UProjectileMovementComponent* Move = NewObject<UProjectileMovementComponent>(Bullet);
		Move->SetUpdatedComponent(Mesh);
		Move->InitialSpeed = BulletSpeed;
		Move->MaxSpeed = BulletSpeed;
		Move->ProjectileGravityScale = 0.f;
		Move->Velocity = FVector(BulletSpeed, 0.f, 0.f);
		Move->RegisterComponent();
		Bullet->SetLifeSpan(3.f);
		Bullets.Add(Bullet);
	}
}

void ADemo0201_CollisionPreset::OnBulletHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Block 응답 → OnComponentHit. 총알은 그 자리에서 사라진다
	++HitCount;
	Flash(OtherActor, TEXT("Hit (Block)"));
	if (AActor* Bullet = HitComp ? HitComp->GetOwner() : nullptr)
	{
		Bullet->Destroy();
	}
}

void ADemo0201_CollisionPreset::OnBulletOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Overlap 응답 → 막히지 않고 통과하면서 BeginOverlap 만 온다
	++OverlapCount;
	Flash(OtherActor, TEXT("Overlap (통과)"));
}

void ADemo0201_CollisionPreset::Flash(AActor* Target, const FString& Event)
{
	const int32 Index = Targets.IndexOfByKey(Target);
	FString Name = Target ? Target->GetActorNameOrLabel() : TEXT("?");
	if (Index != INDEX_NONE)
	{
		FlashTimers.Add(Target, 0.25f);
		if (AStaticMeshActor* SM = Cast<AStaticMeshActor>(Target))
		{
			SetColor(SM->GetStaticMeshComponent(), TargetColors[Index], 0.5f, 0.f, 6.f);
		}
		Name = TargetNames[Index];
	}
	else if (Target && Target->Tags.Contains(TEXT("Wall")))
	{
		Name = TEXT("벽(WorldStatic)");
	}
	EventLog.Insert(FString::Printf(TEXT("%s ← %s"), *Name, *Event), 0);
	if (EventLog.Num() > 3)
	{
		EventLog.SetNum(3);
	}
}

void ADemo0201_CollisionPreset::RefreshLabels()
{
	// 대상과 총알 응답 중 '약한 쪽'이 최종 결과 (Ignore < Overlap < Block)
	const TCHAR* Results[3];
	if (bUseProjectilePreset)
	{
		Results[0] = TEXT("Pawn → Overlap (통과)");
		Results[1] = TEXT("WorldDynamic → Block");
		Results[2] = TEXT("Projectile → Ignore (무시)");
	}
	else
	{
		Results[0] = TEXT("Pawn → Block");
		Results[1] = TEXT("WorldDynamic → Block");
		Results[2] = TEXT("Projectile → Block");
	}
	for (int32 i = 0; i < TargetLabels.Num(); ++i)
	{
		if (TargetLabels[i])
		{
			TargetLabels[i]->SetText(Results[i]);
		}
	}
}

void ADemo0201_CollisionPreset::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PhysicsDemoUtil::KeepViewTarget(this, Camera);

	if (WasKeyPressed(EKeys::One))
	{
		bUseProjectilePreset = !bUseProjectilePreset;
		RefreshLabels();
	}
	FireTimer += DeltaSeconds;
	if (WasKeyPressed(EKeys::E) || FireTimer >= FireInterval)
	{
		FireTimer = 0.f;
		FireVolley();
	}

	// 깜빡임 복구
	for (auto It = FlashTimers.CreateIterator(); It; ++It)
	{
		It.Value() -= DeltaSeconds;
		if (It.Value() <= 0.f)
		{
			if (AStaticMeshActor* SM = Cast<AStaticMeshActor>(It.Key().Get()))
			{
				const int32 Index = Targets.IndexOfByKey(SM);
				SetColor(SM->GetStaticMeshComponent(), TargetColors[FMath::Max(Index, 0)]);
			}
			It.RemoveCurrent();
		}
	}
	Bullets.RemoveAll([](const TObjectPtr<AActor>& B) { return !IsValid(B); });

	SetStatus(FString::Printf(TEXT("현재 총알 프리셋: %s (Object Type = Projectile)\nHit %d회 · Overlap %d회\n%s"),
		bUseProjectilePreset ? TEXT("Projectile (Pawn Overlap · Projectile Ignore)") : TEXT("BlockAll"),
		HitCount, OverlapCount, *FString::Join(EventLog, TEXT("\n"))));
}
