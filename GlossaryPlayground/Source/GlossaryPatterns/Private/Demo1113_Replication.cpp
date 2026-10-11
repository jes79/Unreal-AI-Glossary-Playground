#include "Demo1113_Replication.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

using namespace GlossaryKit;

// ================================================================ AD1113_NetDummy

AD1113_NetDummy::AD1113_NetDummy()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;            // 이 액터를 네트워크로 복제한다(서버가 스폰하면 클라이언트에도 생긴다)
	SetReplicatingMovement(false); // 움직이지 않으니 위치는 복제할 필요 없다

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	RootComponent = Body;
	InitShapeComponent(Body, EShape::Cylinder, FVector(260.f, 260.f, 200.f));
	Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));
}

void AD1113_NetDummy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// 이 줄이 있어야 Health 가 실제로 복제된다(UPROPERTY 지정자만으로는 부족)
	DOREPLIFETIME(AD1113_NetDummy, Health);
}

void AD1113_NetDummy::BeginPlay()
{
	Super::BeginPlay();
	SetColor(Body, Colors::Enemy);
}

void AD1113_NetDummy::ApplyDamageOnServer(float Amount)
{
	if (!HasAuthority())
	{
		return; // 클라이언트는 체력을 바꿀 수 없다 — 바꿔도 다음 복제 때 서버 값으로 덮인다
	}
	const float Old = Health;
	Health = FMath::Max(0.f, Health - Amount);
	if (Health <= 0.f)
	{
		RespawnTimer = 2.f;
	}
	// 서버는 OnRep 이 자동으로 안 불리므로 직접 호출 (리슨 서버 호스트 화면도 갱신되도록)
	bManualRepCall = true;
	OnRep_Health(Old);
	bManualRepCall = false;
}

void AD1113_NetDummy::OnRep_Health(float OldHealth)
{
	bManualRepCall ? ++OnRepManual : ++OnRepAuto;
	// 연출: 피격 플래시 + 체력 비율로 크기
	Flash = 1.f;
	const float Ratio = Health / 100.f;
	SetActorScale3D(FVector(2.6f, 2.6f, 0.4f + 1.6f * Ratio));
}

void AD1113_NetDummy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Flash > 0.f)
	{
		Flash = FMath::Max(0.f, Flash - DeltaSeconds * 3.f);
		SetColor(Body, FMath::Lerp(Colors::Enemy, FLinearColor::White, Flash), 0.5f, 0.f, Flash * 4.f);
	}
	if (HasAuthority() && RespawnTimer > 0.f)
	{
		RespawnTimer -= DeltaSeconds;
		if (RespawnTimer <= 0.f)
		{
			const float Old = Health;
			Health = 100.f;
			bManualRepCall = true;
			OnRep_Health(Old);
			bManualRepCall = false;
		}
	}
}

// ================================================================ AD1113_NetProjectile

AD1113_NetProjectile::AD1113_NetProjectile()
{
	bReplicates = true;
	SetReplicatingMovement(true); // 서버의 위치를 클라이언트에 보낸다

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(18.f);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Collision->SetNotifyRigidBodyCollision(true);
	RootComponent = Collision;

	UStaticMeshComponent* Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Collision);
	InitShapeComponent(Visual, EShape::Sphere, FVector(36.f));
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->InitialSpeed = 2200.f;
	Movement->MaxSpeed = 2200.f;
	Movement->ProjectileGravityScale = 0.f;
	InitialLifeSpan = 2.f;
}

void AD1113_NetProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (UStaticMeshComponent* Visual = FindComponentByClass<UStaticMeshComponent>())
	{
		SetColor(Visual, Colors::Item, 0.4f, 0.f, 4.f);
	}
	if (APawn* Shooter = GetInstigator())
	{
		Collision->IgnoreActorWhenMoving(Shooter, true);
	}
	Collision->OnComponentHit.AddDynamic(this, &AD1113_NetProjectile::HandleHit);
}

