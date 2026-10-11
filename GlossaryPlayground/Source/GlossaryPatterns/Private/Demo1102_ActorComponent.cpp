#include "Demo1102_ActorComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"

using namespace GlossaryKit;

// ================================================================ UD1102_HealthComponent

UD1102_HealthComponent::UD1102_HealthComponent()
{
	// 체력은 이벤트(피해·회복) 때만 바뀌므로 Tick 이 필요 없다
	PrimaryComponentTick.bCanEverTick = false;
}

void UD1102_HealthComponent::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
}

void UD1102_HealthComponent::ApplyDamage(float Amount)
{
	if (IsDead() || Amount <= 0.f)
	{
		return;
	}
	const float Old = Health;
	Health = FMath::Clamp(Health - Amount * DamageMultiplier, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(this, Health, Health - Old);
	if (Health <= 0.f)
	{
		// 주인이 무엇을 할지는 컴포넌트가 모른다 — 알리기만 한다
		OnDepleted.Broadcast(this);
	}
}

void UD1102_HealthComponent::Heal(float Amount)
{
	const float Old = Health;
	Health = FMath::Clamp(Health + Amount, 0.f, MaxHealth);
	if (Health != Old)
	{
		OnHealthChanged.Broadcast(this, Health, Health - Old);
	}
}

// ================================================================ AD1102_Enemy

AD1102_Enemy::AD1102_Enemy()
{
	PrimaryActorTick.bCanEverTick = true;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	RootComponent = Body;
	InitShapeComponent(Body, EShape::Cylinder, FVector(100.f, 100.f, 180.f));

	// 방법 1: 생성자에서 기본 서브오브젝트로 붙인다 → BP 자식·레벨 인스턴스 모두 디테일 패널에서 값 조절 가능
	HealthComponent = CreateDefaultSubobject<UD1102_HealthComponent>(TEXT("HealthComponent"));
	HealthComponent->MaxHealth = 60.f;
}

void AD1102_Enemy::BeginPlay()
{
	Super::BeginPlay();
	SetColor(Body, Colors::Enemy);
	HealthComponent->OnDepleted.AddDynamic(this, &AD1102_Enemy::HandleDepleted);
}

void AD1102_Enemy::HandleDepleted(UD1102_HealthComponent* Health)
{
	SetActorRotation(FRotator(0.f, 0.f, 80.f)); // 옆으로 쓰러짐
	SetColor(Body, Colors::Dark);
	ReviveTimer = 3.f;
}

void AD1102_Enemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (ReviveTimer > 0.f)
	{
		ReviveTimer -= DeltaSeconds;
		if (ReviveTimer <= 0.f)
		{
			SetActorRotation(FRotator::ZeroRotator);
			SetColor(Body, Colors::Enemy);
			HealthComponent->Heal(HealthComponent->MaxHealth);
		}
	}
}

// ================================================================ ADemo1102

ADemo1102_ActorComponent::ADemo1102_ActorComponent()
{
	DemoId = TEXT("1102");
	Controls = TEXT("1 : 플레이어 피격   2 : 적 피격   3 : 상자 피격   4 : 모두 회복\n")
		TEXT("관찰: 세 액터 모두 같은 UD1102_HealthComponent 를 쓴다. 최대 체력·피해 배율만 다르고,\n")
		TEXT("체력이 0 이 됐을 때의 반응(리스폰·쓰러짐·부서짐)은 주인이 이벤트에 연결한다");
	Showcase = TEXT("HealthComponent 하나를 플레이어·적·상자에 붙여 재사용, 주인별 반응은 이벤트로");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-500.f, 0.f, 100.f));
}

