#include "Demo0904_SoundClassMix.h"

#include "Blueprint/WidgetTree.h"
#include "Camera/CameraActor.h"
#include "Components/AudioComponent.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GlossaryAudioKit.h"
#include "GlossaryKit.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundWave.h"

using namespace GlossaryKit;

namespace
{
	const TCHAR* ClassNames[3] = { TEXT("Music"), TEXT("SFX"), TEXT("Voice") };
	const TCHAR* ClassAssets[3] = { TEXT("SC_Music"), TEXT("SC_SFX"), TEXT("SC_Voice") };
	const TCHAR* SourceAssets[3] = { TEXT("S_BGM_Calm_Loop"), TEXT("S_Chime_Loop"), TEXT("S_Voice_Loop") };
	const FLinearColor ClassColors[3] = { Colors::Purple, Colors::Accent, Colors::Ally };
	const float SpeakerY[3] = { -450.f, 0.f, 450.f };
}

// ---------------------------------------------------------------- 옵션 위젯

void UD0904VolumeOptions::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UBorder* Frame = MakeBorder(FLinearColor(0.03f, 0.04f, 0.07f, 0.9f), FMargin(16.f, 12.f));
	AddToCanvas(Canvas, Frame, FAnchors(1.f, 1.f), FVector2D(-20.f, -20.f), FVector2D::ZeroVector, FVector2D(1.f, 1.f));
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Frame->SetContent(Box);
	Box->AddChildToVerticalBox(MakeText(TEXT("옵션 · 사운드"), 18.f, FLinearColor(1.f, 0.85f, 0.3f), true))->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));

	for (int32 i = 0; i < 3; ++i)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Box->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 3.f));

		USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		LabelBox->SetWidthOverride(90.f);
		UTextBlock* Label = MakeText(ClassNames[i], 16.f, ClassColors[i], true);
		LabelBox->SetContent(Label);
		Row->AddChildToHorizontalBox(LabelBox)->SetVerticalAlignment(VAlign_Center);
		Labels.Add(Label);

		USizeBox* SliderBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SliderBox->SetWidthOverride(260.f);
		SliderBox->SetHeightOverride(28.f);
		USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
		Slider->SetMinValue(0.f);
		Slider->SetMaxValue(1.f);
		Slider->SetStepSize(0.05f);
		Slider->SetValue(1.f);
		Slider->SetSliderBarColor(FLinearColor(0.3f, 0.3f, 0.35f));
		Slider->SetSliderHandleColor(ClassColors[i]);
		SliderBox->SetContent(Slider);
		Row->AddChildToHorizontalBox(SliderBox)->SetVerticalAlignment(VAlign_Center);
		Sliders.Add(Slider);

		USizeBox* ValueBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		ValueBox->SetWidthOverride(70.f);
		UTextBlock* Value = MakeText(TEXT("100%"), 16.f);
		ValueBox->SetContent(Value);
		UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(ValueBox);
		ValueSlot->SetVerticalAlignment(VAlign_Center);
		ValueSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
		ValueTexts.Add(Value);
	}
	Sliders[0]->OnValueChanged.AddDynamic(this, &UD0904VolumeOptions::OnMusicChanged);
	Sliders[1]->OnValueChanged.AddDynamic(this, &UD0904VolumeOptions::OnSFXChanged);
	Sliders[2]->OnValueChanged.AddDynamic(this, &UD0904VolumeOptions::OnVoiceChanged);
	Box->AddChildToVerticalBox(MakeText(TEXT("슬라이더 → SetSoundMixClassOverride(SMix_Options, 클래스, 볼륨)"), 13.f, FLinearColor(0.7f, 0.75f, 0.8f)))
		->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
}

void UD0904VolumeOptions::SetValue(int32 Index, float Value, bool bSelected)
{
	if (Sliders.IsValidIndex(Index))
	{
		// USlider::SetValue 는 OnValueChanged 를 다시 방송하므로 되먹임을 막는다 (키 입력 → 슬라이더 표시만 맞춤)
		TGuardValue<bool> Guard(bSyncingFromCode, true);
		if (!FMath::IsNearlyEqual(Sliders[Index]->GetValue(), Value))
		{
			Sliders[Index]->SetValue(Value);
		}
		ValueTexts[Index]->SetText(FText::FromString(FString::Printf(TEXT("%.0f%%"), Value * 100.f)));
		Labels[Index]->SetText(FText::FromString(FString::Printf(TEXT("%s%s"), bSelected ? TEXT("▶ ") : TEXT(""), ClassNames[Index])));
	}
}

