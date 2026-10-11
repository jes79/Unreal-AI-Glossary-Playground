#include "Demo0808_GASAbility.h"

#include "D08_Common.h"
#include "D08_GAS.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0808_GASAbility::ADemo0808_GASAbility()
{
	DemoId = TEXT("0808");
	// 플레이어 컨트롤러(입력 처리) 다음에 Tick 해서 WasKeyPressed 를 놓치지 않게
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	Controls = TEXT("1 또는 왼쪽 클릭 : 파이어볼 (마나 20 · 쿨타임 2초) · R : 마나 재생 ON/OFF\n")
		TEXT("관찰: 쿨타임 중이거나 마나가 20 미만이면 TryActivateAbility 가 실패한다 — 검사 코드는 직접 쓰지 않았다");
	Showcase = TEXT("GAS 파이어볼: 마나 20 Cost GE, 2초 Cooldown GE(태그), 명중 시 데미지 GE — 실패 이유 표시");
	PlayerPawnClass = AD08_GASCharacter::StaticClass();
}

void ADemo0808_GASAbility::BeginPlay()
{
	Super::BeginPlay();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Target = GetWorld()->SpawnActor<AD08_GASDummy>(FVector(300.f, 350.f, 90.f), FRotator::ZeroRotator, Params);
	Target->SetDisplayName(TEXT("허수아비"));

	if (AD08_GASCharacter* Player = Cast<AD08_GASCharacter>(GetPlayerPawn()))
	{
		UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
		// 어빌리티를 '부여'해야 발동할 수 있다 (서버 권한)
		ASC->GiveAbility(FGameplayAbilitySpec(UD0808_GA_Fireball::StaticClass(), 1));
		if (bManaRegen)
		{
			D08GAS::ApplyEffect(ASC, ASC, UD08_GE_ManaRegen::StaticClass());
		}
	}
	Panel = UD08_SidePanel::Create(GetPC(), TEXT("플레이어 ASC"), UD08_SidePanel::EPlace::TopRight, 440.f);
	LastResult = TEXT("1 을 눌러 파이어볼");
}

void ADemo0808_GASAbility::TryCast()
{
	AD08_GASCharacter* Player = Cast<AD08_GASCharacter>(GetPlayerPawn());
	if (!Player)
	{
		return;
	}
	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	// 표적을 바라보고 발사
	if (Target)
	{
		FRotator Face = (Target->GetActorLocation() - Player->GetActorLocation()).Rotation();
		Face.Pitch = 0.f;
		Player->SetActorRotation(Face);
	}
	// 실패 이유 판별용 (발동 전 상태)
	const bool bOnCooldown = ASC->HasMatchingGameplayTag(D08GAS::CooldownFireballTag());
	const float ManaBefore = Player->GetAttributes()->GetMana();

	// 핵심 한 줄: 비용·쿨타임·태그 조건은 GAS 가 검사한다
	const bool bActivated = ASC->TryActivateAbilityByClass(UD0808_GA_Fireball::StaticClass());
	if (bActivated)
	{
		++CastCount;
		LastResult = FString::Printf(TEXT("발동 성공 → 마나 %.0f → %.0f, 쿨타임 2초 시작"), ManaBefore, Player->GetAttributes()->GetMana());
	}
	else
	{
		++FailCount;
		LastResult = bOnCooldown ? TEXT("실패: 쿨타임 중 (Ability.Cooldown.Fireball 태그가 있음)")
			: (ManaBefore < 20.f ? FString::Printf(TEXT("실패: 마나 부족 (%.0f < 20)"), ManaBefore) : FString(TEXT("실패")));
		AD08_FloatingText::Spawn(GetWorld(), bOnCooldown ? TEXT("쿨타임!") : TEXT("마나 부족!"), Player->GetActorLocation() + FVector(0, 0, 130), Colors::Neutral, 24.f);
	}
}

void ADemo0808_GASAbility::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AD08_GASCharacter* Player = Cast<AD08_GASCharacter>(GetPlayerPawn());
	if (!Player)
	{
		SetStatus(TEXT("플레이어가 AD08_GASCharacter 가 아니다"));
		return;
	}
	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	if (WasKeyPressed(EKeys::One) || WasKeyPressed(EKeys::LeftMouseButton))
	{
		TryCast();
	}
	if (WasKeyPressed(EKeys::R))
	{
		bManaRegen = !bManaRegen;
		if (bManaRegen)
		{
			D08GAS::ApplyEffect(ASC, ASC, UD08_GE_ManaRegen::StaticClass());
		}
		else
		{
			ASC->RemoveActiveGameplayEffectBySourceEffect(UD08_GE_ManaRegen::StaticClass(), ASC);
		}
	}

	// 남은 쿨타임 = 쿨타임 태그를 가진 활성 GE 의 남은 시간
	const FGameplayTagContainer CooldownTags(D08GAS::CooldownFireballTag());
	const TArray<float> Remaining = ASC->GetActiveEffectsTimeRemaining(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTags));
	const float CooldownLeft = Remaining.Num() > 0 ? FMath::Max(Remaining[0], 0.f) : 0.f;
	const UD08_AttributeSet* Attr = Player->GetAttributes();

	if (Panel)
	{
		Panel->SetTitle(FString::Printf(TEXT("마나 %.0f / %.0f"), Attr->GetMana(), Attr->GetMaxMana()));
		Panel->SetProgress(Attr->GetMana() / Attr->GetMaxMana(), FLinearColor(0.15f, 0.35f, 1.f));
		Panel->SetBody(FString::Printf(
			TEXT("파이어볼 쿨타임 %s %s\n")
			TEXT("Cost GE: 마나 -20 · Cooldown GE: 2초 · Damage GE: 체력 -25\n")
			TEXT("마나 재생 GE(Infinite, Period 1, +5) : %s\n")
			TEXT("발동 %d회 · 실패 %d회"),
			*D08::Bar(CooldownLeft / 2.f), CooldownLeft > 0.f ? *FString::Printf(TEXT("%.1f초"), CooldownLeft) : TEXT("준비됨"),
			bManaRegen ? TEXT("ON") : TEXT("OFF"), CastCount, FailCount));
	}
	SetStatus(LastResult);
}
