#include "Demo0902_MetaSounds.h"

#include "Camera/CameraActor.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GlossaryAudioKit.h"
#include "GlossaryKit.h"
#include "MetasoundBuilderSubsystem.h"
#include "MetasoundSource.h"
#include "Sound/SoundWave.h"

using namespace GlossaryKit;

namespace
{
	const FName RuntimeBuilderName(TEXT("Glossary0902_RuntimeEngine"));
	constexpr float NeedleMinAngle = -120.f;
	constexpr float NeedleMaxAngle = 120.f;
}

ADemo0902_MetaSounds::ADemo0902_MetaSounds()
{
	DemoId = TEXT("0902");
	Controls = TEXT("↑ 또는 W 를 누르고 있으면 스로틀(RPM 상승), 떼면 RPM 하강 · 1 : 자동 스윕 ON/OFF\n")
		TEXT("관찰: SetFloatParameter(\"RPM\", 0~1) → MetaSound 그래프의 Map Range 가 40~220Hz 로 바꿔 Saw 피치가 오른다\n")
		TEXT("타코미터 바늘·피스톤 속도 = 지금 보낸 RPM 값");
	Showcase = TEXT("MetaSound Source(RPM 입력→Map Range→Saw)를 빌더 API 로 생성, SetFloatParameter 로 엔진 피치 제어 + 타코미터");
	PlayerPawnClass = nullptr;
}

void ADemo0902_MetaSounds::BeginPlay()
{
	Super::BeginPlay();
	GlossaryAudioKit::PrepareAudioDemo();
	UWorld* World = GetWorld();
	Camera = GlossaryAudioKit::SpawnFixedCamera(World, FVector(-1050.f, 0.f, 330.f), FRotator(-6.f, 0.f, 0.f), 70.f);

	// ---- 엔진 블록 + 피스톤 4개 (왼쪽)
	SpawnBlock(World, FVector(300.f, -380.f, 0.f), FVector(220.f, 520.f, 160.f), FLinearColor(0.12f, 0.13f, 0.15f));
	for (int32 i = 0; i < 4; ++i)
	{
		UStaticMeshComponent* Piston = AddShape(this, EShape::Cylinder, FVector(300.f, -560.f + i * 120.f, 220.f), FVector(80.f, 80.f, 120.f), Colors::Neutral);
		SetColor(Piston, Colors::Neutral, 0.3f, 0.9f);
		Pistons.Add(Piston);
	}
	SpawnLabel(World, TEXT("엔진 (피스톤 속도 = RPM)"), FVector(300.f, -380.f, 420.f), Colors::Item, 18.f);

	// ---- 타코미터 (오른쪽): 카메라를 향한 원판 + 눈금 + 바늘
	const FVector DialCenter(300.f, 380.f, 260.f);
	UStaticMeshComponent* Dial = AddShape(this, EShape::Cylinder, DialCenter, FVector(380.f, 380.f, 10.f), Colors::Dark, nullptr, FRotator(90.f, 0.f, 0.f));
	SetColor(Dial, Colors::Dark, 0.4f);
	for (int32 i = 0; i <= 10; ++i)
	{
		const float Angle = FMath::Lerp(NeedleMinAngle, NeedleMaxAngle, i / 10.f);
		const float Rad = FMath::DegreesToRadians(Angle);
		// 바늘 회전과 같은 축(X)으로: 위가 0°, 시계 방향 +
		const FVector Offset(-12.f, FMath::Sin(Rad) * 160.f, FMath::Cos(Rad) * 160.f);
		const FLinearColor TickColor = i >= 8 ? Colors::Enemy : FLinearColor::White;
		UStaticMeshComponent* Tick = AddShape(this, EShape::Cube, DialCenter + Offset, FVector(4.f, 8.f, 28.f), TickColor, nullptr, FRotator(0.f, 0.f, Angle));
		SetColor(Tick, TickColor, 0.5f, 0.f, 2.f);
	}
	NeedlePivot = NewObject<USceneComponent>(this, TEXT("NeedlePivot"));
	NeedlePivot->SetupAttachment(Root);
	NeedlePivot->RegisterComponent();
	NeedlePivot->SetRelativeLocation(DialCenter + FVector(-20.f, 0.f, 0.f));
	Needle = AddShape(this, EShape::Cube, FVector(0.f, 0.f, 70.f), FVector(4.f, 10.f, 150.f), Colors::Accent, NeedlePivot);
	SetColor(Needle, Colors::Accent, 0.5f, 0.f, 3.f);
	SpawnLabel(World, TEXT("RPM 타코미터"), DialCenter + FVector(0.f, 0.f, -230.f), Colors::Item, 18.f);

	// ---- 소리: MS_EngineRPM 에셋 → 없으면 실행 중 MetaSound 빌더 → 그래도 안 되면 웨이브 + 피치
	Engine = NewObject<UAudioComponent>(this, TEXT("EngineAudio"));
	Engine->bAutoActivate = false;
	Engine->SetupAttachment(Root);
	Engine->RegisterComponent();
	Engine->SetVolumeMultiplier(0.35f); // 톱니파는 소리가 커서 줄인다
	Probe = NewObject<UGlossaryAudioProbe>(this);
	Probe->Bind(Engine);

	USoundBase* Sound = LoadAsset<UMetaSoundSource>(GlossaryAudioKit::AssetPath(TEXT("MS_EngineRPM")));
	if (Sound)
	{
		SourceDescription = TEXT("MS_EngineRPM 에셋 (에디터 빌더가 MetaSound 빌더 API 로 생성)");
		bUsingMetaSound = true;
	}
	else
	{
		FString Log;
		if (UMetaSoundSourceBuilder* Builder = GlossaryAudioKit::CreateEngineRPMBuilder(RuntimeBuilderName, Log))
		{
			TScriptInterface<IMetaSoundDocumentInterface> Built = Builder->BuildNewMetaSound(TEXT("MS_EngineRPM_Runtime"));
			Sound = Cast<USoundBase>(Built.GetObject());
		}
		if (Sound)
		{
			SourceDescription = TEXT("실행 중 MetaSound 빌더로 만든 임시 MetaSound Source");
			bUsingMetaSound = true;
		}
		else
		{
			Sound = LoadAsset<USoundWave>(GlossaryAudioKit::AssetPath(TEXT("S_EngineHum_Loop")));
			SourceDescription = FString::Printf(TEXT("대체: S_EngineHum_Loop + SetPitchMultiplier (빌더 실패: %s)"), *Log);
		}
	}
	if (Sound)
	{
		Engine->SetSound(Sound);
		Engine->Play();
	}

	Panel = UGlossaryAudioPanel::Create(this, TEXT("MetaSound 파라미터"));
	if (Panel)
	{
		Panel->AddRow(TEXT("RPM (입력)"), Colors::Accent);
		Panel->AddRow(TEXT("Saw 주파수"), Colors::Cyan);
		Panel->AddRow(TEXT("Envelope"), Colors::Ally);
		Panel->SetFooter(TEXT("Engine->SetFloatParameter(\"RPM\", RPM)\n그래프: RPM → Map Range(0~1 → 40~220Hz) → Saw → Out"));
		Panel->AddToViewport(1);
	}
}

