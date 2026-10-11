#include "Demo0515_Aura.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EffectsDemoKit.h"
#include "GameFramework/Character.h"
#include "GlossaryKit.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"

using namespace GlossaryKit;

ADemo0515_Aura::ADemo0515_Aura()
{
	DemoId = TEXT("0515");
	Controls = TEXT("WASD 이동 · 마우스 시점   C : 버프 오라 ON/OFF\n")
		TEXT("관찰: 캐릭터에 붙은 오라는 두 층 — 발밑에서 회전하는 마법진(가산 머티리얼 판)과\n")
		TEXT("몸을 감싸며 위로 올라가는 Niagara 입자. 캐릭터가 움직이면 같이 따라간다");
	Showcase = TEXT("캐릭터 발밑 회전 마법진 + 위로 올라가는 입자 버프 오라, C 키로 켜고 끄기");
	Environment.Sky = EGlossarySky::Night;
	Environment.ExposureEV100 = 3.f;
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-200.f, 0.f, 100.f));
}

void ADemo0515_Aura::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	for (int32 i = 0; i < 5; ++i)
	{
		SpawnShape(World, EShape::Cylinder, FVector(500.f, -600.f + i * 300.f, 150.f), FVector(60.f, 60.f, 300.f), Colors::Wall);
	}
	SpawnLabel(World, TEXT("C : 버프 오라 ON/OFF"), FVector(300.f, 0.f, 300.f), Colors::Item, 20.f);
}

void ADemo0515_Aura::SetAura(bool bOn)
{
	bAuraOn = bOn;
	if (AuraFX)
	{
		bOn ? AuraFX->Activate(true) : AuraFX->Deactivate();   // Deactivate: 새 입자만 멈추고 남은 입자는 자연스럽게 사라진다
	}
	if (MagicCircle)
	{
		MagicCircle->SetVisibility(bOn);
	}
}

void ADemo0515_Aura::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ACharacter* Character = GetPlayerCharacter();
	if (!bAttached && Character)
	{
		// 캐릭터(캡슐 루트)에 붙인다: 캡슐 바닥 = -HalfHeight
		const float Foot = -Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		MagicCircle = AddShape(Character, EShape::Plane, FVector(0.f, 0.f, Foot + 2.f), FVector(260.f, 260.f, 1.f), FLinearColor::White);
		if (UMaterialInterface* CircleMat = LoadAsset<UMaterialInterface>(EffectsKit::AssetPath(TEXT("M_FX_MagicCircle"))))
		{
			MagicCircle->SetMaterial(0, CircleMat);
		}
		MagicCircle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MagicCircle->SetCastShadow(false);
		MagicCircle->SetUsingAbsoluteRotation(true);   // 캐릭터가 돌아도 마법진은 자기 속도로만 돈다
		AuraFX = EffectsKit::AddNiagara(Character, TEXT("NS_AuraRise"), FVector(0.f, 0.f, Foot + 5.f));
		bAttached = true;
		SetAura(true);
	}
	if (WasKeyPressed(EKeys::C))
	{
		SetAura(!bAuraOn);
	}
	if (MagicCircle && bAuraOn)
	{
		MagicCircle->AddWorldRotation(FRotator(0.f, CircleRotateSpeed * DeltaSeconds, 0.f));
	}
	SetStatus(FString::Printf(TEXT("버프 오라 %s · 마법진 회전 %.0f°/초 · 상승 입자 NS_AuraRise"), bAuraOn ? TEXT("ON") : TEXT("OFF"), CircleRotateSpeed));
}
