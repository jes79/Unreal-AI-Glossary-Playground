#include "GlossaryAudioKit.h"

#include "Blueprint/WidgetTree.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "Misc/App.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryDemoBase.h"
#include "GlossaryKit.h"
#include "Interfaces/MetasoundOutputFormatInterfaces.h"
#include "MetasoundBuilderSubsystem.h"
#include "Sound/SoundWave.h"

namespace GlossaryAudioKit
{
	FString AssetPath(const TCHAR* Name)
	{
		return FString(TEXT("/Game/Glossary/09_Audio/")) + Name;
	}

	ACameraActor* SpawnFixedCamera(UWorld* World, const FVector& Location, const FRotator& Rotation, float FOV)
	{
		ACameraActor* Camera = World ? World->SpawnActor<ACameraActor>(Location, Rotation) : nullptr;
		if (Camera)
		{
			Camera->GetCameraComponent()->SetFieldOfView(FOV);
			Camera->GetCameraComponent()->bConstrainAspectRatio = false;
		}
		return Camera;
	}

	void KeepViewTarget(const AGlossaryDemoBase* Demo, ACameraActor* Camera)
	{
		APlayerController* PC = Demo ? Demo->GetPC() : nullptr;
		if (PC && Camera && PC->GetViewTarget() != Camera)
		{
			PC->SetViewTarget(Camera);
		}
	}

	AActor* SpawnRing(UWorld* World, const FVector& Center, float Radius, const FLinearColor& Color, int32 Segments, float Thickness, float Emissive)
	{
		if (!World)
		{
			return nullptr;
		}
		AActor* Ring = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform(Center));
		USceneComponent* RingRoot = NewObject<USceneComponent>(Ring, TEXT("RingRoot"));
		RingRoot->SetMobility(EComponentMobility::Movable);
		Ring->SetRootComponent(RingRoot);
		RingRoot->RegisterComponent();
		Ring->SetActorLocation(Center);

