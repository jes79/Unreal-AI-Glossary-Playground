#include "Demo0819_DayNight.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "D08_Common.h"
#include "GlossaryEnvironment.h"
#include "GlossaryKit.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace GlossaryKit;

ADemo0819_DayNight::ADemo0819_DayNight()
{
	DemoId = TEXT("0819");
	Controls = TEXT("T : 3시간 앞으로 · P : 일시정지 · F : 10배속 ON/OFF\n")
		TEXT("관찰: 해(Directional Light)만 돌리는데 Sky Atmosphere 가 하늘색·노을·밤하늘을 알아서 바꾸고, 밤이면 가로등이 켜진다");
	Showcase = TEXT("Directional Light 를 1분에 하루 회전, Sky Atmosphere 가 낮·노을·밤하늘로 자동 변화 + 밤에 가로등");
	// 밤에도 무엇이 보이도록 자동 노출을 켠다
	Environment.bFixedExposure = false;
}

void ADemo0819_DayNight::BeginPlay()
{
	Super::BeginPlay();
	Hour = StartHour;

	// 작은 마을: 집 몇 채 + 가로등
	for (int32 i = 0; i < 4; ++i)
	{
		const FVector Base(400.f + (i % 2) * 700.f, (i < 2 ? -1.f : 1.f) * 600.f, 0.f);
		SpawnBlock(GetWorld(), Base, FVector(300.f, 300.f, 260.f), FLinearColor(0.75f, 0.68f, 0.55f));
		SpawnShape(GetWorld(), EShape::Cone, Base + FVector(0.f, 0.f, 330.f), FVector(380.f, 380.f, 140.f), FLinearColor(0.55f, 0.15f, 0.1f));
	}
	for (int32 i = 0; i < 5; ++i)
	{
		const FVector Base(-100.f + i * 400.f, 0.f, 0.f);
		SpawnBlock(GetWorld(), Base, FVector(16.f, 16.f, 320.f), Colors::Dark);
		UStaticMeshComponent* Bulb = AddShape(this, EShape::Sphere, Base + FVector(0.f, 0.f, 335.f), FVector(40.f), FLinearColor(1.f, 0.8f, 0.4f));
		Bulb->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BulbMIDs.Add(SetColor(Bulb, FLinearColor(1.f, 0.8f, 0.4f)));
		UPointLightComponent* Lamp = NewObject<UPointLightComponent>(this);
		Lamp->SetupAttachment(Root);
		Lamp->SetWorldLocation(Base + FVector(0.f, 0.f, 320.f));
		Lamp->SetLightColor(FLinearColor(1.f, 0.75f, 0.45f));
		Lamp->SetAttenuationRadius(900.f);
		Lamp->SetIntensityUnits(ELightUnits::Candelas);
		Lamp->SetIntensity(0.f);
		Lamp->RegisterComponent();
		Lamps.Add(Lamp);
	}
	Panel = UD08_SidePanel::Create(GetPC(), TEXT("시각"), UD08_SidePanel::EPlace::TopRight, 380.f);
	ApplyTime();
}

FString ADemo0819_DayNight::PhaseName() const
{
	if (Hour >= 5.f && Hour < 7.f)   { return TEXT("새벽 · 해 뜸"); }
	if (Hour >= 7.f && Hour < 17.f)  { return TEXT("낮"); }
	if (Hour >= 17.f && Hour < 19.f) { return TEXT("노을 · 해 짐"); }
	return TEXT("밤");
}

void ADemo0819_DayNight::ApplyTime()
{
	AGlossaryEnvironment* Env = AGlossaryEnvironment::Find(this);
	if (!Env || !Env->GetSun())
	{
		return;
	}
	// 6시 = 지평선(0°), 12시 = 머리 위(90°), 18시 = 반대편 지평선(180°), 0시 = 발밑(270°)
	const float Elevation = (Hour - 6.f) / 24.f * 360.f;
	// 빛의 방향은 아래를 향해야 하므로 Pitch = -고도
	Env->GetSun()->SetWorldRotation(FRotator(-Elevation, SunYaw, 0.f));

	// 해가 지평선 아래로 내려가면 밝기를 줄인다 (대기는 각도만으로 이미 붉어지고 어두워진다)
	const float SinElev = FMath::Sin(FMath::DegreesToRadians(Elevation));
	const float DayFactor = FMath::Clamp(SinElev * 4.f + 0.2f, 0.f, 1.f);
	Env->GetSun()->SetIntensity(NoonIntensity * DayFactor);

	// 밤에는 가로등 ON
	const float LampFactor = FMath::Clamp(1.f - (SinElev + 0.05f) * 8.f, 0.f, 1.f);
	for (UPointLightComponent* Lamp : Lamps)
	{
		Lamp->SetIntensity(LampFactor * 800.f);
	}
	for (UMaterialInstanceDynamic* MID : BulbMIDs)
	{
		MID->SetScalarParameterValue(TEXT("Emissive"), LampFactor * 20.f);
	}
}

void ADemo0819_DayNight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::T)) { Hour += 3.f; }
	if (WasKeyPressed(EKeys::P)) { bPaused = !bPaused; }
	if (WasKeyPressed(EKeys::F)) { SpeedScale = SpeedScale > 1.f ? 1.f : 10.f; }

	if (!bPaused)
	{
		// 핵심: DayLengthSeconds(60초) 동안 24시간이 흐른다
		Hour += DeltaSeconds * SpeedScale * 24.f / DayLengthSeconds;
	}
	Hour = FMath::Fmod(Hour, 24.f);
	ApplyTime();

	const int32 H = FMath::FloorToInt(Hour);
	const int32 M = FMath::FloorToInt((Hour - H) * 60.f);
	if (Panel)
	{
		Panel->SetTitle(FString::Printf(TEXT("%02d:%02d  %s"), H, M, *PhaseName()));
		Panel->SetProgress(Hour / 24.f, FLinearColor(1.f, 0.7f, 0.2f));
		Panel->SetBody(FString::Printf(TEXT("하루 = %.0f초 · 속도 ×%.0f%s\n해 Pitch %.0f°"),
			DayLengthSeconds, SpeedScale, bPaused ? TEXT(" · 일시정지") : TEXT(""), -((Hour - 6.f) / 24.f * 360.f)));
	}
	SetStatus(FString::Printf(TEXT("게임 시각 %02d:%02d (%s) · 실제 1초 = 게임 %.0f분"), H, M, *PhaseName(), 24.f * 60.f / DayLengthSeconds * SpeedScale));
}