void ADemo1102_ActorComponent::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// --- 적: 생성자에서 컴포넌트를 갖고 태어난다
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Enemy = World->SpawnActor<AD1102_Enemy>(FVector(300.f, -350.f, 90.f), FRotator::ZeroRotator, Params);

	// --- 상자: 평범한 StaticMeshActor 에 실행 중 컴포넌트를 추가한다 (방법 2: NewObject + RegisterComponent)
	Crate = SpawnShape(World, EShape::Cube, FVector(300.f, 350.f, 60.f), FVector(120.f), FLinearColor(0.55f, 0.35f, 0.15f));
	UD1102_HealthComponent* CrateHealth = NewObject<UD1102_HealthComponent>(Crate, TEXT("HealthComponent"));
	CrateHealth->MaxHealth = 30.f;
	CrateHealth->DamageMultiplier = 0.5f; // 단단한 상자: 피해 절반
	CrateHealth->RegisterComponent();     // 등록해야 BeginPlay·Tick 등 생명주기가 돈다
	Crate->AddInstanceComponent(CrateHealth); // 디테일 패널·저장 대상 목록에 포함
	CrateHealth->OnDepleted.AddDynamic(this, &ADemo1102_ActorComponent::HandleCrateDepleted);

	// --- 플레이어: 이미 있는 캐릭터(GlossaryCore 클래스)에도 코드 수정 없이 붙일 수 있다
	if (ACharacter* Player = GetPlayerCharacter())
	{
		UD1102_HealthComponent* PlayerHealth = NewObject<UD1102_HealthComponent>(Player, TEXT("HealthComponent"));
		PlayerHealth->MaxHealth = 100.f;
		PlayerHealth->RegisterComponent();
		Player->AddInstanceComponent(PlayerHealth);
		PlayerHealth->OnDepleted.AddDynamic(this, &ADemo1102_ActorComponent::HandlePlayerDepleted);
		Owners.Add(Player);
	}
	Owners.Add(Enemy);
	Owners.Add(Crate);

	for (AActor* OwnerActor : Owners)
	{
		if (UD1102_HealthComponent* Health = OwnerActor->FindComponentByClass<UD1102_HealthComponent>())
		{
			Health->OnHealthChanged.AddDynamic(this, &ADemo1102_ActorComponent::HandleAnyHealthChanged);
		}
		Labels.Add(SpawnLabel(World, TEXT(""), OwnerActor->GetActorLocation() + FVector(0.f, 0.f, 160.f), FLinearColor::White, 18.f));
	}

	SpawnLabel(World, TEXT("적 (생성자에서 CreateDefaultSubobject)"), FVector(300.f, -350.f, 330.f), Colors::Enemy, 15.f);
	SpawnLabel(World, TEXT("상자 (실행 중 NewObject)"), FVector(300.f, 350.f, 290.f), Colors::Item, 15.f);

	Panel = UD11_SidePanel::Create(GetPC(), TEXT("UD1102_HealthComponent 인스턴스"), FVector2D(1.f, 0.f), 13.f);
	UpdateLabels();
}

void ADemo1102_ActorComponent::HitActor(AActor* Target)
{
	if (!Target)
	{
		return;
	}
	// 핵심: 대상이 플레이어인지 적인지 상자인지 캐스트하지 않는다. "체력 부품이 있으면" 피해를 준다.
	if (UD1102_HealthComponent* Health = Target->FindComponentByClass<UD1102_HealthComponent>())
	{
		Health->ApplyDamage(HitDamage);
	}
}

void ADemo1102_ActorComponent::HandleAnyHealthChanged(UD1102_HealthComponent* Health, float NewHealth, float Delta)
{
	Log.Add(FString::Printf(TEXT("%s: %+.0f → HP %.0f/%.0f"), *Health->GetOwner()->GetClass()->GetName(), Delta, NewHealth, Health->MaxHealth));
	UpdateLabels();
}

void ADemo1102_ActorComponent::HandlePlayerDepleted(UD1102_HealthComponent* Health)
{
	// 플레이어의 반응: 시작 위치로 돌아가 다시 시작
	Log.Add(TEXT("플레이어 반응: 리스폰 (시작 위치 + 체력 회복)"));
	if (APawn* Player = GetPlayerPawn())
	{
		Player->SetActorLocation(PlayerStartTransform.GetLocation());
	}
	Health->Heal(Health->MaxHealth);
}

