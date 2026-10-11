#include "Demo0211_ChaosDestruction.h"

#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "Field/FieldSystemComponent.h"
#include "Field/FieldSystemObjects.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GeometryCollection/GeometryCollection.h"
#include "GeometryCollection/GeometryCollectionActor.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "GeometryCollection/GeometryCollectionObject.h"
#include "GlossaryKit.h"
#include "PhysicsDemoUtil.h"

using namespace GlossaryKit;

namespace D0211Local
{
	const FVector WallLocation(400.f, 0.f, 0.f);
}
using namespace D0211Local;

ADemo0211_ChaosDestruction::ADemo0211_ChaosDestruction()
{
	DemoId = TEXT("0211");
	Controls = TEXT("E : 화면 가운데로 총 쏘기 · 좌클릭 : 클릭한 곳으로 쏘기 · R : 벽 다시 세우기\n")
		TEXT("관찰: 총알이 맞은 자리 반경 130 에 Field(External Strain)가 걸려 Voronoi 조각이 떨어져 나간다");
	Showcase = TEXT("코드로 Voronoi 20조각 낸 Geometry Collection 벽, 총알이 맞은 곳에 Field System(Strain+Radial Velocity)으로 파괴");
	PlayerPawnClass = nullptr;
	bShowMouseCursor = true;

	FieldSystem = CreateDefaultSubobject<UFieldSystemComponent>(TEXT("FieldSystem"));
	FieldSystem->SetupAttachment(Root);
}

void ADemo0211_ChaosDestruction::BeginPlay()
{
	Super::BeginPlay();
	if (!WallCollection)
	{
		WallCollection = LoadAsset<UGeometryCollection>(TEXT("/Game/Glossary/02_Physics/GC_Wall"));
	}
	Camera = PhysicsDemoUtil::SpawnFixedCamera(GetWorld(), FVector(-650.f, -260.f, 260.f), FRotator(-6.f, 12.f, 0.f), 75.f);
	SpawnWall();
}

void ADemo0211_ChaosDestruction::SpawnWall()
{
	if (WallActor)
	{
		WallActor->Destroy();
	}
	if (!WallCollection)
	{
		SetStatus(TEXT("GC_Wall 에셋이 없습니다 — tools/glossary-build.sh -filter=02 로 생성"));
		return;
	}
	// Geometry Collection Actor 를 지연 스폰: 물리 상태가 만들어지기 전에 Rest Collection 을 지정
	const FTransform Xf(FRotator(0.f, 90.f, 0.f), WallLocation);   // 긴 변(X)을 카메라 정면(Y)으로
	AGeometryCollectionActor* GCActor = GetWorld()->SpawnActorDeferred<AGeometryCollectionActor>(AGeometryCollectionActor::StaticClass(), Xf);
	UGeometryCollectionComponent* GC = GCActor->GetGeometryCollectionComponent();
	GC->SetRestCollection(WallCollection);
	GC->ObjectType = EObjectStateTypeEnum::Chaos_Object_Dynamic;
	GC->bNotifyBreaks = true;
	GC->SetNotifyBreaks(true);
	GC->OnChaosBreakEvent.AddDynamic(this, &ADemo0211_ChaosDestruction::OnWallBreak);
	GCActor->FinishSpawning(Xf);
	WallActor = GCActor;
	WallComponent = GC;

	if (const TSharedPtr<FGeometryCollection, ESPMode::ThreadSafe> Collection = WallCollection->GetGeometryCollection())
	{
		NumPieces = 0;
		const TManagedArray<TSet<int32>>& BoneChildren = Collection->Children;
		for (int32 i = 0; i < BoneChildren.Num(); ++i)
		{
			NumPieces += BoneChildren[i].Num() == 0 ? 1 : 0;   // 잎(leaf) 뼈 = 실제 조각
		}
	}
	BreakEvents = 0;
}

void ADemo0211_ChaosDestruction::FireBullet(const FVector& Target)
{
	FVector Start, Dir;
	if (!PhysicsDemoUtil::GetViewRay(this, Start, Dir))
	{
		return;
	}
	Start += Dir * 100.f;
	const FVector Velocity = (Target - Start).GetSafeNormal() * BulletSpeed;
	AStaticMeshActor* Bullet = SpawnShape(GetWorld(), EShape::Sphere, Start, FVector(16.f), Colors::Item);
	SetColor(Bullet->GetStaticMeshComponent(), Colors::Item, 0.3f, 0.f, 5.f);
	UStaticMeshComponent* Mesh = Bullet->GetStaticMeshComponent();
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh->OnComponentHit.AddDynamic(this, &ADemo0211_ChaosDestruction::OnBulletHit);
	UProjectileMovementComponent* Move = NewObject<UProjectileMovementComponent>(Bullet);
	Move->SetUpdatedComponent(Mesh);
	Move->ProjectileGravityScale = 0.f;
	Move->Velocity = Velocity;
	Move->RegisterComponent();
	Bullet->SetLifeSpan(2.f);
	Bullets.Add(Bullet);
	++Shots;
}

