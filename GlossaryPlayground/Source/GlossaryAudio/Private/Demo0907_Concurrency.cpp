#include "Demo0907_Concurrency.h"

#include "Camera/CameraActor.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GlossaryAudioKit.h"
#include "GlossaryKit.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundConcurrency.h"
#include "Sound/SoundWave.h"

using namespace GlossaryKit;

namespace
{
	constexpr int32 SlotCount = 16;
	const FVector MuzzleLocation(300.f, -500.f, 180.f);
	float SlotY(int32 Index) { return -330.f + Index * 64.f; }
}

ADemo0907_Concurrency::ADemo0907_Concurrency()
{
	DemoId = TEXT("0907");
	Controls = TEXT("자동 연사 0.1초 간격 · 2 : 자동 연사 ON/OFF · F : 한 발\n")
		TEXT("C : Sound Concurrency(Max 4, Stop Oldest) ON/OFF\n")
		TEXT("관찰: ON 이면 주황 슬롯이 4칸을 넘지 않고, 새 총소리가 올 때마다 가장 오래된 소리가 끊긴다(끊긴 수 증가)");
	Showcase = TEXT("총소리 0.1초 연사, SCon_Gunshot(Max 4, Stop Oldest) ON/OFF — 동시 재생 보이스 슬롯·끊긴 수 표시");
	PlayerPawnClass = nullptr;
}

void ADemo0907_Concurrency::BeginPlay()
{
	Super::BeginPlay();
	GlossaryAudioKit::PrepareAudioDemo();
	UWorld* World = GetWorld();
	Camera = GlossaryAudioKit::SpawnFixedCamera(World, FVector(-1000.f, 0.f, 380.f), FRotator(-12.f, 0.f, 0.f), 72.f);

	Gunshot = LoadAsset<USoundWave>(GlossaryAudioKit::AssetPath(TEXT("S_Gunshot")));
	Concurrency = LoadAsset<USoundConcurrency>(GlossaryAudioKit::AssetPath(TEXT("SCon_Gunshot")));

	// 총 (받침대 + 총몸 + 총열)
	SpawnBlock(World, FVector(150.f, -500.f, 0.f), FVector(80.f, 80.f, 140.f), Colors::Dark);
	AddShape(this, EShape::Cube, FVector(150.f, -500.f, 180.f), FVector(220.f, 60.f, 70.f), Colors::Wall);
	AddShape(this, EShape::Cylinder, FVector(270.f, -500.f, 180.f), FVector(25.f, 25.f, 120.f), Colors::Dark, nullptr, FRotator(90.f, 0.f, 0.f));
	MuzzleFlash = AddShape(this, EShape::Sphere, MuzzleLocation + FVector(30.f, 0.f, 0.f), FVector(70.f, 70.f, 70.f), Colors::Item);
	SetColor(MuzzleFlash, Colors::Item, 0.5f, 0.f, 20.f);
	MuzzleFlash->SetCastShadow(false);
	MuzzleFlash->SetVisibility(false);

	// 보이스 슬롯 16칸 (재생 중인 총소리 수)
	for (int32 i = 0; i < SlotCount; ++i)
	{
		UStaticMeshComponent* Slot = AddShape(this, EShape::Cube, FVector(600.f, SlotY(i), 120.f), FVector(50.f, 50.f, 160.f), Colors::Neutral);
		SlotMats.Add(SetColor(Slot, FLinearColor(0.15f, 0.15f, 0.17f), 0.5f, 0.f, 0.f));
	}
	// Max Count 4 경계선
	const float LimitY = (SlotY(3) + SlotY(4)) * 0.5f;
	UStaticMeshComponent* Limit = AddShape(this, EShape::Cube, FVector(590.f, LimitY, 120.f), FVector(10.f, 8.f, 260.f), Colors::Enemy);
	SetColor(Limit, Colors::Enemy, 0.5f, 0.f, 5.f);
	SpawnLabel(World, TEXT("Max Count 4"), FVector(590.f, LimitY, 290.f), Colors::Enemy, 17.f);
	SpawnLabel(World, TEXT("동시에 재생 중인 총소리 (보이스)"), FVector(600.f, SlotY(8), 330.f), Colors::Item, 19.f);

	Panel = UGlossaryAudioPanel::Create(this, TEXT("Sound Concurrency"));
	if (Panel)
	{
		Panel->AddRow(TEXT("재생 중 보이스"), Colors::Accent);
		Panel->AddRow(TEXT("최대 동시 재생"), Colors::Purple);
		Panel->AddRow(TEXT("Stop Oldest 끊김"), Colors::Enemy);
		Panel->SetFooter(TEXT("AudioComponent->ConcurrencySet.Add(SCon_Gunshot)\nMaxCount 4 · ResolutionRule StopOldest\n(요청 후 50ms 지난 보이스만 셈 — 오디오 스레드 반영 시간)"));
		Panel->AddToViewport(1);
	}
}

