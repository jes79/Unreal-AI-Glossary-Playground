#include "Demo0520_PostProcessVolume.h"

#include "Camera/PlayerCameraManager.h"
#include "EffectsDemoKit.h"
#include "Engine/PostProcessVolume.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

namespace
{
	const FVector CaveCenter(1000.f, 0.f, 250.f);
	const FVector CaveSize(1000.f, 700.f, 500.f);
	const FName GlobalTag(TEXT("GlobalPP"));
	const FName CaveTag(TEXT("CavePP"));
}

ADemo0520_PostProcessVolume::ADemo0520_PostProcessVolume()
{
	DemoId = TEXT("0520");
	Controls = TEXT("WASD 이동 · 마우스 시점   1 : 동굴 입구 앞으로   2 : 동굴 안으로   3 : 시작 위치\n")
		TEXT("관찰: 레벨 전체에는 Unbound(Infinite Extent) 볼륨이 약한 따뜻한 톤을 주고,\n")
		TEXT("동굴 안 상자 볼륨(Priority 더 높음)은 어둡고 푸르게 만든다. 입구 근처 Blend Radius 4m 에서 서서히 섞인다");
	Showcase = TEXT("Unbound 전역 Post Process Volume + 동굴 구역 볼륨(어둡고 푸른 톤)이 Blend Radius 로 섞이며 전환");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-200.f, 0.f, 100.f));
}

#if WITH_EDITOR
void ADemo0520_PostProcessVolume::BuildLevel(UWorld* World)
{
	// 1) 전역 볼륨: Infinite Extent(Unbound) — 어디에 있든 적용
	APostProcessVolume* Global = World->SpawnActor<APostProcessVolume>(FVector(0.f, 0.f, 300.f), FRotator::ZeroRotator);
	Global->bUnbound = true;
	Global->Priority = 1.f;
	Global->Tags.Add(GlobalTag);
	Global->Settings.bOverride_ColorSaturation = true;
	Global->Settings.ColorSaturation = FVector4(1.1f, 1.1f, 1.1f, 1.f);
	Global->Settings.bOverride_WhiteTemp = true;
	Global->Settings.WhiteTemp = 7000.f;
	Global->SetActorLabel(TEXT("PPV_Global_Unbound"));

	// 2) 동굴 볼륨: 상자 브러시 안에서만, 경계 BlendRadius 만큼 서서히
	if (APostProcessVolume* Cave = Cast<APostProcessVolume>(EffectsKit::SpawnBoxVolume(World, APostProcessVolume::StaticClass(), CaveCenter, CaveSize)))
	{
		Cave->bUnbound = false;
		Cave->Priority = 2.f;          // 겹치는 곳에서는 Priority 가 높은 볼륨 값이 이긴다
		Cave->BlendRadius = CaveBlendRadius;
		Cave->Tags.Add(CaveTag);
		FPostProcessSettings& S = Cave->Settings;
		S.bOverride_AutoExposureBias = true;   S.AutoExposureBias = -1.8f;            // 어둡게 (노출 보정)
		S.bOverride_ColorSaturation = true;    S.ColorSaturation = FVector4(0.5f, 0.5f, 0.5f, 1.f);
		S.bOverride_ColorGain = true;          S.ColorGain = FVector4(0.75f, 0.9f, 1.2f, 1.f); // 푸른빛
		S.bOverride_VignetteIntensity = true;  S.VignetteIntensity = 0.8f;
		Cave->SetActorLabel(TEXT("PPV_Cave"));
	}
}
#endif

void ADemo0520_PostProcessVolume::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	for (TActorIterator<APostProcessVolume> It(World); It; ++It)
	{
		if (It->ActorHasTag(GlobalTag)) GlobalVolume = *It;
		if (It->ActorHasTag(CaveTag)) CaveVolume = *It;
	}

	// 동굴 모양: 양 벽 + 지붕 + 막힌 뒷벽 (입구는 -X 쪽)
	const FLinearColor Rock(0.25f, 0.22f, 0.2f);
	SpawnBlock(World, CaveCenter + FVector(0.f, -CaveSize.Y * 0.5f - 50.f, -CaveSize.Z * 0.5f), FVector(CaveSize.X, 100.f, CaveSize.Z), Rock);
	SpawnBlock(World, CaveCenter + FVector(0.f, CaveSize.Y * 0.5f + 50.f, -CaveSize.Z * 0.5f), FVector(CaveSize.X, 100.f, CaveSize.Z), Rock);
	SpawnBlock(World, CaveCenter + FVector(0.f, 0.f, CaveSize.Z * 0.5f), FVector(CaveSize.X, CaveSize.Y + 200.f, 100.f), Rock);
	SpawnBlock(World, CaveCenter + FVector(CaveSize.X * 0.5f + 50.f, 0.f, -CaveSize.Z * 0.5f), FVector(100.f, CaveSize.Y + 200.f, CaveSize.Z), Rock);
	// 동굴 안 소품 (색이 바뀌는 것을 보기 위한 컬러 상자)
	SpawnBlock(World, CaveCenter + FVector(200.f, -150.f, -CaveSize.Z * 0.5f), FVector(100.f, 100.f, 100.f), Colors::Item);
	SpawnBlock(World, CaveCenter + FVector(300.f, 150.f, -CaveSize.Z * 0.5f), FVector(100.f, 100.f, 150.f), Colors::Enemy);
	SpawnBlock(World, FVector(200.f, -500.f, 0.f), FVector(120.f, 120.f, 120.f), Colors::Ally);
	SpawnLabel(World, TEXT("동굴 — PPV_Cave (Priority 2, Blend Radius 4m)"), CaveCenter + FVector(-CaveSize.X * 0.5f, 0.f, CaveSize.Z * 0.5f + 80.f), Colors::Item, 20.f);
}

void ADemo0520_PostProcessVolume::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ACharacter* Character = GetPlayerCharacter();
	auto Teleport = [Character](const FVector& Loc)
	{
		if (Character) Character->TeleportTo(Loc, FRotator::ZeroRotator);
		if (APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr) PC->SetControlRotation(FRotator::ZeroRotator);
	};
	if (WasKeyPressed(EKeys::One))   Teleport(FVector(CaveCenter.X - CaveSize.X * 0.5f - 150.f, 0.f, 100.f));
	if (WasKeyPressed(EKeys::Two))   Teleport(FVector(CaveCenter.X + 100.f, 0.f, 100.f));
	if (WasKeyPressed(EKeys::Three)) Teleport(FVector(-200.f, 0.f, 100.f));

	// 카메라 위치 기준 동굴 볼륨 가중치: 안이면 1, 경계 밖 BlendRadius 안이면 거리에 따라 줄어든다
	float CaveWeight = 0.f;
	if (CaveVolume && GetPC())
	{
		const FVector Cam = GetPC()->PlayerCameraManager->GetCameraLocation();
		float Distance = 0.f;
		if (CaveVolume->EncompassesPoint(Cam, 0.f, &Distance))
		{
			CaveWeight = 1.f;
		}
		else if (Distance < CaveVolume->BlendRadius)
		{
			CaveWeight = 1.f - Distance / CaveVolume->BlendRadius;
		}
	}
	SetStatus(FString::Printf(TEXT("전역 볼륨(Unbound) %s · 동굴 볼륨 가중치 %.2f %s"),
		GlobalVolume ? TEXT("적용 중") : TEXT("없음"), CaveWeight, CaveWeight >= 1.f ? TEXT("(동굴 안)") : CaveWeight > 0.f ? TEXT("(경계에서 섞이는 중)") : TEXT("(밖)")));
}
