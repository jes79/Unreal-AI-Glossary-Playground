#include "Demo0304_Montage.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "D03_GraphWidget.h"
#include "D03_Mannequin.h"
#include "GameFramework/Character.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0304_Montage::ADemo0304_Montage()
{
	DemoId = TEXT("0304");
	Controls = TEXT("WASD 이동 · 왼쪽 클릭 또는 F : 공격\n")
		TEXT("섹션이 끝나기 전에 다시 누르면 Attack1 → Attack2 → Attack3 로 이어진다 (늦으면 몽타주 종료)\n")
		TEXT("관찰: 오른쪽 아래 타임라인의 커서가 섹션을 넘어가는 순간과 '예약된 다음 섹션'");
	Showcase = TEXT("코드로 만든 3섹션 몽타주(Attack1/2/3)를 공격 입력으로 재생, Montage_SetNextSection 콤보, 섹션 타임라인 표시");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-500.f, 0.f, 100.f));
}

void ADemo0304_Montage::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	Combo = D03::LoadMontage(TEXT("AM_Attack_Combo"));
	Npc = D03::SpawnMannequin(World, FVector(0.f, 260.f, 0.f), 180.f);
	SpawnLabel(World, TEXT("NPC: 3.5초마다 3단 콤보"), FVector(0.f, 260.f, 250.f), Colors::Item, 18.f);
	Graph = UD03_GraphWidget::Create(GetPC(), TEXT("AM_Attack_Combo 섹션 (플레이어)"));
	if (Graph)
	{
		Graph->PanelSize = FVector2D(560.f, 200.f);
		Graph->YMin = 0.f; Graph->YMax = 1.f; Graph->GridY = 0;
		Graph->XLabel = TEXT("몽타주 시간 (초)");
	}
}

FName ADemo0304_Montage::ComboInput(ACharacter* Character, UAnimMontage* Montage)
{
	UAnimInstance* Anim = Character && Montage ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!Anim)
	{
		return NAME_None;
	}
	if (!Anim->Montage_IsPlaying(Montage))
	{
		// 처음 누름: 몽타주 재생 (첫 섹션 Attack1 부터)
		Anim->Montage_Play(Montage, 1.f);
		return TEXT("Attack1");
	}
	// 재생 중에 누름: 지금 섹션이 끝나면 이어질 섹션을 예약
	const FName Current = Anim->Montage_GetCurrentSection(Montage);
	const FName Next = Current == TEXT("Attack1") ? FName(TEXT("Attack2")) : Current == TEXT("Attack2") ? FName(TEXT("Attack3")) : NAME_None;
	if (Next != NAME_None)
	{
		Anim->Montage_SetNextSection(Current, Next, Montage);
	}
	return Next;
}

void ADemo0304_Montage::DrawMontageTimeline(UD03_GraphWidget* Graph, UAnimMontage* Montage, ACharacter* Character, bool bNotifies)
{
	if (!Graph || !Montage)
	{
		return;
	}
	Graph->ClearDynamic();
	const float Length = Montage->GetPlayLength();
	Graph->XMin = 0.f;
	Graph->XMax = Length;
	Graph->GridX = FMath::Max(1, FMath::RoundToInt(Length / 0.5f));
	const FLinearColor Colors3[] = { FLinearColor(0.2f, 0.45f, 0.9f, 0.9f), FLinearColor(0.15f, 0.65f, 0.4f, 0.9f), FLinearColor(0.75f, 0.35f, 0.15f, 0.9f) };
	UAnimInstance* Anim = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	const bool bPlaying = Anim && Anim->Montage_IsActive(Montage);
	const FName Current = bPlaying ? Anim->Montage_GetCurrentSection(Montage) : NAME_None;
	for (int32 i = 0; i < Montage->CompositeSections.Num(); ++i)
	{
		const FCompositeSection& S = Montage->CompositeSections[i];
		const float Start = S.GetTime();
		const float End = Montage->CompositeSections.IsValidIndex(i + 1) ? Montage->CompositeSections[i + 1].GetTime() : Length;
		FLinearColor C = Colors3[i % 3];
		if (S.SectionName == Current) { C = FLinearColor(1.f, 0.8f, 0.1f, 0.95f); }
		Graph->Bars.Add({ Start, End, 0.35f, 0.85f, C, S.SectionName.ToString() });
	}
	if (bNotifies)
	{
		for (const FAnimNotifyEvent& E : Montage->Notifies)
		{
			Graph->VLines.Add({ E.GetTime(), FLinearColor(1.f, 0.15f, 0.15f) });
			Graph->Texts.Add({ FVector2D(E.GetTime(), 0.3f), TEXT("Hit"), FLinearColor(1.f, 0.3f, 0.3f), 10.f });
		}
	}
	if (bPlaying)
	{
		Graph->VLines.Add({ Anim->Montage_GetPosition(Montage), FLinearColor::White });
	}
}