		// 둘레를 Segments 조각으로 나눠 작은 상자를 접선 방향으로 놓는다
		const float SegmentLength = 2.f * PI * Radius / Segments * 1.05f;
		for (int32 i = 0; i < Segments; ++i)
		{
			const float Angle = 2.f * PI * i / Segments;
			const FVector Local(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Thickness * 0.5f);
			const FRotator Rot(0.f, FMath::RadiansToDegrees(Angle) + 90.f, 0.f);
			UStaticMeshComponent* Seg = GlossaryKit::AddShape(Ring, GlossaryKit::EShape::Cube, Local, FVector(SegmentLength, Thickness, Thickness), Color, RingRoot, Rot);
			if (Seg)
			{
				Seg->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Seg->SetCastShadow(false);
				GlossaryKit::SetColor(Seg, Color, 0.6f, 0.f, Emissive);
			}
		}
		return Ring;
	}

	bool GetListener(const AGlossaryDemoBase* Demo, FVector& OutLocation, FRotator& OutRotation)
	{
		APlayerController* PC = Demo ? Demo->GetPC() : nullptr;
		if (!PC)
		{
			return false;
		}
		FVector FrontDir, RightDir;
		PC->GetAudioListenerPosition(OutLocation, FrontDir, RightDir);
		OutRotation = FrontDir.Rotation();
		return true;
	}

	UMetaSoundSourceBuilder* CreateEngineRPMBuilder(FName BuilderName, FString& OutLog)
	{
		UMetaSoundBuilderSubsystem* Subsystem = UMetaSoundBuilderSubsystem::Get();
		if (!Subsystem)
		{
			OutLog = TEXT("UMetaSoundBuilderSubsystem 없음");
			return nullptr;
		}

		EMetaSoundBuilderResult Result = EMetaSoundBuilderResult::Failed;
		auto Check = [&OutLog, &Result](const TCHAR* Step)
		{
			OutLog += FString::Printf(TEXT("%s:%s "), Step, Result == EMetaSoundBuilderResult::Succeeded ? TEXT("OK") : TEXT("실패"));
			return Result == EMetaSoundBuilderResult::Succeeded;
		};

		// 1) Source 빌더: 계속 도는 엔진 소리라 One-Shot 이 아닌 Mono 소스
		FMetaSoundBuilderNodeOutputHandle OnPlay;
		FMetaSoundBuilderNodeInputHandle OnFinished;
		TArray<FMetaSoundBuilderNodeInputHandle> AudioOuts;
		UMetaSoundSourceBuilder* Builder = Subsystem->CreateSourceBuilder(BuilderName, OnPlay, OnFinished, AudioOuts, Result,
			EMetaSoundOutputAudioFormat::Mono, /*bIsOneShot*/ false);
		if (!Check(TEXT("CreateSourceBuilder")) || !Builder || AudioOuts.Num() == 0)
		{
			return nullptr;
		}

		// 2) 그래프 입력 "RPM" (Float, 기본 0) — 코드에서 SetFloatParameter("RPM", x) 로 바꾸는 값
		FName FloatType;
		const FMetasoundFrontendLiteral DefaultRPM = Subsystem->CreateFloatMetaSoundLiteral(0.f, FloatType);
		const FMetaSoundBuilderNodeOutputHandle RPMOut = Builder->AddGraphInputNode(TEXT("RPM"), FloatType, DefaultRPM, Result);
		if (!Check(TEXT("Input RPM")))
		{
			return nullptr;
		}

		// 3) Map Range (Float): RPM 0~1 → 주파수 40~220Hz
		const FMetaSoundNodeHandle MapRange = Builder->AddNodeByClassName(FMetasoundFrontendClassName(TEXT("MapRange"), TEXT("MapRange"), TEXT("Float")), Result);
		if (!Check(TEXT("MapRange")))
		{
			return nullptr;
		}
		Builder->SetNodeInputDefault(MapRange, TEXT("In Range A"), 0.f, Result);
		Builder->SetNodeInputDefault(MapRange, TEXT("In Range B"), 1.f, Result);
		Builder->SetNodeInputDefault(MapRange, TEXT("Out Range A"), 40.f, Result);
		Builder->SetNodeInputDefault(MapRange, TEXT("Out Range B"), 220.f, Result);
		Check(TEXT("MapRange 기본값"));
		Builder->ConnectNodes(RPMOut, Builder->FindNodeInputByName(MapRange, TEXT("In"), Result), Result);
		if (!Check(TEXT("RPM→In")))
		{
			return nullptr;
		}

		// 4) Saw 오실레이터: 톱니파가 엔진 배기음처럼 거칠게 들린다. Glide 로 주파수 변화를 부드럽게
		const FMetaSoundNodeHandle Saw = Builder->AddNodeByClassName(FMetasoundFrontendClassName(TEXT("UE"), TEXT("Saw"), TEXT("Audio")), Result);
		if (!Check(TEXT("Saw")))
		{
			return nullptr;
		}
		Builder->SetNodeInputDefault(Saw, TEXT("Glide"), 0.3f, Result);
		Builder->ConnectNodes(Builder->FindNodeOutputByName(MapRange, TEXT("Out Value"), Result),
			Builder->FindNodeInputByName(Saw, TEXT("Frequency"), Result), Result);
		if (!Check(TEXT("Map→Frequency")))
		{
			return nullptr;
		}

		// 5) Saw → 출력(Mono Out)
		Builder->ConnectNodes(Builder->FindNodeOutputByName(Saw, TEXT("Audio"), Result), AudioOuts[0], Result);
		if (!Check(TEXT("Saw→Out")))
		{
			return nullptr;
		}
		return Builder;
	}

	void PrepareAudioDemo()
	{
		FApp::SetUnfocusedVolumeMultiplier(1.f);
	}

	float AmplitudeToBar(float Amplitude, float FloorDb)
	{
		if (Amplitude <= KINDA_SMALL_NUMBER)
		{
			return 0.f;
		}
		const float Db = 20.f * FMath::LogX(10.f, Amplitude);
		return FMath::Clamp(1.f - Db / FloorDb, 0.f, 1.f);
	}
}

// ---------------------------------------------------------------- UGlossaryAudioProbe