void ADemo1102_ActorComponent::HandleCrateDepleted(UD1102_HealthComponent* Health)
{
	// 상자의 반응: 부서져 파편이 튄다
	Log.Add(TEXT("상자 반응: 부서짐 → 4초 뒤 다시 생김"));
	const FVector Center = Crate->GetActorLocation();
	for (int32 i = 0; i < 8; ++i)
	{
		const FVector Offset(FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-20.f, 40.f));
		if (AStaticMeshActor* Piece = SpawnShape(GetWorld(), EShape::Cube, Center + Offset, FVector(35.f), FLinearColor(0.55f, 0.35f, 0.15f), FRotator::ZeroRotator, true))
		{
			Piece->GetStaticMeshComponent()->AddImpulse(Offset.GetSafeNormal() * 300.f + FVector(0.f, 0.f, 300.f), NAME_None, true);
			Piece->SetLifeSpan(3.5f);
		}
	}
	Crate->SetActorHiddenInGame(true);
	Crate->SetActorEnableCollision(false);
	CrateRespawnTimer = 4.f;
}

void ADemo1102_ActorComponent::UpdateLabels()
{
	for (int32 i = 0; i < Owners.Num() && i < Labels.Num(); ++i)
	{
		const UD1102_HealthComponent* Health = Owners[i] ? Owners[i]->FindComponentByClass<UD1102_HealthComponent>() : nullptr;
		if (Health && Labels[i])
		{
			const float Ratio = Health->GetHealth() / Health->MaxHealth;
			Labels[i]->SetText(FString::Printf(TEXT("HP %.0f / %.0f"), Health->GetHealth(), Health->MaxHealth));
			Labels[i]->SetColor(FLinearColor::LerpUsingHSV(FLinearColor(1.f, 0.15f, 0.1f), FLinearColor(0.2f, 1.f, 0.3f), Ratio));
		}
	}
}

void ADemo1102_ActorComponent::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (WasKeyPressed(EKeys::One)) { HitActor(GetPlayerPawn()); }
	if (WasKeyPressed(EKeys::Two)) { HitActor(Enemy); }
	if (WasKeyPressed(EKeys::Three) && !Crate->IsHidden()) { HitActor(Crate); }
	if (WasKeyPressed(EKeys::Four))
	{
		for (AActor* OwnerActor : Owners)
		{
			if (UD1102_HealthComponent* Health = OwnerActor->FindComponentByClass<UD1102_HealthComponent>())
			{
				Health->Heal(1000.f);
			}
		}
	}

	if (CrateRespawnTimer > 0.f)
	{
		CrateRespawnTimer -= DeltaSeconds;
		if (CrateRespawnTimer <= 0.f)
		{
			Crate->SetActorHiddenInGame(false);
			Crate->SetActorEnableCollision(true);
			Crate->FindComponentByClass<UD1102_HealthComponent>()->Heal(1000.f);
		}
	}

	// 라벨은 주인을 따라다닌다(플레이어가 움직이므로)
	for (int32 i = 0; i < Owners.Num() && i < Labels.Num(); ++i)
	{
		if (Owners[i] && Labels[i])
		{
			Labels[i]->SetActorLocation(Owners[i]->GetActorLocation() + FVector(0.f, 0.f, 150.f));
		}
	}

	if (Panel)
	{
		FString Body = TEXT("같은 클래스, 다른 주인 · 다른 설정:\n");
		for (AActor* OwnerActor : Owners)
		{
			if (const UD1102_HealthComponent* Health = OwnerActor->FindComponentByClass<UD1102_HealthComponent>())
			{
				Body += FString::Printf(TEXT("  %-22s Max %3.0f · 배율 %.1f · HP %3.0f\n"),
					*OwnerActor->GetClass()->GetName(), Health->MaxHealth, Health->DamageMultiplier, Health->GetHealth());
			}
		}
		Body += TEXT("\n데미지 코드 (주인 종류를 모름):\n  Target->FindComponentByClass<UD1102_HealthComponent>()\n        ->ApplyDamage(25);\n\n[이벤트 로그]\n");
		Body += Log.Join();
		Panel->SetBody(Body);
	}
	SetStatus(FString::Printf(TEXT("HealthComponent 3개 (플레이어·적·상자) · 피해 %.0f\n상자는 DamageMultiplier 0.5 라 같은 피해에 절반만 깎인다"), HitDamage));
}
