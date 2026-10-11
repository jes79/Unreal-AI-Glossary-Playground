#include "Demo0903_Attenuation.h"

#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryAudioKit.h"
#include "GlossaryKit.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWave.h"

using namespace GlossaryKit;

ADemo0903_Attenuation::ADemo0903_Attenuation()
{
	DemoId = TEXT("0903");
	Controls = TEXT("WASD 이동 · 마우스 시점 · C : Attenuation ON/OFF\n")
		TEXT("관찰: 주황 링(Inner 200) 안은 최대 볼륨, 파란 링(200+Falloff 1500)까지 줄어 밖에서는 0\n")
		TEXT("카메라를 돌리면 좌/우 방향(Spatialization)이 바뀐다");
	Showcase = TEXT("모닥불 소리 ATT_Campfire(Inner 200·Falloff 1500·Spatialization), 감쇠 링·거리·계산/측정 볼륨·좌우 표시");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-100.f, 500.f, 100.f));
}

void ADemo0903_Attenuation::BeginPlay()
{
	Super::BeginPlay();
	GlossaryAudioKit::PrepareAudioDemo();
	UWorld* World = GetWorld();

	// 모닥불: 통나무 + 흔들리는 불꽃
	for (int32 i = 0; i < 4; ++i)
	{
		SpawnShape(World, EShape::Cylinder, FireLocation + FVector(0.f, 0.f, 15.f), FVector(25.f, 25.f, 140.f),
			FLinearColor(0.25f, 0.12f, 0.05f), FRotator(90.f, i * 45.f, 0.f));
	}
	Flame = AddShape(this, EShape::Cone, FireLocation + FVector(0.f, 0.f, 70.f), FVector(80.f, 80.f, 110.f), Colors::Accent);
	SetColor(Flame, Colors::Accent, 0.5f, 0.f, 8.f);
	Flame->SetCastShadow(false);

	Attenuation = LoadAsset<USoundAttenuation>(GlossaryAudioKit::AssetPath(TEXT("ATT_Campfire")));
	const float Inner = Attenuation ? Attenuation->Attenuation.AttenuationShapeExtents.X : 200.f;
	const float Falloff = Attenuation ? Attenuation->Attenuation.FalloffDistance : 1500.f;

	// 감쇠 구간 링: Inner Radius / Inner + Falloff
	GlossaryAudioKit::SpawnRing(World, FireLocation + FVector(0.f, 0.f, 1.f), Inner, Colors::Accent, 32, 10.f, 3.f);
	GlossaryAudioKit::SpawnRing(World, FireLocation + FVector(0.f, 0.f, 1.f), Inner + Falloff, Colors::Cyan, 96, 14.f, 2.f);
	GlossaryAudioKit::SpawnRing(World, FireLocation + FVector(0.f, 0.f, 1.f), Inner + Falloff * 0.5f, Colors::Cyan * 0.5f, 72, 6.f, 0.5f);
	SpawnLabel(World, FString::Printf(TEXT("Inner Radius %.0f (최대 볼륨)"), Inner), FireLocation + FVector(0.f, 0.f, 200.f), Colors::Accent, 16.f);
	SpawnLabel(World, FString::Printf(TEXT("Falloff 끝 %.0fcm (볼륨 0)"), Inner + Falloff), FireLocation + FVector(-(Inner + Falloff), 0.f, 60.f), Colors::Cyan, 18.f);
	SpawnLabel(World, TEXT("볼륨 50%"), FireLocation + FVector(-(Inner + Falloff * 0.5f), 0.f, 40.f), FLinearColor::White, 15.f);

	Fire = NewObject<UAudioComponent>(this, TEXT("CampfireAudio"));
	Fire->bAutoActivate = false;
	Fire->SetupAttachment(Root);
	Fire->RegisterComponent();
	Fire->SetWorldLocation(FireLocation + FVector(0.f, 0.f, 60.f));
	Fire->SetSound(LoadAsset<USoundWave>(GlossaryAudioKit::AssetPath(TEXT("S_Campfire_Loop"))));
	Probe = NewObject<UGlossaryAudioProbe>(this);
	Probe->Bind(Fire);
	ApplyAttenuation();
	Fire->Play();

	Panel = UGlossaryAudioPanel::Create(this, TEXT("모닥불 소리"));
	if (Panel)
	{
		Panel->AddRow(TEXT("거리"), Colors::Neutral);
		Panel->AddRow(TEXT("감쇠 볼륨(계산)"), Colors::Accent);
		Panel->AddRow(TEXT("Envelope(측정)"), Colors::Ally);
		Panel->AddRow(TEXT("좌/우 방향"), Colors::Cyan);
		Panel->SetFooter(TEXT("계산 = FSoundAttenuationSettings::Evaluate(리스너 위치)\n측정 = OnAudioSingleEnvelopeValue"));
		Panel->AddToViewport(1);
	}
}