void UGlossaryAudioProbe::Bind(UAudioComponent* InComponent)
{
	Component = InComponent;
	if (InComponent)
	{
		InComponent->OnAudioSingleEnvelopeValue.AddDynamic(this, &UGlossaryAudioProbe::HandleEnvelope);
		InComponent->OnAudioPlaybackPercent.AddDynamic(this, &UGlossaryAudioProbe::HandlePercent);
	}
}

float UGlossaryAudioProbe::GetEnvelope() const
{
	return (FPlatformTime::Seconds() - EnvelopeTime) < 0.2 ? Envelope : 0.f;
}

void UGlossaryAudioProbe::HandleEnvelope(const USoundWave* PlayingSoundWave, const float EnvelopeValue)
{
	Envelope = EnvelopeValue;
	EnvelopeTime = FPlatformTime::Seconds();
}

void UGlossaryAudioProbe::HandlePercent(const USoundWave* PlayingSoundWave, const float PlaybackPercent)
{
	// 웨이브가 바뀌거나 재생 위치가 되감기면 새 재생으로 본다
	if (PlayingSoundWave != CurrentWave.Get() || PlaybackPercent + 0.02f < LastPercent)
	{
		PitchSamples.Reset();
	}
	CurrentWave = PlayingSoundWave;
	LastPercent = PlaybackPercent;
	PitchSamples.Add(FVector2D(FPlatformTime::Seconds(), PlaybackPercent));
}

void UGlossaryAudioProbe::ResetPitchSamples()
{
	PitchSamples.Reset();
	CurrentWave = nullptr;
	LastPercent = 0.f;
}

bool UGlossaryAudioProbe::EstimatePitch(float& OutPitch) const
{
	const USoundWave* Wave = CurrentWave.Get();
	if (!Wave || PitchSamples.Num() < 4 || Wave->Duration <= 0.f)
	{
		return false;
	}
	// 최소제곱 직선의 기울기 = 초당 재생 비율. 게임 스레드에 도착하는 시각의 흔들림을 평균으로 줄인다
	const double T0 = PitchSamples[0].X;
	double SumT = 0, SumP = 0, SumTT = 0, SumTP = 0;
	for (const FVector2D& S : PitchSamples)
	{
		const double T = S.X - T0;
		SumT += T; SumP += S.Y; SumTT += T * T; SumTP += T * S.Y;
	}
	const double N = PitchSamples.Num();
	const double Denom = N * SumTT - SumT * SumT;
	if (FMath::Abs(Denom) < 1e-9 || (PitchSamples.Last().X - T0) < 0.1)
	{
		return false;
	}
	const double Slope = (N * SumTP - SumT * SumP) / Denom;
	OutPitch = static_cast<float>(Slope * Wave->Duration);
	return OutPitch > 0.2f && OutPitch < 4.f;
}

// ---------------------------------------------------------------- UGlossaryAudioPanel

int32 UGlossaryAudioPanel::AddRow(const FString& Label, const FLinearColor& Color)
{
	return RowDefs.Add({ Label, Color });
}

void UGlossaryAudioPanel::SetRow(int32 Index, float Fraction, const FString& Value)
{
	if (Bars.IsValidIndex(Index) && Bars[Index])
	{
		Bars[Index]->SetPercent(FMath::Clamp(Fraction, 0.f, 1.f));
	}
	if (Values.IsValidIndex(Index) && Values[Index])
	{
		Values[Index]->SetText(FText::FromString(Value));
	}
}

void UGlossaryAudioPanel::SetRowColor(int32 Index, const FLinearColor& Color)
{
	if (Bars.IsValidIndex(Index) && Bars[Index])
	{
		Bars[Index]->SetFillColorAndOpacity(Color);
	}
}

void UGlossaryAudioPanel::SetFooter(const FString& Text)
{
	PendingFooter = Text;
	if (Footer)
	{
		Footer->SetText(FText::FromString(Text));
	}
}

