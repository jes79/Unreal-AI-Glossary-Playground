#include "Demo0305_AnimNotify.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "D0305_AnimNotify_Hit.h"
#include "D03_GraphWidget.h"
#include "D03_Mannequin.h"
#include "Demo0304_Montage.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GlossaryKit.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

using namespace GlossaryKit;

ADemo0305_AnimNotify::ADemo0305_AnimNotify()
{
	DemoId = TEXT("0305");
	Controls = TEXT("1 : AnimNotify 판정   2 : 입력 즉시 판정(나쁜 예)   (공격자가 3.5초마다 3단 콤보)\n")
		TEXT("관찰: 주먹이 다 뻗는 프레임(타임라인의 빨간 Hit 선)에서만 데미지·효과음·피격 리액션이 난다\n")
		TEXT("2 로 바꾸면 휘두르기도 전에 맞아 버린다 — 타격 타이밍을 애니메이션에 맡기는 이유");
	Showcase = TEXT("콤보 몽타주 각 섹션의 타격 프레임에 C++ AnimNotify, Notify 순간 Sphere Overlap 판정·효과음·피격 리액션, 즉시 판정과 비교");
	PlayerPawnClass = nullptr;
}

void ADemo0305_AnimNotify::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	Combo = D03::LoadMontage(TEXT("AM_Attack_Combo"));
	HitSound = LoadAsset<USoundBase>(D03::Folder + TEXT("S_Hit"));
	HitReact = D03::LoadAnim(TEXT("Rifle/HitReact/MM_HitReact_Front_Lgt_01"));

	Attacker = D03::SpawnMannequin(World, FVector(0.f, -60.f, 0.f), 90.f);
	Target = D03::SpawnMannequin(World, FVector(0.f, 70.f, 0.f), -90.f);
	if (Target)
	{
		// 허수아비는 빨간 옷
		SetColor(Target->GetMesh(), Colors::Enemy, 0.5f);
	}
	Flash = SpawnShape(World, EShape::Sphere, FVector(0, 0, -500), FVector(40.f), Colors::Accent, FRotator::ZeroRotator, false, true);
	if (Flash)
	{
		SetColor(Flash->GetStaticMeshComponent(), Colors::Accent, 0.3f, 0.f, 20.f);
		Flash->SetActorHiddenInGame(true);
		Flash->SetActorEnableCollision(false);
	}

	// 전역 Notify 이벤트 구독: 어떤 캐릭터의 몽타주에서든 'Hit' 프레임을 지나면 불린다
	HitHandle = UD0305_AnimNotify_Hit::OnHit.AddUObject(this, &ADemo0305_AnimNotify::OnAnimHit);
	D03::SpawnViewCamera(World, GetPC(), FVector(-420.f, 60.f, 190.f), FVector(60.f, 10.f, 110.f), 55.f);

	Graph = UD03_GraphWidget::Create(GetPC(), TEXT("AM_Attack_Combo · 빨간 선 = AnimNotify 'Hit'"));
	if (Graph)
	{
		Graph->PanelSize = FVector2D(560.f, 200.f);
		Graph->YMin = 0.f; Graph->YMax = 1.f; Graph->GridY = 0;
		Graph->XLabel = TEXT("몽타주 시간 (초)");
	}
}

void ADemo0305_AnimNotify::EndPlay(const EEndPlayReason::Type Reason)
{
	UD0305_AnimNotify_Hit::OnHit.Remove(HitHandle);
	Super::EndPlay(Reason);
}

void ADemo0305_AnimNotify::OnAnimHit(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation)
{
	ACharacter* MeshOwner = Mesh ? Cast<ACharacter>(Mesh->GetOwner()) : nullptr;
	if (!bUseNotify || !MeshOwner || MeshOwner == Target || MeshOwner->GetWorld() != GetWorld())
	{
		return;
	}
	UAnimInstance* Anim = Mesh->GetAnimInstance();
	const FName Section = Anim && Combo ? Anim->Montage_GetCurrentSection(Combo) : NAME_None;
	DoHitCheck(MeshOwner, FString::Printf(TEXT("Notify '%s' @ %s %.2fs"), *Section.ToString(), *GetNameSafe(Animation),
		Anim && Combo ? Anim->Montage_GetPosition(Combo) : 0.f));
}

