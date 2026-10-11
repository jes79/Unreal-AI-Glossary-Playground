#include "Demo1106_ObjectPooling.h"

#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GlossaryKit.h"
#include "HAL/PlatformTime.h"

using namespace GlossaryKit;

namespace
{
	// 화면 표시용 누적 카운터 (이 레벨에서만 쓴다)
	int64 GDestroyCalls = 0;
	const FLinearColor SpawnColor(1.f, 0.25f, 0.15f);
	const FLinearColor PoolColor(0.2f, 1.f, 0.35f);
}

// ================================================================ AD1106_Bullet

AD1106_Bullet::AD1106_Bullet()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	InitShapeComponent(Mesh, EShape::Sphere, FVector(22.f));
	Mesh->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Mesh->SetGenerateOverlapEvents(false);
}

void AD1106_Bullet::Launch(const FVector& Location, const FVector& InVelocity, float InLifetime)
{
	SetActorLocation(Location);
	Velocity = InVelocity;
	Lifetime = InLifetime;
}

void AD1106_Bullet::SetActive(bool bActive)
{
	// 요청 예문 그대로: SetActorHiddenInGame + 충돌 끄기. Tick 도 꺼서 쉬는 동안 비용 0.
	SetActorHiddenInGame(!bActive);
	SetActorEnableCollision(bActive);
	SetActorTickEnabled(bActive);
}

void AD1106_Bullet::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	SetActorLocation(GetActorLocation() + Velocity * DeltaSeconds);
	Lifetime -= DeltaSeconds;
	if (Lifetime <= 0.f)
	{
		if (OwnerPool)
		{
			OwnerPool->Release(this); // 재사용: 숨기고 풀로
		}
		else
		{
			++GDestroyCalls;
			Destroy();                // 일반 방식: 파괴 → 나중에 GC 가 메모리를 회수
		}
	}
}

// ================================================================ UD1106_BulletPool

UD1106_BulletPool::UD1106_BulletPool()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UD1106_BulletPool::BeginPlay()
{
	Super::BeginPlay();
	// 미리 만들기(Prewarm): 로딩 중에 한 번에 비용을 치르고, 플레이 중에는 만들지 않는다
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 i = 0; i < PoolSize; ++i)
	{
		AD1106_Bullet* Bullet = GetWorld()->SpawnActor<AD1106_Bullet>(FVector(0.f, 0.f, -1000.f), FRotator::ZeroRotator, Params);
		++SpawnCalls;
		Bullet->OwnerPool = this;
		SetColor(Bullet->Mesh, PoolColor, 0.4f, 0.f, 3.f);
		Bullet->SetActive(false);
		All.Add(Bullet);
		Free.Add(Bullet);
	}
}

AD1106_Bullet* UD1106_BulletPool::Acquire()
{
	if (Free.Num() == 0)
	{
		++EmptyCount; // 풀이 비었다 → 이 발은 쏘지 않는다(크기를 늘리거나 가장 오래된 것을 회수하는 전략도 있다)
		return nullptr;
	}
	AD1106_Bullet* Bullet = Free.Pop(EAllowShrinking::No);
	Bullet->SetActive(true);
	++ReuseCount;
	return Bullet;
}

void UD1106_BulletPool::Release(AD1106_Bullet* Bullet)
{
	Bullet->SetActive(false);
	Free.Add(Bullet);
}

// ================================================================ ADemo1106

ADemo1106_ObjectPooling::ADemo1106_ObjectPooling()
{
	DemoId = TEXT("1106");
	Controls = TEXT("1 : 왼쪽(Spawn/Destroy) 발사 켜기/끄기   2 : 오른쪽(풀 50개) 발사 켜기/끄기\n")
		TEXT("[ / ] : 초당 발사 수 -10 / +10\n")
		TEXT("관찰: 왼쪽은 SpawnActor·Destroy 누적이 계속 늘고, 오른쪽은 시작 때 50번 만든 뒤 0 — 재사용 횟수만 는다");
	Showcase = TEXT("Spawn/Destroy 포탑 vs 총알 풀 50개 포탑, 생성·파괴 호출 수와 스폰 비용 비교");
	PlayerPawnClass = nullptr; // 고정 카메라
}

void ADemo1106_ObjectPooling::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	GDestroyCalls = 0;

	// 풀은 컴포넌트로 데모 액터에 붙인다
	Pool = NewObject<UD1106_BulletPool>(this, TEXT("BulletPool"));
	Pool->PoolSize = 50;
	Pool->RegisterComponent();

	SpawnBlock(World, FVector(0.f, -450.f, 0.f), FVector(120.f, 120.f, 100.f), SpawnColor);
	SpawnBlock(World, FVector(0.f, 450.f, 0.f), FVector(120.f, 120.f, 100.f), PoolColor);
	SpawnLabel(World, TEXT("SpawnActor / Destroy"), FVector(0.f, -450.f, 230.f), SpawnColor, 20.f);
	SpawnLabel(World, TEXT("Object Pool (50)"), FVector(0.f, 450.f, 230.f), PoolColor, 20.f);

	// 고정 카메라: 두 포탑과 탄막이 다 보이게 비스듬히 위에서
	if (APlayerController* PC = GetPC())
	{
		ACameraActor* Cam = World->SpawnActor<ACameraActor>(FVector(-1300.f, 0.f, 900.f), FRotator(-30.f, 0.f, 0.f));
		PC->SetViewTarget(Cam);
	}
	Panel = UD11_SidePanel::Create(GetPC(), TEXT("호출 수 비교"), FVector2D(1.f, 1.f), 15.f);
}