void ADemo0903_Attenuation::ApplyAttenuation()
{
	if (!Fire)
	{
		return;
	}
	// 핵심: 소리에 Attenuation 에셋을 지정하면 거리 감쇠 + 방향(Spatialization)이 함께 적용된다
	Fire->SetAttenuationSettings(bUseAttenuation ? Attenuation.Get() : nullptr);
	if (Fire->IsPlaying())
	{
		Fire->Play(); // 설정은 재생을 시작할 때 읽히므로 다시 재생
	}
}

void ADemo0903_Attenuation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (WasKeyPressed(EKeys::C))
	{
		bUseAttenuation = !bUseAttenuation;
		ApplyAttenuation();
	}

	FlameTime += DeltaSeconds;
	if (Flame)
	{
		const float Flicker = 1.f + 0.12f * FMath::Sin(FlameTime * 17.f) + 0.08f * FMath::Sin(FlameTime * 29.f);
		Flame->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.1f * Flicker));
	}

	FVector ListenerLoc;
	FRotator ListenerRot;
	if (!GlossaryAudioKit::GetListener(this, ListenerLoc, ListenerRot) || !Fire)
	{
		return;
	}
	const FVector SoundLoc = Fire->GetComponentLocation();
	const float Distance = FVector::Dist2D(ListenerLoc, SoundLoc);

	// Attenuation 이 계산하는 볼륨 배율 (엔진이 쓰는 것과 같은 함수)
	float Computed = 1.f;
	if (bUseAttenuation && Attenuation)
	{
		Computed = Attenuation->Attenuation.Evaluate(Fire->GetComponentTransform(), ListenerLoc);
	}

	// 좌우: 리스너 오른쪽 방향과 소리 방향의 내적 (-1 왼쪽 ~ +1 오른쪽)
	const FVector ToSound = (SoundLoc - ListenerLoc).GetSafeNormal2D();
	const FVector Right = FRotationMatrix(ListenerRot).GetUnitAxis(EAxis::Y).GetSafeNormal2D();
	const float Pan = (bUseAttenuation && Distance > 1.f) ? FVector::DotProduct(ToSound, Right) : 0.f;
	const float Envelope = Probe ? Probe->GetEnvelope() : 0.f;

	if (Panel)
	{
		const float Inner = Attenuation ? Attenuation->Attenuation.AttenuationShapeExtents.X : 200.f;
		const float Falloff = Attenuation ? Attenuation->Attenuation.FalloffDistance : 1500.f;
		Panel->SetRow(0, Distance / (Inner + Falloff + 500.f), FString::Printf(TEXT("%.0f cm"), Distance));
		Panel->SetRow(1, Computed, FString::Printf(TEXT("%.0f %%"), Computed * 100.f));
		Panel->SetRow(2, GlossaryAudioKit::AmplitudeToBar(Envelope), FString::Printf(TEXT("%.3f"), Envelope));
		Panel->SetRow(3, 0.5f + Pan * 0.5f, FMath::Abs(Pan) < 0.15f ? TEXT("가운데") : (Pan < 0.f ? TEXT("왼쪽") : TEXT("오른쪽")));
	}

	const TCHAR* Zone = !bUseAttenuation ? TEXT("Attenuation OFF → 거리와 상관없이 같은 크기, 방향감 없음")
		: Computed >= 0.999f ? TEXT("Inner Radius 안 → 최대 볼륨")
		: Computed <= 0.001f ? TEXT("Falloff 밖 → 들리지 않음")
		: TEXT("Falloff 구간 → 멀어질수록 작아짐");
	SetStatus(FString::Printf(TEXT("Attenuation %s · 거리 %.0fcm · 볼륨 %.0f%% · %s\n%s"),
		bUseAttenuation ? TEXT("ON (ATT_Campfire)") : TEXT("OFF"), Distance, Computed * 100.f,
		FMath::Abs(Pan) < 0.15f ? TEXT("정면/뒤") : (Pan < 0.f ? TEXT("왼쪽에서 들림") : TEXT("오른쪽에서 들림")), Zone));
}