void ADemo0305_AnimNotify::DoHitCheck(ACharacter* Who, const FString& Source)
{
	if (!Who || !Target)
	{
		return;
	}
	// 데미지 판정: 공격자 앞 HitForward 위치의 구 안에 대상 캡슐이 있는가
	const FVector Center = Who->GetActorLocation() + Who->GetActorForwardVector() * HitForward + FVector(0, 0, 20.f);
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(D0305Hit), false, Who);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(HitRadius), Params);
	bool bHit = false;
	for (const FOverlapResult& O : Overlaps)
	{
		bHit |= O.GetActor() == Target;
	}
	LastHit = FString::Printf(TEXT("%s → %s"), *Source, bHit ? TEXT("명중") : TEXT("빗나감"));
	if (!bHit)
	{
		return;
	}
	++HitCount;
	TargetHP = FMath::Max(0.f, TargetHP - Damage);
	if (TargetHP <= 0.f) { TargetHP = 100.f; }
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, Center);
	}
	if (UAnimInstance* TA = Target->GetMesh()->GetAnimInstance(); TA && HitReact)
	{
		TA->PlaySlotAnimationAsDynamicMontage(HitReact, TEXT("DefaultSlot"), 0.05f, 0.25f);
	}
	if (Flash)
	{
		Flash->SetActorLocation(Target->GetActorLocation() + FVector(0, 0, 40.f) - Target->GetActorForwardVector() * -10.f);
		Flash->SetActorHiddenInGame(false);
		FlashTimer = 0.12f;
	}
}

void ADemo0305_AnimNotify::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (WasKeyPressed(EKeys::One)) { bUseNotify = true; }
	if (WasKeyPressed(EKeys::Two)) { bUseNotify = false; }

	auto Press = [this](ACharacter* Who)
	{
		ADemo0304_Montage::ComboInput(Who, Combo);
		if (!bUseNotify)
		{
			// 나쁜 예: 입력 순간 바로 판정 — 애니메이션은 아직 팔을 들지도 않았다
			DoHitCheck(Who, TEXT("입력 즉시"));
		}
	};
	if (WasKeyPressed(EKeys::LeftMouseButton) || WasKeyPressed(EKeys::F))
	{
		Press(GetPlayerCharacter());
	}
	AttackTimer += DeltaSeconds;
	const float PressTimes[] = { 0.f, 0.45f, 1.0f };
	if (Presses < 3 && AttackTimer >= PressTimes[Presses])
	{
		Press(Attacker);
		++Presses;
	}
	if (AttackTimer >= 3.5f)
	{
		AttackTimer = 0.f;
		Presses = 0;
		// 공격 시퀀스에 Root Motion 이 있어 조금씩 앞으로 나가므로 주기마다 제자리로
		if (Attacker) { Attacker->SetActorLocationAndRotation(FVector(0.f, -60.f, 97.f), FRotator(0.f, 90.f, 0.f)); }
	}

	if (FlashTimer > 0.f)
	{
		FlashTimer -= DeltaSeconds;
		if (FlashTimer <= 0.f && Flash) { Flash->SetActorHiddenInGame(true); }
	}

	ADemo0304_Montage::DrawMontageTimeline(Graph, Combo, Attacker, true);
	if (Target)
	{
		Target->SetHeadLabel(FString::Printf(TEXT("허수아비 HP %.0f"), TargetHP), Colors::Enemy, 16.f);
	}
	SetStatus(FString::Printf(TEXT("판정 방식: %s · 명중 %d회\n마지막: %s"),
		bUseNotify ? TEXT("AnimNotify 'Hit' (타격 프레임)") : TEXT("입력 즉시 (Notify 무시)"), HitCount, *LastHit));
}
