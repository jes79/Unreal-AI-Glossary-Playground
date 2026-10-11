#include "Demo0807_GameplayTags.h"

#include "Components/StaticMeshComponent.h"
#include "D08_Common.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0807_GameplayTags::ADemo0807_GameplayTags()
{
	DemoId = TEXT("0807");
	Controls = TEXT("1 : State.Stunned · 2 : State.Burning · 3 : State.Poisoned · 4 : State.Invincible 붙이기/떼기\n")
		TEXT("관찰: 기절이면 순찰을 멈추고, 화상·중독이면 체력이 깎이고, 무적이면 데미지를 무시한다 — 모두 HasTag 한 줄");
	Showcase = TEXT("적 상태를 bool 대신 Gameplay Tag 컨테이너로 관리, 1~4로 State.* 태그를 붙이면 행동·색이 바뀜");
}

void ADemo0807_GameplayTags::BeginPlay()
{
	Super::BeginPlay();
	// DefaultGame.ini 의 GameplayTagList 에 정의된 태그를 이름으로 얻는다
	TagState = FGameplayTag::RequestGameplayTag(TEXT("State"));
	TagStunned = FGameplayTag::RequestGameplayTag(TEXT("State.Stunned"));
	TagBurning = FGameplayTag::RequestGameplayTag(TEXT("State.Burning"));
	TagPoisoned = FGameplayTag::RequestGameplayTag(TEXT("State.Poisoned"));
	TagInvincible = FGameplayTag::RequestGameplayTag(TEXT("State.Invincible"));

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Enemy = GetWorld()->SpawnActor<AD08_Dummy>(FVector(300.f, 0.f, 90.f), FRotator::ZeroRotator, Params);
	Enemy->RespawnDelay = 2.f;
	Enemy->Configure(TEXT("적"), 100.f, Colors::Enemy);

	// 기절 표시용 별 3개 (머리 위를 돈다)
	for (int32 i = 0; i < 3; ++i)
	{
		// 표적 메시는 원기둥 비율로 늘어나 있으므로 데모 액터에 붙이고 월드 위치로 움직인다
		UStaticMeshComponent* Star = AddShape(this, EShape::Sphere, FVector::ZeroVector, FVector(22.f), Colors::Item);
		SetColor(Star, Colors::Item, 0.3f, 0.f, 4.f);
		Star->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Stars.Add(Star);
	}

	// 순찰 경로 표시
	SpawnBlock(GetWorld(), FVector(300.f, 0.f, 0.f), FVector(60.f, 1400.f, 4.f), Colors::Dark);

	Panel = UD08_SidePanel::Create(GetPC(), TEXT("StateTags (FGameplayTagContainer)"), UD08_SidePanel::EPlace::TopRight, 460.f);

	// 시작 상태: 화상 하나
	StateTags.AddTag(TagBurning);
}

void ADemo0807_GameplayTags::Toggle(const FGameplayTag& Tag)
{
	if (StateTags.HasTagExact(Tag))
	{
		StateTags.RemoveTag(Tag);
	}
	else
	{
		StateTags.AddTag(Tag);
	}
}

void ADemo0807_GameplayTags::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Enemy)
	{
		return;
	}
	Time += DeltaSeconds;
	if (WasKeyPressed(EKeys::One))   { Toggle(TagStunned); }
	if (WasKeyPressed(EKeys::Two))   { Toggle(TagBurning); }
	if (WasKeyPressed(EKeys::Three)) { Toggle(TagPoisoned); }
	if (WasKeyPressed(EKeys::Four))  { Toggle(TagInvincible); }

	// ---- 태그로 행동 결정 ----
	const bool bStunned = StateTags.HasTag(TagStunned);
	if (!bStunned)
	{
		PatrolPhase += DeltaSeconds * PatrolSpeed / 600.f;
		Enemy->SetActorLocation(FVector(300.f, FMath::Sin(PatrolPhase) * 600.f, 90.f));
	}
	for (int32 i = 0; i < Stars.Num(); ++i)
	{
		const float A = Time * 4.f + i * 2.f * PI / 3.f;
		Stars[i]->SetVisibility(bStunned);
		Stars[i]->SetWorldLocation(Enemy->GetActorLocation() + FVector(FMath::Cos(A) * 60.f, FMath::Sin(A) * 60.f, 130.f));
	}

	// 지속 데미지: 화상·중독. 무적 태그가 있으면 무시
	float Dps = 0.f;
	if (StateTags.HasTag(TagBurning))  { Dps += BurnDps; }
	if (StateTags.HasTag(TagPoisoned)) { Dps += PoisonDps; }
	if (StateTags.HasTag(TagInvincible))
	{
		Dps = 0.f;
	}
	DotAccum += Dps * DeltaSeconds;
	if (DotAccum >= 5.f)
	{
		const bool bBurn = StateTags.HasTag(TagBurning);
		Enemy->ApplyHit(DotAccum, bBurn ? Colors::Accent : Colors::Ally, bBurn ? TEXT("화상") : TEXT("중독"));
		DotAccum = 0.f;
	}

	// 색: 우선순위 무적 > 화상 > 중독 > 기본
	FLinearColor Color = Colors::Enemy;
	float Glow = 0.f;
	if (StateTags.HasTag(TagInvincible))     { Color = FLinearColor(0.9f, 0.9f, 1.f); Glow = 1.5f; }
	else if (StateTags.HasTag(TagBurning))   { Color = Colors::Accent; Glow = 1.5f + FMath::Sin(Time * 12.f); }
	else if (StateTags.HasTag(TagPoisoned))  { Color = Colors::Ally; Glow = 0.6f; }
	Enemy->BaseColor = Color;
	Enemy->SetBodyColor(Color, FMath::Max(0.f, Glow));

	// 부모 태그 검사: State.* 중 하나라도 있으면 true (HasTag 는 부모 매칭, HasTagExact 는 정확히 같은 것만)
	const bool bAnyState = StateTags.HasTag(TagState);
	const bool bExactState = StateTags.HasTagExact(TagState);

	if (Panel)
	{
		Panel->SetBody(FString::Printf(
			TEXT("%s\n\n")
			TEXT("HasTag(State.Stunned)   = %s → %s\n")
			TEXT("HasTag(State.Burning)   = %s\n")
			TEXT("HasTag(State.Poisoned)  = %s\n")
			TEXT("HasTag(State.Invincible)= %s\n")
			TEXT("HasTag(State)      = %s (부모 태그 매칭)\n")
			TEXT("HasTagExact(State) = %s"),
			StateTags.IsEmpty() ? TEXT("(태그 없음)") : *StateTags.ToStringSimple(),
			bStunned ? TEXT("true") : TEXT("false"), bStunned ? TEXT("순찰 멈춤") : TEXT("순찰 중"),
			StateTags.HasTag(TagBurning) ? TEXT("true") : TEXT("false"),
			StateTags.HasTag(TagPoisoned) ? TEXT("true") : TEXT("false"),
			StateTags.HasTag(TagInvincible) ? TEXT("true") : TEXT("false"),
			bAnyState ? TEXT("true") : TEXT("false"),
			bExactState ? TEXT("true") : TEXT("false")));
	}
	SetStatus(FString::Printf(TEXT("태그 %d개 · 지속 데미지 %.0f/초 · 적 HP %.0f"), StateTags.Num(), Dps, Enemy->Health));
}
