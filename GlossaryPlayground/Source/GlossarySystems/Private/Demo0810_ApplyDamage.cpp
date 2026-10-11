#include "Demo0810_ApplyDamage.h"

#include "Engine/DamageEvents.h"
#include "GameFramework/Controller.h"
#include "GlossaryKit.h"
#include "Kismet/GameplayStatics.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- 적

float AD0810_Enemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 부모 호출: OnTakeAnyDamage 델리게이트 방송 등 엔진 기본 처리
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// Damage Type 클래스의 기본 객체(CDO)에서 배율을 읽는다
	const UD0810_DamageType_Normal* Type = DamageEvent.DamageTypeClass
		? Cast<UD0810_DamageType_Normal>(DamageEvent.DamageTypeClass->GetDefaultObject())
		: nullptr;
	const float Multiplier = Type ? Type->DamageMultiplier : 1.f;
	const float Final = DamageAmount * Multiplier;

	const FString TypeName = Type ? Type->DisplayName : TEXT("UDamageType");
	ApplyHit(Final, Type ? Type->Color : FLinearColor::White, FString::Printf(TEXT("%s ×%.1f"), *TypeName, Multiplier));
	LastHit = FString::Printf(TEXT("%s %.0f × %.1f = %.0f"), *TypeName, DamageAmount, Multiplier, Final);
	return Final; // 실제로 받은 양을 돌려준다
}

// ---------------------------------------------------------------- 데모

ADemo0810_ApplyDamage::ADemo0810_ApplyDamage()
{
	DemoId = TEXT("0810");
	Controls = TEXT("1 : 일반 · 2 : 화염 · 3 : 얼음 — 모두 ApplyDamage(적, 10, …, DamageType)\n")
		TEXT("관찰: 같은 10 을 보내도 적의 TakeDamage 가 Damage Type 배율로 10 / 20 / 5 를 깎는다");
	Showcase = TEXT("ApplyDamage(10)을 일반·화염(×2)·얼음(×0.5) Damage Type 으로 보내면 TakeDamage 가 10/20/5 로 처리");
}

void ADemo0810_ApplyDamage::BeginPlay()
{
	Super::BeginPlay();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Enemy = GetWorld()->SpawnActor<AD0810_Enemy>(FVector(300.f, 0.f, 90.f), FRotator::ZeroRotator, Params);
	Enemy->Configure(TEXT("적"), 200.f, Colors::Enemy);

	// 키 안내 표지
	const FLinearColor Cols[] = { FLinearColor::White, FLinearColor(1.f, 0.4f, 0.05f), FLinearColor(0.3f, 0.8f, 1.f) };
	const TCHAR* Texts[] = { TEXT("1 일반 ×1"), TEXT("2 화염 ×2"), TEXT("3 얼음 ×0.5") };
	for (int32 i = 0; i < 3; ++i)
	{
		SpawnLabel(GetWorld(), Texts[i], FVector(-200.f, -300.f + i * 300.f, 220.f), Cols[i], 22.f);
	}
	Panel = UD08_SidePanel::Create(GetPC(), TEXT("TakeDamage 기록"), UD08_SidePanel::EPlace::TopRight, 440.f);
}

void ADemo0810_ApplyDamage::Shoot(TSubclassOf<UDamageType> DamageType)
{
	APawn* Pawn = GetPlayerPawn();
	if (!Pawn || !Enemy)
	{
		return;
	}
	const UD0810_DamageType_Normal* Type = Cast<UD0810_DamageType_Normal>(DamageType->GetDefaultObject());
	TWeakObjectPtr<AD0810_Enemy> WeakEnemy = Enemy;
	TWeakObjectPtr<APawn> WeakPawn = Pawn;
	const float Damage = BaseDamage;
	TWeakObjectPtr<ADemo0810_ApplyDamage> WeakThis = this;
	AD08_Projectile::Launch(GetWorld(), Pawn->GetActorLocation() + FVector(0, 0, 40), Enemy, Type ? Type->Color : FLinearColor::White, 30.f, 1800.f,
		[WeakEnemy, WeakPawn, WeakThis, Damage, DamageType](AActor*)
		{
			AD0810_Enemy* E = WeakEnemy.Get();
			if (!E)
			{
				return;
			}
			// 핵심: 데미지를 '보내는' 쪽은 이 한 줄. 얼마나 깎일지는 받는 쪽(TakeDamage)이 정한다
			const float Applied = UGameplayStatics::ApplyDamage(E, Damage,
				WeakPawn.IsValid() ? WeakPawn->GetController() : nullptr, WeakPawn.Get(), DamageType);
			if (ADemo0810_ApplyDamage* Self = WeakThis.Get())
			{
				Self->Log.Add(FString::Printf(TEXT("ApplyDamage(%.0f) → TakeDamage: %s (반환 %.0f)"), Damage, *E->LastHit, Applied));
				if (Self->Log.Num() > 7)
				{
					Self->Log.RemoveAt(0);
				}
			}
		});
}

void ADemo0810_ApplyDamage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One))   { Shoot(UD0810_DamageType_Normal::StaticClass()); }
	if (WasKeyPressed(EKeys::Two))   { Shoot(UD0810_DamageType_Fire::StaticClass()); }
	if (WasKeyPressed(EKeys::Three)) { Shoot(UD0810_DamageType_Ice::StaticClass()); }

	if (Panel)
	{
		FString Body;
		for (const FString& Line : Log)
		{
			Body += Line + TEXT("\n");
		}
		Panel->SetTitle(FString::Printf(TEXT("적 HP %.0f / %.0f"), Enemy ? Enemy->Health : 0.f, Enemy ? Enemy->MaxHealth : 0.f));
		Panel->SetProgress(Enemy ? Enemy->Health / Enemy->MaxHealth : 0.f, Colors::Enemy);
		Panel->SetBody(Body.IsEmpty() ? TEXT("1/2/3 을 눌러 데미지를 보낸다") : Body.TrimEnd());
	}
	SetStatus(Enemy && !Enemy->LastHit.IsEmpty() ? FString::Printf(TEXT("마지막 피격: %s"), *Enemy->LastHit) : TEXT("1/2/3 으로 같은 10 데미지를 종류만 바꿔 보낸다"));
}