void UD0904VolumeOptions::OnMusicChanged(float Value) { if (!bSyncingFromCode && Demo.IsValid()) Demo->SetClassVolume(0, Value); }
void UD0904VolumeOptions::OnSFXChanged(float Value)   { if (!bSyncingFromCode && Demo.IsValid()) Demo->SetClassVolume(1, Value); }
void UD0904VolumeOptions::OnVoiceChanged(float Value) { if (!bSyncingFromCode && Demo.IsValid()) Demo->SetClassVolume(2, Value); }

// ---------------------------------------------------------------- 데모

ADemo0904_SoundClassMix::ADemo0904_SoundClassMix()
{
	DemoId = TEXT("0904");
	Controls = TEXT("오른쪽 아래 슬라이더를 마우스로 끌기 · 또는 1/2/3 으로 Music/SFX/Voice 고르고 [ / ] 또는 ↓/↑ 로 -/+ 10%, M : 0%↔100%\n")
		TEXT("관찰: 슬라이더가 Sound Class 그룹 볼륨을 바꾸면 그 그룹 스피커만 작아진다(Envelope 막대도 같이)");
	Showcase = TEXT("Music·SFX·Voice Sound Class + SMix_Options, 옵션 슬라이더 → SetSoundMixClassOverride, 그룹별 스피커·Envelope");
	PlayerPawnClass = nullptr;
	bShowMouseCursor = true;
}

void ADemo0904_SoundClassMix::BeginPlay()
{
	Super::BeginPlay();
	GlossaryAudioKit::PrepareAudioDemo();
	UWorld* World = GetWorld();
	Camera = GlossaryAudioKit::SpawnFixedCamera(World, FVector(-700.f, 0.f, 300.f), FRotator(-12.f, 0.f, 0.f), 70.f);

	OptionsMix = LoadAsset<USoundMix>(GlossaryAudioKit::AssetPath(TEXT("SMix_Options")));
	for (int32 i = 0; i < 3; ++i)
	{
		Classes.Add(LoadAsset<USoundClass>(GlossaryAudioKit::AssetPath(ClassAssets[i])));

		// 스피커: 받침 + 콘(원판) + 볼륨 막대
		const FVector Base(400.f, SpeakerY[i], 0.f);
		SpawnBlock(World, Base, FVector(160.f, 200.f, 60.f), Colors::Dark);
		UStaticMeshComponent* Speaker = AddShape(this, EShape::Cylinder, Base + FVector(0.f, 0.f, 200.f), FVector(200.f, 200.f, 40.f), ClassColors[i], nullptr, FRotator(90.f, 0.f, 0.f));
		SetColor(Speaker, ClassColors[i], 0.4f, 0.f, 1.5f);
		Speakers.Add(Speaker);
		UStaticMeshComponent* Bar = AddShape(this, EShape::Cube, Base + FVector(-40.f, 140.f, 60.f), FVector(20.f, 30.f, 200.f), ClassColors[i]);
		SetColor(Bar, ClassColors[i], 0.4f, 0.f, 3.f);
		VolumeBars.Add(Bar);
		SpawnLabel(World, FString::Printf(TEXT("%s (%s)"), ClassNames[i], SourceAssets[i]), Base + FVector(-120.f, 0.f, 20.f), ClassColors[i], 17.f);

		// 소스: 각 웨이브는 빌더에서 SoundClassObject 가 지정돼 있다 (BGM=Music, 차임=SFX, 목소리=Voice)
		UAudioComponent* Source = NewObject<UAudioComponent>(this, *FString::Printf(TEXT("Source_%s"), ClassNames[i]));
		Source->bAutoActivate = false;
		Source->SetupAttachment(Root);
		Source->RegisterComponent();
		Source->SetSound(LoadAsset<USoundWave>(GlossaryAudioKit::AssetPath(SourceAssets[i])));
		UGlossaryAudioProbe* Probe = NewObject<UGlossaryAudioProbe>(this);
		Probe->Bind(Source);
		Source->Play();
		Sources.Add(Source);
		Probes.Add(Probe);
	}

	// 옵션 믹스를 활성화: 이후 SetSoundMixClassOverride 로 클래스별 볼륨을 덮어쓴다
	if (OptionsMix)
	{
		UGameplayStatics::PushSoundMixModifier(this, OptionsMix);
	}

	if (APlayerController* PC = GetPC())
	{
		Options = CreateWidget<UD0904VolumeOptions>(PC, UD0904VolumeOptions::StaticClass());
		Options->Demo = this;
		Options->AddToViewport(2);
	}
	Panel = UGlossaryAudioPanel::Create(this, TEXT("그룹별 출력 (Envelope)"));
	if (Panel)
	{
		for (int32 i = 0; i < 3; ++i)
		{
			Panel->AddRow(ClassNames[i], ClassColors[i]);
		}
		Panel->SetFooter(TEXT("실제 재생 중인 소리의 Envelope (Sound Class 볼륨이 곱해진 출력)"));
		Panel->AddToViewport(1);
	}
	for (int32 i = 0; i < 3; ++i)
	{
		SetClassVolume(i, ClassVolumes.IsValidIndex(i) ? ClassVolumes[i] : 1.f);
	}
}