void ADemo1106_ObjectPooling::FireSpawnDestroy(const FVector& Muzzle, const FVector& Velocity)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const uint64 Start = FPlatformTime::Cycles64();
	// 매 발마다 새 액터: 메모리 할당 + 컴포넌트 생성·등록 + BeginPlay
	AD1106_Bullet* Bullet = GetWorld()->SpawnActor<AD1106_Bullet>(Muzzle, FRotator::ZeroRotator, Params);
	SetColor(Bullet->Mesh, SpawnColor, 0.4f, 0.f, 3.f);
	Bullet->Launch(Muzzle, Velocity, BulletLifetime);
	SpawnSeconds += FPlatformTime::ToSeconds64(FPlatformTime::Cycles64() - Start);
	++SpawnCalls;
}

void ADemo1106_ObjectPooling::FirePooled(const FVector& Muzzle, const FVector& Velocity)
{
	const uint64 Start = FPlatformTime::Cycles64();
	// 풀에서 빌리기: 위치·속도만 다시 넣고 보이게 켠다
	if (AD1106_Bullet* Bullet = Pool->Acquire())
	{
		Bullet->Launch(Muzzle, Velocity, BulletLifetime);
		++PooledShots;
	}
	AcquireSeconds += FPlatformTime::ToSeconds64(FPlatformTime::Cycles64() - Start);
}

void ADemo1106_ObjectPooling::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (WasKeyPressed(EKeys::One)) { bLeftOn = !bLeftOn; }
	if (WasKeyPressed(EKeys::Two)) { bRightOn = !bRightOn; }
	if (WasKeyPressed(EKeys::LeftBracket)) { FireRate = FMath::Max(10.f, FireRate - 10.f); }
	if (WasKeyPressed(EKeys::RightBracket)) { FireRate = FMath::Min(200.f, FireRate + 10.f); }

	// 같은 시각에 같은 방향으로 두 포탑이 쏜다(부채꼴로 흔들기)
	SweepTime += DeltaSeconds;
	Accumulator += DeltaSeconds * FireRate;
	while (Accumulator >= 1.f)
	{
		Accumulator -= 1.f;
		const float Yaw = FMath::Sin(SweepTime * 2.f) * 25.f;
		const FVector Velocity = FRotator(8.f, Yaw, 0.f).Vector() * BulletSpeed;
		if (bLeftOn)  { FireSpawnDestroy(FVector(80.f, -450.f, 120.f), Velocity); }
		if (bRightOn) { FirePooled(FVector(80.f, 450.f, 120.f), Velocity); }
	}

	int32 LiveSpawned = 0;
	for (TActorIterator<AD1106_Bullet> It(GetWorld()); It; ++It)
	{
		LiveSpawned += It->OwnerPool ? 0 : 1;
	}

	if (Panel)
	{
		const double SpawnUs = SpawnCalls ? SpawnSeconds / SpawnCalls * 1e6 : 0.0;
		const double PoolUs = PooledShots ? AcquireSeconds / PooledShots * 1e6 : 0.0;
		Panel->SetBody(FString::Printf(
			TEXT("초당 %.0f발 · 수명 %.1f초\n\n")
			TEXT("[왼쪽] SpawnActor / Destroy  %s\n")
			TEXT("  SpawnActor 누적  %lld\n  Destroy 누적     %lld\n  지금 살아 있는 총알 %d\n  1발 스폰 비용  %.1f µs\n\n")
			TEXT("[오른쪽] 풀 %d개  %s\n")
			TEXT("  SpawnActor 누적  %d (시작 때만)\n  Destroy 누적     0\n  재사용 %d회 · 사용 중 %d/%d · 풀 부족 %d회\n  1발 꺼내기 비용  %.1f µs"),
			FireRate, BulletLifetime,
			bLeftOn ? TEXT("ON") : TEXT("OFF"), SpawnCalls, GDestroyCalls, LiveSpawned, SpawnUs,
			Pool->NumTotal(), bRightOn ? TEXT("ON") : TEXT("OFF"),
			Pool->SpawnCalls, Pool->ReuseCount, Pool->NumActive(), Pool->NumTotal(), Pool->EmptyCount, PoolUs));
	}
	SetStatus(FString::Printf(TEXT("Spawn/Destroy: 생성 %lld · 파괴 %lld   |   풀: 생성 %d · 재사용 %d\n파괴된 액터는 GC 가 나중에 치운다(가비지 누적) — 풀은 가비지를 만들지 않는다"),
		SpawnCalls, GDestroyCalls, Pool->SpawnCalls, Pool->ReuseCount));
}