UGlossaryAudioPanel* UGlossaryAudioPanel::Create(AGlossaryDemoBase* Demo, const FString& InTitle, float InBarWidth)
{
	APlayerController* PC = Demo ? Demo->GetPC() : nullptr;
	if (!PC)
	{
		return nullptr;
	}
	UGlossaryAudioPanel* Panel = CreateWidget<UGlossaryAudioPanel>(PC, UGlossaryAudioPanel::StaticClass());
	Panel->Title = InTitle;
	Panel->BarWidth = InBarWidth;
	return Panel;
}

void UGlossaryAudioPanel::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UBorder* Frame = MakeBorder(FLinearColor(0.02f, 0.03f, 0.05f, 0.82f), FMargin(14.f, 10.f));
	// 정보 패널(왼쪽 위 620×440)과 겹치지 않게 오른쪽 위에 붙인다
	AddToCanvas(Canvas, Frame, FAnchors(1.f, 0.f), FVector2D(-20.f, 20.f), FVector2D::ZeroVector, FVector2D(1.f, 0.f));

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Frame->SetContent(Box);
	if (!Title.IsEmpty())
	{
		UVerticalBoxSlot* TitleSlot = Box->AddChildToVerticalBox(MakeText(Title, 17.f, FLinearColor(1.f, 0.85f, 0.3f), true));
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}

	Bars.Reset();
	Values.Reset();
	for (const FRowDef& Def : RowDefs)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UVerticalBoxSlot* RowSlot = Box->AddChildToVerticalBox(Row);
		RowSlot->SetPadding(FMargin(0.f, 2.f));

		USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		LabelBox->SetWidthOverride(130.f);
		LabelBox->SetContent(MakeText(Def.Label, 14.f, FLinearColor(0.85f, 0.88f, 0.92f)));
		Row->AddChildToHorizontalBox(LabelBox)->SetVerticalAlignment(VAlign_Center);

		USizeBox* BarBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		BarBox->SetWidthOverride(BarWidth);
		BarBox->SetHeightOverride(16.f);
		UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
		Bar->SetFillColorAndOpacity(Def.Color);
		Bar->SetPercent(0.f);
		BarBox->SetContent(Bar);
		Row->AddChildToHorizontalBox(BarBox)->SetVerticalAlignment(VAlign_Center);
		Bars.Add(Bar);

		USizeBox* ValueBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		ValueBox->SetWidthOverride(150.f);
		UTextBlock* Value = MakeText(TEXT("-"), 14.f, FLinearColor::White);
		ValueBox->SetContent(Value);
		UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(ValueBox);
		ValueSlot->SetVerticalAlignment(VAlign_Center);
		ValueSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
		Values.Add(Value);
	}

	Footer = MakeText(PendingFooter, 13.f, FLinearColor(0.7f, 0.75f, 0.8f));
	Footer->SetAutoWrapText(true);
	UVerticalBoxSlot* FooterSlot = Box->AddChildToVerticalBox(Footer);
	FooterSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
}

// ---------------------------------------------------------------- UGlossaryAudioBanner

UGlossaryAudioBanner* UGlossaryAudioBanner::Create(AGlossaryDemoBase* Demo)
{
	APlayerController* PC = Demo ? Demo->GetPC() : nullptr;
	if (!PC)
	{
		return nullptr;
	}
	UGlossaryAudioBanner* Banner = CreateWidget<UGlossaryAudioBanner>(PC, UGlossaryAudioBanner::StaticClass());
	Banner->AddToViewport(5);
	Banner->Hide();
	return Banner;
}

void UGlossaryAudioBanner::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	Frame = MakeBorder(FLinearColor(0.f, 0.f, 0.f, 0.65f), FMargin(28.f, 10.f));
	Label = MakeText(TEXT(""), 34.f, FLinearColor::White, true);
	Frame->SetContent(Label);
	AddToCanvas(Canvas, Frame, FAnchors(0.5f, 1.f), FVector2D(0.f, -60.f), FVector2D::ZeroVector, FVector2D(0.5f, 1.f));
}

void UGlossaryAudioBanner::Show(const FString& Text, const FLinearColor& Color)
{
	if (Label)
	{
		Label->SetText(FText::FromString(Text));
		Label->SetColorAndOpacity(FSlateColor(Color));
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UGlossaryAudioBanner::Hide()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
