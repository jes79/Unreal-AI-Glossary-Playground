#include "Demo0527_AutoExposure.h"

#include "EffectsDemoKit.h"
#include "Engine/PostProcessVolume.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

namespace
{
	const FVector CaveCenter(0.f, 0.f, 0.f);
	const FVector InsideSpot(-100.f, 0.f, 100.f);
	const FVector OutsideSpot(-1300.f, 0.f, 100.f);
}

ADemo0527_AutoExposure::ADemo0527_AutoExposure()
{
	DemoId = TEXT("0527");
	Controls = TEXT("WASD 이동   1 : 동굴 밖으로   2 : 동굴 안으로   E : Auto Exposure ↔ 고정 노출\n")
		TEXT("관찰: 어두운 동굴에 적응한 눈(노출)으로 밖에 나오면 하얗게 눈부셨다가 Speed Up 0.6 으로 천천히 적응한다.\n")
		TEXT("반대로 동굴에 들어가면 처음엔 깜깜했다가 Speed Down 2 로 빠르게 밝아진다. 범위는 Min/Max EV100 -2 ~ 14 로 고정");
	Showcase = TEXT("어두운 동굴 ↔ 밝은 바깥 이동 시 Auto Exposure 눈 적응(Speed Up/Down, Min/Max EV100 고정), 고정 노출과 비교");
	Environment.bFixedExposure = false;   // 노출 데모: 환경의 노출 고정을 끈다
	PlayerStartTransform = FTransform(FRotator(0.f, 180.f, 0.f), InsideSpot);
}

void ADemo0527_AutoExposure::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	// 동굴: 사방이 막히고 -X 쪽 입구만 열린 상자 (안은 해가 거의 들지 않는다)
	const FLinearColor Rock(0.2f, 0.18f, 0.16f);
	SpawnBlock(World, FVector(0.f, -450.f, 0.f), FVector(1200.f, 100.f, 500.f), Rock);
	SpawnBlock(World, FVector(0.f, 450.f, 0.f), FVector(1200.f, 100.f, 500.f), Rock);
	SpawnBlock(World, FVector(0.f, 0.f, 500.f), FVector(1200.f, 1000.f, 100.f), Rock);
	SpawnBlock(World, FVector(650.f, 0.f, 0.f), FVector(100.f, 1000.f, 500.f), Rock);
	SpawnBlock(World, FVector(-600.f, -270.f, 0.f), FVector(100.f, 260.f, 500.f), Rock);   // 입구 양옆 벽 (가운데만 뚫림)
	SpawnBlock(World, FVector(-600.f, 270.f, 0.f), FVector(100.f, 260.f, 500.f), Rock);
	SpawnBlock(World, FVector(-600.f, 0.f, 300.f), FVector(100.f, 280.f, 200.f), Rock);
	// 바깥: 밝은 색 상자들
	for (int32 i = 0; i < 6; ++i)
	{
		SpawnBlock(World, FVector(-1800.f - (i % 2) * 300.f, -900.f + i * 360.f, 0.f), FVector(200.f, 200.f, 200.f + i * 50.f), i % 2 ? Colors::Item : Colors::Neutral);
	}
	SpawnLabel(World, TEXT("동굴 입구"), FVector(-700.f, 0.f, 330.f), Colors::Item, 20.f);

	// Auto Exposure 설정 (Histogram, Min/Max EV100 고정, 적응 속도)
	Volume = EffectsKit::SpawnUnboundPostProcess(World);
	FPostProcessSettings& S = Volume->Settings;
	S.bOverride_AutoExposureMethod = true;      S.AutoExposureMethod = AEM_Histogram;
	S.bOverride_AutoExposureMinBrightness = true; S.AutoExposureMinBrightness = MinEV100;   // Extend Default Luminance Range(기본 켜짐) → EV100 단위
	S.bOverride_AutoExposureMaxBrightness = true; S.AutoExposureMaxBrightness = MaxEV100;
	S.bOverride_AutoExposureSpeedUp = true;     S.AutoExposureSpeedUp = SpeedUp;     // 어두운 곳 → 밝은 곳 (느리게 = 눈부심이 오래)
	S.bOverride_AutoExposureSpeedDown = true;   S.AutoExposureSpeedDown = SpeedDown; // 밝은 곳 → 어두운 곳
	S.bOverride_AutoExposureBias = true;        S.AutoExposureBias = 0.f;
}

void ADemo0527_AutoExposure::Teleport(bool bInside)
{
	if (ACharacter* Character = GetPlayerCharacter())
	{
		const FRotator Facing(0.f, 180.f, 0.f);   // 항상 바깥(밝은 쪽)을 본다
		Character->TeleportTo(bInside ? InsideSpot : OutsideSpot, Facing);
		if (APlayerController* PC = GetPC())
		{
			PC->SetControlRotation(FRotator(-5.f, Facing.Yaw, 0.f));
		}
	}
	Where = bInside ? TEXT("동굴 안") : TEXT("바깥");
}

void ADemo0527_AutoExposure::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One)) Teleport(false);
	if (WasKeyPressed(EKeys::Two)) Teleport(true);
	if (WasKeyPressed(EKeys::E))
	{
		bAuto = !bAuto;
		if (Volume)
		{
			// 고정 노출 비교: Manual + Apply Physical Camera Exposure 끔 → Exposure Compensation 0 = 다른 데모의 고정 EV100 10 과 같은 밝기
			Volume->Settings.AutoExposureMethod = bAuto ? AEM_Histogram : AEM_Manual;
			Volume->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
			Volume->Settings.AutoExposureApplyPhysicalCameraExposure = false;
			Volume->Settings.AutoExposureBias = 0.f;
		}
	}
	if (ACharacter* Character = GetPlayerCharacter())
	{
		Where = Character->GetActorLocation().X > -600.f ? TEXT("동굴 안") : TEXT("바깥");
	}
	SetStatus(FString::Printf(TEXT("%s · %s · Speed Up %.1f / Down %.1f · EV100 범위 %.0f ~ %.0f"),
		*Where, bAuto ? TEXT("Auto Exposure (Histogram)") : TEXT("고정 노출 (Manual)"), SpeedUp, SpeedDown, MinEV100, MaxEV100));
}
