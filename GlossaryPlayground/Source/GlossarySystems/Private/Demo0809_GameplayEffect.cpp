#include "Demo0809_GameplayEffect.h"

#include "D08_Common.h"
#include "D08_GAS.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0809_GameplayEffect::ADemo0809_GameplayEffect()
{
	DemoId = TEXT("0809");
	Controls = TEXT("1 : 왼쪽 표적에 독 GE · 2 : 오른쪽 표적에 독 GE · 3 : 둘 다 회복 GE(+40)\n")
		TEXT("관찰: 독을 걸면 1초마다 -10 씩 5번 깎이고, 걸려 있는 동안 State.Poisoned 태그(초록)가 붙는다");
	Showcase = TEXT("Health·Mana AttributeSet 표적 2개에 독 GE(5초·Period 1·-10) 적용, 1초마다 틱과 남은 시간·태그 표시");
}

void ADemo0809_GameplayEffect::BeginPlay()
{
	Super::BeginPlay();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const TCHAR* Names[] = { TEXT("표적 A"), TEXT("표적 B") };
	for (int32 i = 0; i < 2; ++i)
	{
		AD08_GASDummy* Dummy = GetWorld()->SpawnActor<AD08_GASDummy>(FVector(300.f, i == 0 ? -250.f : 250.f, 90.f), FRotator::ZeroRotator, Params);
		Dummy->SetDisplayName(Names[i]);
		Targets.Add(Dummy);
	}
	Panel = UD08_SidePanel::Create(GetPC(), TEXT("독 GE 틱 기록 (표적 A)"), UD08_SidePanel::EPlace::TopRight, 460.f);
	AutoTimer = AutoPoisonDelay;
}

void ADemo0809_GameplayEffect::ApplyPoison(AD08_GASDummy* Target)
{
	if (!Target)
	{
		return;
	}
	// 시전자 없이 자기 자신을 Source 로 적용 (실제 게임에서는 공격자의 ASC 가 Source)
	UAbilitySystemComponent* ASC = Target->GetAbilitySystemComponent();
	D08GAS::ApplyEffect(ASC, ASC, UD0809_GE_Poison::StaticClass());
	AD08_FloatingText::Spawn(GetWorld(), TEXT("독!"), Target->GetActorLocation() + FVector(0, 0, 170), Colors::Ally, 26.f);
}

void ADemo0809_GameplayEffect::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Targets.Num() < 2)
	{
		return;
	}
	if (AutoTimer > 0.f)
	{
		AutoTimer -= DeltaSeconds;
		if (AutoTimer <= 0.f)
		{
			ApplyPoison(Targets[0]);
		}
	}
	if (WasKeyPressed(EKeys::One)) { ApplyPoison(Targets[0]); }
	if (WasKeyPressed(EKeys::Two)) { ApplyPoison(Targets[1]); }
	if (WasKeyPressed(EKeys::Three))
	{
		for (AD08_GASDummy* T : Targets)
		{
			UAbilitySystemComponent* ASC = T->GetAbilitySystemComponent();
			D08GAS::ApplyEffect(ASC, ASC, UD0809_GE_Heal::StaticClass());
		}
	}

	FString Lines;
	FString StatusText;
	for (int32 i = 0; i < Targets.Num(); ++i)
	{
		AD08_GASDummy* T = Targets[i];
		UAbilitySystemComponent* ASC = T->GetAbilitySystemComponent();
		// 독 GE 의 남은 시간: State.Poisoned 태그를 주는 활성 GE 를 찾는다
		const TArray<float> Remaining = ASC->GetActiveEffectsTimeRemaining(
			FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(D08GAS::PoisonedTag())));
		const int32 PoisonCount = Remaining.Num();
		const float Left = PoisonCount > 0 ? FMath::Max(0.f, FMath::Max(Remaining)) : 0.f;
		const UD08_AttributeSet* A = T->GetAttributes();
		StatusText += FString::Printf(TEXT("%s: Health %.0f · Mana %.0f · 독 %d개 %s\n"),
			i == 0 ? TEXT("A") : TEXT("B"), A->GetHealth(), A->GetMana(), PoisonCount,
			PoisonCount > 0 ? *FString::Printf(TEXT("(남은 %.1f초)"), Left) : TEXT(""));
	}
	for (const FString& Line : Targets[0]->HealthLog)
	{
		Lines += Line + TEXT("\n");
	}
	if (Panel)
	{
		Panel->SetBody(FString::Printf(TEXT("GE: DurationPolicy HasDuration 5초 · Period 1초\nModifier: Health Add -10 · 부여 태그 State.Poisoned\n\n%s"),
			Lines.IsEmpty() ? TEXT("(아직 변화 없음)") : *Lines));
	}
	SetStatus(StatusText.TrimEnd());
}