FString ADemo0304_Montage::Describe(ACharacter* Character) const
{
	UAnimInstance* Anim = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!Anim || !Combo || !Anim->Montage_IsActive(Combo))
	{
		return TEXT("대기");
	}
	const FName Cur = Anim->Montage_GetCurrentSection(Combo);
	FAnimMontageInstance* Inst = Anim->GetActiveInstanceForMontage(Combo);
	FName Next = NAME_None;
	if (Inst)
	{
		Next = Inst->GetNextSection();
	}
	return FString::Printf(TEXT("섹션 %s (%.2fs) → 다음 %s%s"), *Cur.ToString(), Anim->Montage_GetPosition(Combo),
		Next.IsNone() ? TEXT("없음(끝나면 종료)") : *Next.ToString(), Anim->Montage_IsPlaying(Combo) ? TEXT("") : TEXT(" · 블렌드 아웃"));
}

void ADemo0304_Montage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (WasKeyPressed(EKeys::LeftMouseButton) || WasKeyPressed(EKeys::F))
	{
		PlayerQueued = ComboInput(GetPlayerCharacter(), Combo);
	}

	// NPC: 0.0 / 0.45 / 1.0 초에 눌러 3단을 잇는다
	NpcTimer += DeltaSeconds;
	const float PressTimes[] = { 0.f, 0.45f, 1.0f };
	if (NpcPresses < 3 && NpcTimer >= PressTimes[NpcPresses])
	{
		ComboInput(Npc, Combo);
		++NpcPresses;
	}
	if (NpcTimer >= NpcComboInterval)
	{
		NpcTimer = 0.f;
		NpcPresses = 0;
		// 공격 시퀀스의 Root Motion 으로 조금씩 앞으로 나가므로 주기마다 제자리로
		if (Npc) { Npc->SetActorLocationAndRotation(FVector(0.f, 260.f, 97.f), FRotator(0.f, 180.f, 0.f)); }
	}

	DrawMontageTimeline(Graph, Combo, GetPlayerCharacter(), false);
	if (Npc)
	{
		Npc->SetHeadLabel(Describe(Npc), Colors::Item, 15.f);
	}
	// 플레이어 섹션이 바뀔 때마다 기록
	if (ACharacter* P = GetPlayerCharacter())
	{
		UAnimInstance* PA = P->GetMesh()->GetAnimInstance();
		const FName Sec = PA && Combo && PA->Montage_IsActive(Combo) ? PA->Montage_GetCurrentSection(Combo) : NAME_None;
		if (Sec != PlayerLastSection && !Sec.IsNone())
		{
			PlayerHistory = (Sec == TEXT("Attack1") ? FString() : PlayerHistory + TEXT(" → ")) + Sec.ToString();
		}
		PlayerLastSection = Sec;
	}
	SetStatus(FString::Printf(TEXT("플레이어  %s\n플레이어 콤보 기록: %s\nNPC      %s"), *Describe(GetPlayerCharacter()), PlayerHistory.IsEmpty() ? TEXT("-") : *PlayerHistory, *Describe(Npc)));
}
