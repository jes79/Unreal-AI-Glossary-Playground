#include "Demo0522_Vignette.h"

#include "EffectsDemoKit.h"
#include "Engine/PostProcessVolume.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0522_Vignette::ADemo0522_Vignette()
{
	DemoId = TEXT("0522");
	Controls = TEXT("WASD 이동   Space : 피해 -10%   R : 회복 +10%   1 : 100%   2 : 50%   3 : 25%\n")
		TEXT("관찰: 체력이 30% 이하가 되면 Vignette Intensity 가 0.2 → 0.8 로 부드럽게 올라가 화면 가장자리가 어두워진다.\n")
		TEXT("위급할수록 시야가 좁아지는 느낌 (체력 회복하면 다시 0.2)");
	Showcase = TEXT("체력 30% 이하에서 Vignette Intensity 0.8 로 화면 가장자리 어둡게, 피해/회복 키로 비교");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-300.f, 0.f, 100.f));
}

void ADemo0522_Vignette::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	// 가장자리 변화가 잘 보이게 밝은 색 기둥들을 화면 전체에 깐다
	for (int32 i = 0; i < 9; ++i)
	{
		const FLinearColor Color = i % 3 == 0 ? Colors::Item : i % 3 == 1 ? Colors::Cyan : Colors::Ally;
		SpawnBlock(World, FVector(500.f + (i / 3) * 300.f, -700.f + (i % 3) * 700.f, 0.f), FVector(150.f, 150.f, 300.f + (i % 4) * 80.f), Color);
	}
	Volume = EffectsKit::SpawnUnboundPostProcess(World);
	Volume->Settings.bOverride_VignetteIntensity = true;
	Volume->Settings.VignetteIntensity = NormalVignette;
}

void ADemo0522_Vignette::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	if (WasKeyPressed(EKeys::SpaceBar)) Health = FMath::Max(0.f, Health - 0.1f);
	if (WasKeyPressed(EKeys::R))        Health = FMath::Min(1.f, Health + 0.1f);
	if (WasKeyPressed(EKeys::One))      Health = 1.f;
	if (WasKeyPressed(EKeys::Two))      Health = 0.5f;
	if (WasKeyPressed(EKeys::Three))    Health = 0.25f;

	// 핵심: 체력 30% 이하 → 0.8, 아니면 0.2. 갑자기 바뀌지 않게 보간
	const float Target = Health <= LowHealthThreshold ? LowHealthVignette : NormalVignette;
	CurrentVignette = FMath::FInterpTo(CurrentVignette, Target, DeltaSeconds, 6.f);
	if (Volume)
	{
		Volume->Settings.VignetteIntensity = CurrentVignette;
	}
	const int32 Bars = FMath::RoundToInt(Health * 20.f);
	SetStatus(FString::Printf(TEXT("체력 %3.0f%% [%s%s] · Vignette Intensity %.2f%s"), Health * 100.f,
		*FString::ChrN(Bars, TEXT('|')), *FString::ChrN(20 - Bars, TEXT('.')), CurrentVignette,
		Health <= LowHealthThreshold ? TEXT("  ← 위급 (30% 이하)") : TEXT("")));
}