void ADemo0904_SoundClassMix::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (OptionsMix)
	{
		for (USoundClass* Class : Classes)
		{
			if (Class)
			{
				UGameplayStatics::ClearSoundMixClassOverride(this, OptionsMix, Class, 0.f);
			}
		}
		UGameplayStatics::PopSoundMixModifier(this, OptionsMix);
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0904_SoundClassMix::SetClassVolume(int32 Index, float Volume)
{
	if (!Classes.IsValidIndex(Index) || !ClassVolumes.IsValidIndex(Index))
	{
		return;
	}
	ClassVolumes[Index] = FMath::Clamp(Volume, 0.f, 1.f);
	Selected = Index;
	// 핵심 한 줄: 믹스 안의 이 Sound Class 볼륨을 덮어쓴다 (FadeInTime 동안 부드럽게)
	if (OptionsMix && Classes[Index])
	{
		UGameplayStatics::SetSoundMixClassOverride(this, OptionsMix, Classes[Index], ClassVolumes[Index], 1.f, FadeInTime, true);
	}
}

void ADemo0904_SoundClassMix::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	GlossaryAudioKit::KeepViewTarget(this, Camera);
	Time += DeltaSeconds;

	if (WasKeyPressed(EKeys::One))   { Selected = 0; }
	if (WasKeyPressed(EKeys::Two))   { Selected = 1; }
	if (WasKeyPressed(EKeys::Three)) { Selected = 2; }
	if (WasKeyPressed(EKeys::LeftBracket) || WasKeyPressed(EKeys::Down))  { SetClassVolume(Selected, ClassVolumes[Selected] - 0.1f); }
	if (WasKeyPressed(EKeys::RightBracket) || WasKeyPressed(EKeys::Up)) { SetClassVolume(Selected, ClassVolumes[Selected] + 0.1f); }
	if (WasKeyPressed(EKeys::M))            { SetClassVolume(Selected, ClassVolumes[Selected] > 0.01f ? 0.f : 1.f); }

	FString Line;
	for (int32 i = 0; i < 3; ++i)
	{
		const float Envelope = Probes.IsValidIndex(i) ? Probes[i]->GetEnvelope() : 0.f;
		if (Options)
		{
			Options->SetValue(i, ClassVolumes[i], i == Selected);
		}
		if (Panel)
		{
			Panel->SetRow(i, GlossaryAudioKit::AmplitudeToBar(Envelope), FString::Printf(TEXT("%.3f"), Envelope));
		}
		// 스피커 콘이 Envelope 에 맞춰 떨리고, 옆 막대 높이 = 그룹 볼륨
		const float Pulse = 1.f + FMath::Clamp(Envelope * 2.f, 0.f, 0.3f);
		const float Size = FMath::Lerp(0.35f, 1.f, ClassVolumes[i]);
		Speakers[i]->SetRelativeScale3D(FVector(1.5f * Size * Pulse, 1.5f * Size * Pulse, 0.4f));
		const float BarHeight = FMath::Max(2.f, ClassVolumes[i] * 260.f);
		VolumeBars[i]->SetRelativeScale3D(FVector(0.2f, 0.3f, BarHeight / 100.f));
		VolumeBars[i]->SetRelativeLocation(FVector(360.f, SpeakerY[i] + 140.f, 60.f + BarHeight * 0.5f));
		Line += FString::Printf(TEXT("%s %.0f%%  "), ClassNames[i], ClassVolumes[i] * 100.f);
	}
	SetStatus(FString::Printf(TEXT("SMix_Options 적용 중 · %s\n선택: %s (키 ↓/↑ 또는 [ ] 로 조절)"), *Line, ClassNames[Selected]));
}