void ADemo0211_ChaosDestruction::OnBulletHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	AActor* Bullet = HitComp ? HitComp->GetOwner() : nullptr;
	if (OtherActor == WallActor)
	{
		BreakAt(Hit.ImpactPoint, Bullet ? Bullet->GetVelocity().GetSafeNormal() : FVector::ForwardVector);
	}
	if (Bullet)
	{
		Bullet->Destroy();
	}
}

void ADemo0211_ChaosDestruction::BreakAt(const FVector& Location, const FVector& Direction)
{
	// 1) External Strain: 반경 안 클러스터 연결에 변형(Strain)을 걸어 Damage Threshold 를 넘기면 조각으로 분리
	FieldSystem->ApplyStrainField(true, Location, FieldRadius, StrainMagnitude, 2);

	// 2) Radial Velocity: 떨어진 조각을 맞은 지점에서 바깥으로 밀기 (반경 밖은 Culling 으로 제외)
	URadialFalloff* Mask = NewObject<URadialFalloff>(this);
	Mask->SetRadialFalloff(1.f, 0.f, 1.f, 0.f, FieldRadius * 1.5f, Location, EFieldFalloffType::Field_FallOff_None);
	URadialVector* Push = NewObject<URadialVector>(this);
	Push->SetRadialVector(PushVelocity, Location - Direction * 60.f);   // 총알 진행 방향으로 더 밀리게 중심을 앞쪽에
	UCullingField* Culled = NewObject<UCullingField>(this);
	Culled->SetCullingField(Mask, Push, EFieldCullingOperationType::Field_Culling_Outside);
	FieldSystem->ApplyPhysicsField(true, EFieldPhysicsType::Field_LinearVelocity, nullptr, Culled);

	LastBreak = Location;
	BreakFlash = 1.f;
}

void ADemo0211_ChaosDestruction::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PhysicsDemoUtil::KeepViewTarget(this, Camera);

	if (WasKeyPressed(EKeys::R))
	{
		SpawnWall();
	}
	FHitResult Hit;
	if (WasKeyPressed(EKeys::E) && PhysicsDemoUtil::TraceScreenCenter(this, Hit))
	{
		FireBullet(Hit.ImpactPoint);
	}
	if (WasKeyPressed(EKeys::LeftMouseButton) && PhysicsDemoUtil::TraceCursor(this, Hit))
	{
		FireBullet(Hit.ImpactPoint);
	}

	// 보험: 스윕이 GC 를 놓친 경우에도 벽 면을 지나는 순간 맞은 것으로 처리
	for (AStaticMeshActor* Bullet : Bullets)
	{
		if (IsValid(Bullet) && Bullet->GetActorLocation().X >= WallLocation.X - 60.f)
		{
			BreakAt(Bullet->GetActorLocation(), Bullet->GetVelocity().GetSafeNormal());
			Bullet->Destroy();
		}
	}
	Bullets.RemoveAll([](const TObjectPtr<AStaticMeshActor>& B) { return !IsValid(B); });

	if (BreakFlash > 0.f)
	{
		BreakFlash -= DeltaSeconds;
		DrawDebugSphere(GetWorld(), LastBreak, FieldRadius, 20, FColor::Orange, false, -1.f, 0, 1.5f);
	}

	SetStatus(FString::Printf(TEXT("GC_Wall: Voronoi %d조각 · 사격 %d회 · 떨어져 나간 조각(Break 이벤트) %d\nField: External Strain %.0f (반경 %.0f) + Radial Velocity %.0f"),
		NumPieces, Shots, BreakEvents, StrainMagnitude, FieldRadius, PushVelocity));
}

void ADemo0211_ChaosDestruction::OnWallBreak(const FChaosBreakEvent& BreakEvent)
{
	// bNotifyBreaks 를 켜면 클러스터에서 조각이 떨어질 때마다 호출된다
	++BreakEvents;
}
