#include "Demo1103_Interface.h"

#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

// ================================================================ 공통 베이스 (모양·펄스 연출만. 인터페이스는 구현하지 않는다)

AD1103_PropBase::AD1103_PropBase()
{
	PrimaryActorTick.bCanEverTick = true;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	RootComponent = Body;
}

void AD1103_PropBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Pulse > 0.f)
	{
		Pulse = FMath::Max(0.f, Pulse - DeltaSeconds * 3.f);
		SetActorScale3D(BaseScale * (1.f + Pulse * 0.25f));
	}
}

// ================================================================ AD1103_Barrel

AD1103_Barrel::AD1103_Barrel()
{
	InitShapeComponent(Body, EShape::Cylinder, FVector(90.f, 90.f, 130.f));
}

void AD1103_Barrel::BeginPlay()
{
	Super::BeginPlay();
	BaseScale = GetActorScale3D();
	SetColor(Body, Colors::Enemy);
}

void AD1103_Barrel::TakeHit_Implementation(float Damage, AActor* InstigatorActor)
{
	++HitCount;
	Pulse = 1.f;
	if (HitCount % 3 == 0)
	{
		// 폭발: 주변으로 불꽃 구체가 튀고 색이 검게
		for (int32 i = 0; i < 6; ++i)
		{
			const FVector Dir = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 1.2f).GetSafeNormal();
			if (AStaticMeshActor* Spark = SpawnShape(GetWorld(), EShape::Sphere, GetActorLocation() + Dir * 60.f, FVector(25.f), Colors::Accent, FRotator::ZeroRotator, true))
			{
				SetColor(Spark->GetStaticMeshComponent(), Colors::Accent, 0.5f, 0.f, 6.f);
				Spark->GetStaticMeshComponent()->AddImpulse(Dir * 700.f, NAME_None, true);
				Spark->SetLifeSpan(1.5f);
			}
		}
		LastReaction = TEXT("폭발! (3번째 피격)");
	}
	else
	{
		LastReaction = FString::Printf(TEXT("흔들림 (%d/3)"), HitCount % 3);
	}
}

// ================================================================ AD1103_Dummy

AD1103_Dummy::AD1103_Dummy()
{
	InitShapeComponent(Body, EShape::Cube, FVector(60.f, 60.f, 180.f));
}

void AD1103_Dummy::BeginPlay()
{
	Super::BeginPlay();
	BaseScale = GetActorScale3D();
	SetColor(Body, FLinearColor(0.85f, 0.75f, 0.5f));
}

void AD1103_Dummy::TakeHit_Implementation(float Damage, AActor* InstigatorActor)
{
	++HitCount;
	TotalDamage += Damage;
	Wobble = 1.f;
	LastReaction = FString::Printf(TEXT("누적 피해 %.0f"), TotalDamage);
}

void AD1103_Dummy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Wobble > 0.f)
	{
		Wobble = FMath::Max(0.f, Wobble - DeltaSeconds);
		const float Angle = FMath::Sin(Wobble * 25.f) * 20.f * Wobble;
		SetActorRotation(FRotator(Angle, 0.f, 0.f));
	}
}

// ================================================================ AD1103_Glass

AD1103_Glass::AD1103_Glass()
{
	InitShapeComponent(Body, EShape::Cube, FVector(20.f, 220.f, 200.f));
}

void AD1103_Glass::BeginPlay()
{
	Super::BeginPlay();
	BaseScale = GetActorScale3D();
	SetColor(Body, Colors::Cyan, 0.05f, 0.f, 0.3f);
}

void AD1103_Glass::TakeHit_Implementation(float Damage, AActor* InstigatorActor)
{
	if (IsHidden())
	{
		LastReaction = TEXT("이미 깨짐");
		return;
	}
	++HitCount;
	for (int32 i = 0; i < 10; ++i)
	{
		const FVector Offset(0.f, FMath::FRandRange(-100.f, 100.f), FMath::FRandRange(-90.f, 90.f));
		if (AStaticMeshActor* Shard = SpawnShape(GetWorld(), EShape::Cube, GetActorLocation() + Offset, FVector(8.f, 30.f, 30.f), Colors::Cyan, FRotator(0.f, 0.f, FMath::FRandRange(0.f, 90.f)), true))
		{
			Shard->GetStaticMeshComponent()->AddImpulse(FVector(FMath::FRandRange(100.f, 300.f), Offset.Y, 100.f), NAME_None, true);
			Shard->SetLifeSpan(3.f);
		}
	}
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	RepairTimer = 3.f;
	LastReaction = TEXT("와장창 (3초 뒤 복구)");
}

void AD1103_Glass::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (RepairTimer > 0.f)
	{
		RepairTimer -= DeltaSeconds;
		if (RepairTimer <= 0.f)
		{
			SetActorHiddenInGame(false);
			SetActorEnableCollision(true);
		}
	}
}

// ================================================================ ADemo1103

ADemo1103_Interface::ADemo1103_Interface()
{
	DemoId = TEXT("1103");
	Controls = TEXT("F : 플레이어 주변 13m 안의 모든 액터에 TakeHit 보내기 (WASD 로 이동해서 범위 바꾸기)\n")
		TEXT("관찰: 폭발통·허수아비·유리창은 서로 다른 클래스지만 같은 호출 하나로 각자 다르게 반응,\n")
		TEXT("인터페이스가 없는 회색 벽은 Implements 검사에서 걸러져 아무 일도 없다");
	Showcase = TEXT("Damageable 인터페이스로 폭발통·허수아비·유리창을 캐스트 없이 Execute_TakeHit 한 줄로");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-700.f, 100.f, 100.f));
}

