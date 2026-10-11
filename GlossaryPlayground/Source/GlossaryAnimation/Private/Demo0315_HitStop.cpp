#include "Demo0315_HitStop.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "D0305_AnimNotify_Hit.h"
#include "D03_Mannequin.h"
#include "Demo0304_Montage.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryKit.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

using namespace GlossaryKit;

ADemo0315_HitStop::ADemo0315_HitStop()
{
	DemoId = TEXT("0315");
	Controls = TEXT("1 : 히트 스톱 ON/OFF   2 : 길이 ×6 (0.48초, 느리게 확인용)\n")
		TEXT("관찰: 주먹이 닿는 순간 두 캐릭터만 0.08초 거의 멈췄다가(Custom Time Dilation 0.05) 이어진다.\n")
		TEXT("월드 시간은 그대로라 옆의 회전 큐브는 멈추지 않는다 — 타격감은 '두 액터만' 멈추는 데서 나온다");
	Showcase = TEXT("타격 Notify 순간 공격자·피격자 Custom Time Dilation 0.05 를 0.08초 적용, 회전 큐브는 그대로, ON/OFF·6배 길이 비교");
	PlayerPawnClass = nullptr;
}

void ADemo0315_HitStop::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	Combo = D03::LoadMontage(TEXT("AM_Attack_Combo"));
	HitSound = LoadAsset<USoundBase>(D03::Folder + TEXT("S_Hit"));
	HitReact = D03::LoadAnim(TEXT("Rifle/HitReact/MM_HitReact_Front_Lgt_01"));

	Attacker = D03::SpawnMannequin(World, FVector(0.f, -60.f, 0.f), 90.f);
	Target = D03::SpawnMannequin(World, FVector(0.f, 70.f, 0.f), -90.f);
	if (Target) { SetColor(Target->GetMesh(), Colors::Enemy, 0.5f); }

	Spinner = SpawnShape(World, EShape::Cube, FVector(250.f, 0.f, 120.f), FVector(60.f), Colors::Cyan);
	SpawnLabel(World, TEXT("월드 시간 기준 (안 멈춤)"), FVector(250.f, 0.f, 220.f), Colors::Cyan, 15.f);
	Flash = SpawnShape(World, EShape::Sphere, FVector(0, 0, -500), FVector(40.f), Colors::Accent);
	if (Flash)
	{
		SetColor(Flash->GetStaticMeshComponent(), Colors::Accent, 0.3f, 0.f, 25.f);
		Flash->SetActorHiddenInGame(true);
		Flash->SetActorEnableCollision(false);
	}
	HitHandle = UD0305_AnimNotify_Hit::OnHit.AddUObject(this, &ADemo0315_HitStop::OnAnimHit);
	D03::SpawnViewCamera(World, GetPC(), FVector(-420.f, 60.f, 190.f), FVector(60.f, 10.f, 110.f), 55.f);
}

void ADemo0315_HitStop::EndPlay(const EEndPlayReason::Type Reason)
{
	UD0305_AnimNotify_Hit::OnHit.Remove(HitHandle);
	Super::EndPlay(Reason);
}

void ADemo0315_HitStop::SetDilation(float Value)
{
	// 핵심: 월드(Global Time Dilation)가 아니라 두 액터의 Custom Time Dilation 만 바꾼다
	if (Attacker) { Attacker->CustomTimeDilation = Value; }
	if (Target) { Target->CustomTimeDilation = Value; }
}

void ADemo0315_HitStop::OnAnimHit(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation)
{
	if (!Mesh || Mesh->GetOwner() != Attacker)
	{
		return;
	}
	if (HitSound) { UGameplayStatics::PlaySoundAtLocation(this, HitSound, Target->GetActorLocation()); }
	if (UAnimInstance* TA = Target ? Target->GetMesh()->GetAnimInstance() : nullptr; TA && HitReact)
	{
		TA->PlaySlotAnimationAsDynamicMontage(HitReact, TEXT("DefaultSlot"), 0.05f, 0.25f);
	}
	if (Flash)
	{
		Flash->SetActorLocation(Target->GetActorLocation() + FVector(0, -30.f, 45.f));
		Flash->SetActorHiddenInGame(false);
	}
	if (bHitStopEnabled)
	{
		SetDilation(HitStopDilation);
		// 타이머는 실시간 기준 — 멈춘 액터의 시간으로 재면 0.08초가 1.6초가 된다
		StopStartRealTime = FPlatformTime::Seconds();
		StopEndRealTime = StopStartRealTime + HitStopDuration * DurationScale;
		bStopping = true;
		++StopCount;
	}
}

void ADemo0315_HitStop::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One)) { bHitStopEnabled = !bHitStopEnabled; }
	if (WasKeyPressed(EKeys::Two)) { DurationScale = DurationScale > 1.f ? 1.f : 6.f; }

	const double Now = FPlatformTime::Seconds();
	if (bStopping && Now >= StopEndRealTime)
	{
		SetDilation(1.f);
		bStopping = false;
		LastStopLength = static_cast<float>(Now - StopStartRealTime);
		if (Flash) { Flash->SetActorHiddenInGame(true); }
	}
	if (!bStopping && Flash && !Flash->IsHidden() && !bHitStopEnabled)
	{
		Flash->SetActorHiddenInGame(true);
	}

	// 공격자는 3.5초마다 3단 콤보 (몽타주 진행도 Custom Time Dilation 의 영향을 받는다)
	AttackTimer += DeltaSeconds;
	const float PressTimes[] = { 0.f, 0.45f, 1.0f };
	if (Presses < 3 && AttackTimer >= PressTimes[Presses])
	{
		ADemo0304_Montage::ComboInput(Attacker, Combo);
		++Presses;
	}
	if (AttackTimer >= 3.5f)
	{
		AttackTimer = 0.f;
		Presses = 0;
		if (Attacker) { Attacker->SetActorLocationAndRotation(FVector(0.f, -60.f, 97.f), FRotator(0.f, 90.f, 0.f)); }
	}
	if (Spinner)
	{
		Spinner->AddActorWorldRotation(FRotator(0.f, 180.f * DeltaSeconds, 0.f));
	}
	if (Attacker)
	{
		Attacker->SetHeadLabel(bStopping ? TEXT("HIT STOP!") : TEXT("공격자"), bStopping ? Colors::Accent : FLinearColor::White, bStopping ? 22.f : 15.f);
	}
	SetStatus(FString::Printf(TEXT("히트 스톱 %s · Custom Time Dilation %.2f · 길이 %.2f 초%s\n횟수 %d · 마지막 실제 정지 %.3f 초 · 지금 공격자 Dilation %.2f"),
		bHitStopEnabled ? TEXT("ON") : TEXT("OFF"), HitStopDilation, HitStopDuration * DurationScale, DurationScale > 1.f ? TEXT(" (×6 확인용)") : TEXT(""),
		StopCount, LastStopLength, Attacker ? Attacker->CustomTimeDilation : 1.f));
}
