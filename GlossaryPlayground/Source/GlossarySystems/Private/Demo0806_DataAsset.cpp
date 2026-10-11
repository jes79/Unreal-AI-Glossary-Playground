#include "Demo0806_DataAsset.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "D08_Common.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0806_DataAsset::ADemo0806_DataAsset()
{
	DemoId = TEXT("0806");
	Controls = TEXT("1 권총 · 2 소총 · 3 샷건 (각각 다른 Data Asset) · F : 자동 사격 ON/OFF\n")
		TEXT("관찰: 발사 코드는 하나인데 에셋의 데미지·연사속도·메시에 따라 손의 무기와 탄·초당 데미지가 달라진다");
	Showcase = TEXT("무기 Primary Data Asset 3개(권총·소총·샷건)를 바꿔 끼우면 메시·데미지·연사속도가 바뀌어 표적에 사격");
	const TCHAR* Names[] = { TEXT("DA_0806_Pistol"), TEXT("DA_0806_Rifle"), TEXT("DA_0806_Shotgun") };
	for (const TCHAR* Name : Names)
	{
		Weapons.Add(TSoftObjectPtr<UD0806_WeaponData>(FSoftObjectPath(FString::Printf(TEXT("/Game/Glossary/08_Systems/%s.%s"), Name, Name))));
	}
}

void ADemo0806_DataAsset::BeginPlay()
{
	Super::BeginPlay();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Target = GetWorld()->SpawnActor<AD08_Dummy>(FVector(200.f, 0.f, 90.f), FRotator::ZeroRotator, Params);
	Target->RespawnDelay = 1.f;
	Target->Configure(TEXT("표적"), 500.f, Colors::Enemy);

	// 손에 쥘 무기 메시 컴포넌트 (메시는 Data Asset 에서 채운다)
	if (ACharacter* Character = GetPlayerCharacter())
	{
		WeaponMesh = NewObject<UStaticMeshComponent>(Character, TEXT("WeaponMesh"));
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponMesh->SetupAttachment(Character->GetMesh(), TEXT("hand_r"));
		WeaponMesh->RegisterComponent();
	}
	Panel = UD08_SidePanel::Create(GetPC(), TEXT("무기"), UD08_SidePanel::EPlace::TopRight, 440.f);
	Equip(1);
}

void ADemo0806_DataAsset::Equip(int32 Index)
{
	if (!Weapons.IsValidIndex(Index))
	{
		return;
	}
	Current = Weapons[Index].LoadSynchronous();
	if (!Current)
	{
		SetStatus(TEXT("DA_0806_* 에셋이 없다 — tools/glossary-build.sh -filter=08"));
		return;
	}
	ShotsFired = 0;
	EquipTime = GetWorld()->GetTimeSeconds();
	Cooldown = 0.f;
	if (WeaponMesh)
	{
		// 소프트 참조라서 필요할 때 LoadSynchronous (큰 프로젝트는 비동기 로드)
		WeaponMesh->SetStaticMesh(Current->Mesh.LoadSynchronous());
		WeaponMesh->SetRelativeScale3D(Current->MeshSize / 100.f);
		WeaponMesh->SetRelativeLocation(FVector(0.f, Current->MeshSize.X * 0.3f, 0.f));
		WeaponMesh->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
		SetColor(WeaponMesh, Current->Color, 0.3f, 0.7f, 0.5f);
	}
}

void ADemo0806_DataAsset::Fire()
{
	APawn* Pawn = GetPlayerPawn();
	if (!Pawn || !Current || !Target)
	{
		return;
	}
	++ShotsFired;
	const FVector Muzzle = WeaponMesh ? WeaponMesh->GetComponentLocation() : Pawn->GetActorLocation();
	const float Damage = Current->Damage;
	const FLinearColor Color = Current->Color;
	TWeakObjectPtr<AD08_Dummy> WeakTarget = Target;
	// 탄 크기도 데미지에 비례 → 샷건은 크고 느린 탄, 소총은 작고 빠른 탄
	AD08_Projectile::Launch(GetWorld(), Muzzle, Target, Color, 8.f + Damage * 0.6f, 2500.f,
		[WeakTarget, Damage, Color](AActor*)
		{
			if (AD08_Dummy* T = WeakTarget.Get())
			{
				T->ApplyHit(Damage, Color);
			}
		});
}

void ADemo0806_DataAsset::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One))   { Equip(0); }
	if (WasKeyPressed(EKeys::Two))   { Equip(1); }
	if (WasKeyPressed(EKeys::Three)) { Equip(2); }
	if (WasKeyPressed(EKeys::F))     { bAutoFire = !bAutoFire; }

	// 플레이어가 항상 표적을 바라보게 (사격 방향이 잘 보이도록)
	if (APawn* Pawn = GetPlayerPawn())
	{
		if (Target)
		{
			FRotator Face = (Target->GetActorLocation() - Pawn->GetActorLocation()).Rotation();
			Face.Pitch = 0.f;
			Pawn->SetActorRotation(Face);
		}
	}

	if (!Current)
	{
		return;
	}
	// 같은 발사 코드: 쿨다운 = 1 / FireRate
	Cooldown -= DeltaSeconds;
	if ((bAutoFire || IsKeyDown(EKeys::LeftMouseButton)) && Cooldown <= 0.f)
	{
		Fire();
		Cooldown = 1.f / FMath::Max(0.01f, Current->FireRate);
	}

	const float Elapsed = FMath::Min(3.f, GetWorld()->GetTimeSeconds() - EquipTime);
	const float MeasuredDps = Elapsed > 0.5f && Target ? Target->GetRecentDamage(Elapsed) / Elapsed : 0.f;
	if (Panel)
	{
		Panel->SetTitle(FString::Printf(TEXT("%s  (%s)"), *Current->WeaponName, *Current->GetName()));
		Panel->SetBody(FString::Printf(
			TEXT("데미지 %.0f · 연사속도 %.1f발/초\n이론 DPS %.0f · 측정 DPS %.0f\n메시 %s\nPrimaryAssetId %s"),
			Current->Damage, Current->FireRate, Current->Damage * Current->FireRate, MeasuredDps,
			*Current->Mesh.GetAssetName(), *Current->GetPrimaryAssetId().ToString()));
	}
	SetStatus(FString::Printf(TEXT("장착: %s · 발사 %d발 · 자동 사격 %s"), *Current->WeaponName, ShotsFired, bAutoFire ? TEXT("ON") : TEXT("OFF")));
}