void ADemo1103_Interface::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Targets.Add(World->SpawnActor<AD1103_Barrel>(FVector(300.f, -350.f, 65.f), FRotator::ZeroRotator, Params));
	Targets.Add(World->SpawnActor<AD1103_Dummy>(FVector(300.f, -50.f, 90.f), FRotator::ZeroRotator, Params));
	Targets.Add(World->SpawnActor<AD1103_Glass>(FVector(300.f, 250.f, 100.f), FRotator::ZeroRotator, Params));
	// 인터페이스가 없는 평범한 벽 (AStaticMeshActor)
	Targets.Add(SpawnBlock(World, FVector(300.f, 600.f, 0.f), FVector(60.f, 160.f, 220.f), Colors::Wall));

	SpawnLabel(World, TEXT("폭발통\nAD1103_Barrel"), FVector(300.f, -350.f, 200.f), Colors::Enemy, 15.f);
	SpawnLabel(World, TEXT("허수아비\nAD1103_Dummy"), FVector(300.f, -50.f, 260.f), FLinearColor(0.95f, 0.85f, 0.6f), 15.f);
	SpawnLabel(World, TEXT("유리창\nAD1103_Glass"), FVector(300.f, 250.f, 280.f), Colors::Cyan, 15.f);
	SpawnLabel(World, TEXT("벽 (인터페이스 없음)\nAStaticMeshActor"), FVector(300.f, 600.f, 300.f), Colors::Neutral, 15.f);

	Panel = UD11_SidePanel::Create(GetPC(), TEXT("Execute_TakeHit 결과"), FVector2D(1.f, 0.f), 13.f);
}

void ADemo1103_Interface::HitAround(const FVector& Center, float Radius)
{
	++WaveCount;
	RingTimer = 0.4f;
	RingCenter = Center;
	Log.Add(FString::Printf(TEXT("--- 충격파 #%d ---"), WaveCount));

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor == GetPlayerPawn() || FVector::Dist2D(Actor->GetActorLocation(), Center) > Radius || !Targets.Contains(Actor))
		{
			continue; // 바닥·라벨 등 장면 소품은 목록에서 생략(결과 로그를 짧게)
		}

		// ▼ 핵심 두 줄: 캐스트 없이 "이 인터페이스를 구현했나?" → 구현했으면 Execute_ 로 호출
		//   Implements<> 는 C++ 구현과 BP 구현 모두를 잡는다. (Cast<ID1103_Damageable> 는 BP 전용 구현을 못 잡는다)
		if (Actor->Implements<UD1103_Damageable>())
		{
			ID1103_Damageable::Execute_TakeHit(Actor, HitDamage, GetPlayerPawn());
			const AD1103_PropBase* Prop = Cast<AD1103_PropBase>(Actor); // 화면 표시용으로만 읽는다
			Log.Add(FString::Printf(TEXT("%s → TakeHit: %s"), *Actor->GetClass()->GetName(), Prop ? *Prop->LastReaction : TEXT("")));
		}
		else
		{
			Log.Add(FString::Printf(TEXT("%s → 인터페이스 없음, 건너뜀"), *Actor->GetClass()->GetName()));
		}
	}
}

void ADemo1103_Interface::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const APawn* Player = GetPlayerPawn();
	const FVector Center = Player ? Player->GetActorLocation() : FVector::ZeroVector;

	if (WasKeyPressed(EKeys::F))
	{
		HitAround(Center, HitRadius);
	}

	// 범위 표시: 평소엔 얇은 원, 쏜 직후엔 두꺼운 주황 원
	const FVector Ground(Center.X, Center.Y, 5.f);
	if (RingTimer > 0.f)
	{
		RingTimer -= DeltaSeconds;
		DrawDebugCircle(GetWorld(), FVector(RingCenter.X, RingCenter.Y, 5.f), HitRadius * (1.f - RingTimer), 64, FColor::Orange, false, -1.f, 0, 8.f, FVector::ForwardVector, FVector::RightVector, false);
	}
	DrawDebugCircle(GetWorld(), Ground, HitRadius, 64, FColor(255, 255, 255, 120), false, -1.f, 0, 2.f, FVector::ForwardVector, FVector::RightVector, false);

	if (Panel)
	{
		Panel->SetBody(FString(TEXT("호출 코드 (대상 클래스를 모른다):\n"))
			+ TEXT("  if (Actor->Implements<UD1103_Damageable>())\n")
			+ TEXT("      ID1103_Damageable::Execute_TakeHit(Actor, 10, Player);\n\n")
			+ Log.Join());
	}
	int32 InRange = 0;
	for (const AActor* T : Targets)
	{
		InRange += (T && FVector::Dist2D(T->GetActorLocation(), Center) <= HitRadius) ? 1 : 0;
	}
	SetStatus(FString::Printf(TEXT("충격파 %d회 · 반경 %.0fcm 안 대상 %d/4\n구현 클래스 3종 (Barrel·Dummy·Glass) + 미구현 벽 1개"), WaveCount, HitRadius, InRange));
}