void ADemo0907_Concurrency::Fire()
{
	if (!Gunshot)
	{
		return;
	}
	UAudioComponent* Audio = NewObject<UAudioComponent>(this);
	Audio->bAutoActivate = false;
	Audio->SetupAttachment(Root);
	Audio->SetSound(Gunshot);
	Audio->SetVolumeMultiplier(0.5f);
	// 핵심: 컴포넌트(또는 사운드 에셋)의 Concurrency 설정. 같은 SCon_Gunshot 그룹끼리 동시 재생 수를 센다
	if (bUseConcurrency && Concurrency)
	{
		Audio->ConcurrencySet.Add(Concurrency);
	}
	Audio->RegisterComponent();
	Audio->SetWorldLocation(MuzzleLocation);
	Audio->Play();

	FShot Shot;
	Shot.Audio = Audio;
	Shot.StartTime = FPlatformTime::Seconds();
	Shots.Add(Shot);
	++ShotCount;
	FlashTime = 0.05f;
}

void ADemo0907_Concurrency::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	GlossaryAudioKit::KeepViewTarget(this, Camera);

	if (WasKeyPressed(EKeys::C))
	{
		bUseConcurrency = !bUseConcurrency;
		CutCount = 0;
		PeakVoices = 0;
	}
	if (WasKeyPressed(EKeys::Two))
	{
		bAutoFire = !bAutoFire;
	}
	if (WasKeyPressed(EKeys::F))
	{
		Fire();
	}
	if (bAutoFire)
	{
		FireTimer += DeltaSeconds;
		while (FireTimer >= FireInterval)
		{
			FireTimer -= FireInterval;
			Fire();
		}
	}

	FlashTime -= DeltaSeconds;
	if (MuzzleFlash)
	{
		MuzzleFlash->SetVisibility(FlashTime > 0.f);
	}

	// 끝난 소리 정리: 길이(1.2초)보다 훨씬 일찍 끝났으면 Concurrency 가 끊은 것
	const double Now = FPlatformTime::Seconds();
	const float Duration = Gunshot ? Gunshot->Duration : 1.2f;
	int32 Playing = 0;
	for (int32 i = Shots.Num() - 1; i >= 0; --i)
	{
		UAudioComponent* Audio = Shots[i].Audio.Get();
		if (Audio && Audio->IsPlaying())
		{
			// 방금(50ms 안) 요청한 소리는 아직 오디오 스레드가 처리 전이라 세지 않는다
			// (이 순간엔 Stop Oldest 로 꺼질 소리의 정지 알림도 아직 안 왔다)
			if (Now - Shots[i].StartTime >= 0.05)
			{
				++Playing;
			}
			continue;
		}
		if (Now - Shots[i].StartTime < Duration * 0.85f)
		{
			++CutCount;
		}
		if (Audio)
		{
			Audio->DestroyComponent();
		}
		Shots.RemoveAt(i);
	}
	PeakVoices = FMath::Max(PeakVoices, Playing);

	for (int32 i = 0; i < SlotMats.Num(); ++i)
	{
		const bool bOn = i < Playing;
		const FLinearColor Color = bOn ? (i >= 4 ? Colors::Enemy : Colors::Accent) : FLinearColor(0.15f, 0.15f, 0.17f);
		SlotMats[i]->SetVectorParameterValue(TEXT("Color"), Color);
		SlotMats[i]->SetScalarParameterValue(TEXT("Emissive"), bOn ? 3.f : 0.f);
	}
	if (Panel)
	{
		Panel->SetRow(0, Playing / static_cast<float>(SlotCount), FString::Printf(TEXT("%d"), Playing));
		Panel->SetRow(1, PeakVoices / static_cast<float>(SlotCount), FString::Printf(TEXT("%d"), PeakVoices));
		Panel->SetRow(2, FMath::Min(1.f, CutCount / 100.f), FString::Printf(TEXT("%d"), CutCount));
	}
	SetStatus(FString::Printf(TEXT("Concurrency %s · 발사 %d · 재생 중 보이스 %d (최대 %d) · 끊긴 소리 %d\n%s"),
		bUseConcurrency ? TEXT("ON (SCon_Gunshot: Max 4, Stop Oldest)") : TEXT("OFF"), ShotCount, Playing, PeakVoices, CutCount,
		bUseConcurrency ? TEXT("5번째 총소리가 오면 가장 오래된 것을 끈다 → 항상 4개 이하") : TEXT("제한 없음 → 꼬리가 겹쳐 10개 넘게 쌓인다")));
}