void AD1113_NetProjectile::HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 판정은 서버만. 클라이언트의 복제본은 보여 주기만 한다
	if (!HasAuthority())
	{
		return;
	}
	if (AD1113_NetDummy* Target = Cast<AD1113_NetDummy>(OtherActor))
	{
		Target->ApplyDamageOnServer(20.f);
	}
	Destroy(); // 서버에서 Destroy → 클라이언트 복제본도 사라진다
}

// ================================================================ AD1113_NetCharacter

AD1113_NetCharacter::AD1113_NetCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// ACharacter 는 기본으로 bReplicates = true, 이동도 CharacterMovement 가 예측·보정하며 복제한다
}

void AD1113_NetCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AD1113_NetCharacter, ShotsFired);
}

void AD1113_NetCharacter::RequestFire()
{
	++RequestCount;
	const FVector Dir = GetActorForwardVector();
	const FVector Origin = GetActorLocation() + Dir * 90.f + FVector(0.f, 0.f, 30.f);
	// 클라이언트: 네트워크로 서버에 요청 / 서버·싱글: 바로 _Implementation 실행
	ServerFire(Origin, Dir);
}

bool AD1113_NetCharacter::ServerFire_Validate(FVector_NetQuantize Origin, FVector_NetQuantizeNormal Direction)
{
	// 서버 검증: 총구가 캐릭터에서 너무 멀면(순간이동 치트 등) 거부 → false 면 그 클라이언트 연결이 끊긴다
	return FVector::Dist(Origin, GetActorLocation()) < 500.f;
}

void AD1113_NetCharacter::ServerFire_Implementation(FVector_NetQuantize Origin, FVector_NetQuantizeNormal Direction)
{
	// 여기는 항상 서버(권한 있는 쪽)에서 실행된다
	check(HasAuthority());
	++ServerFireExecuted;
	++ShotsFired; // Replicated → 클라이언트에 전달

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	GetWorld()->SpawnActor<AD1113_NetProjectile>(Origin, Direction.Rotation(), Params);

	MulticastMuzzleFlash();
}

void AD1113_NetCharacter::MulticastMuzzleFlash_Implementation()
{
	// 서버와 모든 클라이언트에서 실행되는 일회성 연출 (복제되지 않는 로컬 액터)
	++MuzzleFlashes;
	const FVector Muzzle = GetActorLocation() + GetActorForwardVector() * 90.f + FVector(0.f, 0.f, 30.f);
	if (AStaticMeshActor* Flash = SpawnShape(GetWorld(), EShape::Sphere, Muzzle, FVector(60.f), Colors::Accent))
	{
		SetColor(Flash->GetStaticMeshComponent(), Colors::Accent, 0.5f, 0.f, 10.f);
		Flash->SetActorEnableCollision(false);
		Flash->SetLifeSpan(0.08f);
	}
}

// ================================================================ ADemo1113

ADemo1113_Replication::ADemo1113_Replication()
{
	DemoId = TEXT("1113");
	Controls = TEXT("F : 발사 (RequestFire → ServerFire Server RPC → 서버가 총알 스폰·피해 → Health 복제 + OnRep_Health)\n")
		TEXT("L : 이 레벨을 ?listen 으로 다시 열어 진짜 Listen Server 로 전환\n")
		TEXT("2인 테스트: 에디터 Play ▾ > Number of Players 2, Net Mode 'Play As Listen Server' → 클라이언트 창에서 쏘면 OnRep 자동 호출이 는다");
	Showcase = TEXT("Replicated 체력 + OnRep_Health 표적과 Server RPC 발사, NetMode·Role·RPC/OnRep 호출 수 표시");
	PlayerPawnClass = AD1113_NetCharacter::StaticClass();
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-500.f, 0.f, 100.f));
}

void ADemo1113_Replication::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 복제 액터는 서버만 스폰한다. 클라이언트에는 복제로 생긴다(클라이언트가 따로 스폰하면 서로 다른 가짜가 된다)
	if (GetNetMode() != NM_Client)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Dummy = World->SpawnActor<AD1113_NetDummy>(FVector(500.f, 0.f, 100.f), FRotator::ZeroRotator, Params);
	}
	DummyLabel = SpawnLabel(World, TEXT(""), FVector(500.f, 0.f, 330.f), Colors::Enemy, 20.f);
	SpawnBlock(World, FVector(500.f, 0.f, 0.f), FVector(300.f, 300.f, 4.f), Colors::Platform);
	Panel = UD11_SidePanel::Create(GetPC(), TEXT("네트워크 상태 (이 프로세스)"), FVector2D(1.f, 0.f), 13.f);
}