void ADemo0902_MetaSounds::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMetaSoundBuilderSubsystem* Subsystem = UMetaSoundBuilderSubsystem::Get())
	{
		Subsystem->UnregisterBuilder(RuntimeBuilderName);
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0902_MetaSounds::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	GlossaryAudioKit::KeepViewTarget(this, Camera);

	if (WasKeyPressed(EKeys::One))
	{
		bAutoSweep = !bAutoSweep;
	}
	const bool bThrottle = IsKeyDown(EKeys::Up) || IsKeyDown(EKeys::W);
	if (bThrottle)
	{
		bAutoSweep = false;
	}

	if (bAutoSweep)
	{
		// 자동: 0 → 1 → 0 을 6초 주기로 오르내린다
		SweepTime += DeltaSeconds;
		RPM = 0.5f - 0.5f * FMath::Cos(SweepTime * 2.f * PI / 6.f);
	}
	else
	{
		RPM = FMath::Clamp(RPM + (bThrottle ? ThrottleRate : -ThrottleRate) * DeltaSeconds, 0.f, 1.f);
	}

	// 핵심 한 줄: MetaSound 그래프 입력에 값을 보낸다
	if (Engine)
	{
		if (bUsingMetaSound)
		{
			Engine->SetFloatParameter(RPMParameterName, RPM);
		}
		else
		{
			Engine->SetPitchMultiplier(FMath::Lerp(FrequencyRange.X, FrequencyRange.Y, RPM) / 60.f);
		}
	}

	const float Frequency = FMath::Lerp(FrequencyRange.X, FrequencyRange.Y, RPM);

	// 바늘·피스톤
	if (NeedlePivot)
	{
		NeedlePivot->SetRelativeRotation(FRotator(0.f, 0.f, FMath::Lerp(NeedleMinAngle, NeedleMaxAngle, RPM)));
	}
	CrankAngle += DeltaSeconds * Frequency * 0.08f * 2.f * PI;
	for (int32 i = 0; i < Pistons.Num(); ++i)
	{
		const float Offset = FMath::Sin(CrankAngle + i * PI * 0.5f) * 40.f;
		Pistons[i]->SetRelativeLocation(FVector(300.f, -560.f + i * 120.f, 220.f + Offset));
	}

	const float Envelope = Probe ? Probe->GetEnvelope() : 0.f;
	if (Panel)
	{
		Panel->SetRow(0, RPM, FString::Printf(TEXT("%.2f"), RPM));
		Panel->SetRow(1, (Frequency - FrequencyRange.X) / (FrequencyRange.Y - FrequencyRange.X), FString::Printf(TEXT("%.0f Hz"), Frequency));
		Panel->SetRow(2, GlossaryAudioKit::AmplitudeToBar(Envelope), FString::Printf(TEXT("%.3f"), Envelope));
	}
	SetStatus(FString::Printf(TEXT("SetFloatParameter(\"%s\", %.2f) → %.0f Hz · %s\n소리: %s · 재생 %s"),
		*RPMParameterName.ToString(), RPM, Frequency, bAutoSweep ? TEXT("자동 스윕") : (bThrottle ? TEXT("스로틀 ON") : TEXT("스로틀 OFF")),
		*SourceDescription, Engine && Engine->IsPlaying() ? TEXT("중") : TEXT("안 됨")));
}