FString ADemo1113_Replication::NetModeName(ENetMode Mode)
{
	switch (Mode)
	{
	case NM_Standalone:     return TEXT("Standalone (혼자 = 서버 권한)");
	case NM_ListenServer:   return TEXT("ListenServer (호스트 = 서버 + 플레이어)");
	case NM_DedicatedServer:return TEXT("DedicatedServer");
	case NM_Client:         return TEXT("Client");
	default:                return TEXT("?");
	}
}

FString ADemo1113_Replication::RoleName(ENetRole Role)
{
	return StaticEnum<ENetRole>()->GetNameStringByValue(Role);
}

void ADemo1113_Replication::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* World = GetWorld();

	if (!Dummy)
	{
		// 클라이언트: 서버가 복제해 준 표적을 찾는다
		for (TActorIterator<AD1113_NetDummy> It(World); It; ++It)
		{
			Dummy = *It;
			break;
		}
	}

	AD1113_NetCharacter* Player = Cast<AD1113_NetCharacter>(GetPlayerPawn());
	if (Player && WasKeyPressed(EKeys::F))
	{
		Player->RequestFire();
	}
	if (WasKeyPressed(EKeys::L) && GetNetMode() == NM_Standalone)
	{
		// 같은 맵을 listen 옵션으로 다시 연다 → 이 프로세스가 포트 7777 에서 접속을 받는 Listen Server 가 된다
		// (다른 실행 파일에서 'open 127.0.0.1' 로 접속 가능. Windows 방화벽 허용 창이 뜰 수 있다)
		UGameplayStatics::OpenLevel(this, FName(*World->GetOutermost()->GetName()), true, TEXT("listen"));
		return;
	}

	if (Dummy && DummyLabel)
	{
		DummyLabel->SetText(FString::Printf(TEXT("Health %.0f  (Replicated)"), Dummy->Health));
	}

	if (Panel)
	{
		FString Body = FString::Printf(TEXT("NetMode: %s\n\n"), *NetModeName(GetNetMode()));
		if (Player)
		{
			Body += FString::Printf(TEXT("플레이어  LocalRole %s · RemoteRole %s\n")
				TEXT("  F 입력 %d회 → ServerFire 실행 %d회 (이 프로세스)\n")
				TEXT("  ShotsFired (Replicated) %d · Multicast 섬광 %d회\n\n"),
				*RoleName(Player->GetLocalRole()), *RoleName(Player->GetRemoteRole()),
				Player->RequestCount, Player->ServerFireExecuted, Player->ShotsFired, Player->MuzzleFlashes);
		}
		if (Dummy)
		{
			Body += FString::Printf(TEXT("표적  LocalRole %s · Health %.0f\n")
				TEXT("  OnRep_Health 자동 %d회 (클라이언트에서만)\n  OnRep_Health 수동 %d회 (서버가 직접 호출)\n\n"),
				*RoleName(Dummy->GetLocalRole()), Dummy->Health, Dummy->OnRepAuto, Dummy->OnRepManual);
		}
		Body += GetNetMode() == NM_Standalone
			? TEXT("지금은 혼자 실행이라 이 프로세스가 서버다:\n  ServerFire 는 네트워크 없이 바로 실행되고\n  OnRep 은 수동 호출만 보인다. (L = Listen Server)\n")
			: TEXT("Listen Server: 다른 클라이언트가 접속하면\n  그 쪽에서 OnRep 자동 호출이 일어난다.\n");
		Body += TEXT("\n2인 PIE: Play ▾ > Multiplayer Options\n  Number of Players 2\n  Net Mode: Play As Listen Server\n  → 창 2개, 클라이언트 창에서 F");
		Panel->SetBody(Body);
	}
	SetStatus(FString::Printf(TEXT("%s · 표적 Health %.0f"), *NetModeName(GetNetMode()), Dummy ? Dummy->Health : -1.f));
}
