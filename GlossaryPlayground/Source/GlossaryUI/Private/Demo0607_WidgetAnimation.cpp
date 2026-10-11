#include "Demo0607_WidgetAnimation.h"

#include "Animation/MovieScene2DTransformSection.h"
#include "Animation/MovieScene2DTransformTrack.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GlossaryKit.h"
#include "GlossaryUIKit.h"
#include "MovieScene.h"
#include "Sections/MovieSceneFloatSection.h"
#include "Tracks/MovieSceneFloatTrack.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- 실행 중 UWidgetAnimation 조립

UWidgetAnimation* Demo0607::BuildAnimation(UUserWidget* Owner, UWidget* Target, const TArray<FKey2D>& TransformKeys,
	const TArray<TPair<float, float>>& OpacityKeys)
{
	// 1) 애니메이션 = 시퀀스. 안에 MovieScene 이 있고 시간 단위(TickResolution)를 정한다
	UWidgetAnimation* Anim = NewObject<UWidgetAnimation>(Owner, NAME_None, RF_Transient);
	UMovieScene* Scene = NewObject<UMovieScene>(Anim, NAME_None, RF_Transient);
	Anim->MovieScene = Scene;
	const FFrameRate TickRate(24000, 1);
	Scene->SetTickResolutionDirectly(TickRate);
	Scene->SetDisplayRate(FFrameRate(60, 1));

	float EndTime = 0.f;
	for (const FKey2D& K : TransformKeys) { EndTime = FMath::Max(EndTime, K.Time); }
	for (const TPair<float, float>& K : OpacityKeys) { EndTime = FMath::Max(EndTime, K.Key); }
	auto ToFrame = [&TickRate](float Seconds) { return (Seconds * TickRate).RoundToFrame(); };
	Scene->SetPlaybackRange(FFrameNumber(0), ToFrame(EndTime).Value);

	// 2) 대상 위젯을 '소유(Possessable)' 로 등록하고 위젯 이름으로 바인딩 (디자이너에서 +Track 으로 위젯을 고르는 것)
	const FGuid Guid = Scene->AddPossessable(Target->GetName(), Target->GetClass());
	UMovieSceneSequence* Sequence = Anim;
	Sequence->BindPossessableObject(Guid, *Target, Owner);

	// 3) Transform 트랙 (RenderTransform 속성): Translation · Scale 키
	if (TransformKeys.Num() > 0)
	{
		UMovieScene2DTransformTrack* Track = Cast<UMovieScene2DTransformTrack>(Scene->AddTrack(UMovieScene2DTransformTrack::StaticClass(), Guid));
		Track->SetPropertyNameAndPath(TEXT("RenderTransform"), TEXT("RenderTransform"));
		UMovieSceneTrack* TrackBase = Track;
		UMovieScene2DTransformSection* Section = Cast<UMovieScene2DTransformSection>(TrackBase->CreateNewSection());
		Section->SetRange(TRange<FFrameNumber>::All());
		// 키를 넣지 않는 채널은 기본값(항등 변환)으로
		Section->Rotation.SetDefault(0.f);
		Section->Shear[0].SetDefault(0.f);
		Section->Shear[1].SetDefault(0.f);
		for (const FKey2D& K : TransformKeys)
		{
			const FFrameNumber F = ToFrame(K.Time);
			Section->Translation[0].AddCubicKey(F, K.Translation.X);
			Section->Translation[1].AddCubicKey(F, K.Translation.Y);
			Section->Scale[0].AddCubicKey(F, K.Scale.X);
			Section->Scale[1].AddCubicKey(F, K.Scale.Y);
		}
		for (FMovieSceneFloatChannel* C : { &Section->Translation[0], &Section->Translation[1], &Section->Scale[0], &Section->Scale[1] })
		{
			C->AutoSetTangents();
		}
		TrackBase->AddSection(*Section);
	}

	// 4) Float 트랙 (RenderOpacity 속성)
	if (OpacityKeys.Num() > 0)
	{
		UMovieSceneFloatTrack* Track = Cast<UMovieSceneFloatTrack>(Scene->AddTrack(UMovieSceneFloatTrack::StaticClass(), Guid));
		Track->SetPropertyNameAndPath(TEXT("RenderOpacity"), TEXT("RenderOpacity"));
		UMovieSceneTrack* TrackBase = Track;
		UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(TrackBase->CreateNewSection());
		Section->SetRange(TRange<FFrameNumber>::All());
		for (const TPair<float, float>& K : OpacityKeys)
		{
			Section->GetChannel().AddCubicKey(ToFrame(K.Key), K.Value);
		}
		Section->GetChannel().AutoSetTangents();
		TrackBase->AddSection(*Section);
	}
	return Anim;
}

// ---------------------------------------------------------------- 호버 버튼

void UDemo0607_HoverButton::BuildTree()
{
	UButton* Button = GlossaryUIKit::MakeButton(WidgetTree, Label, 18.f, FLinearColor(0.15f, 0.3f, 0.6f, 1.f));
	WidgetTree->RootWidget = GlossaryUIKit::WrapSize(WidgetTree, Button, 300.f, 52.f);
}

void UDemo0607_HoverButton::NativeConstruct()
{
	Super::NativeConstruct();
	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	if (!HoverAnim)
	{
		// 0 → HoverDuration 동안 Scale 1 → 1.1 (자기 자신 = 루트 위젯을 바인딩)
		HoverAnim = Demo0607::BuildAnimation(this, this,
			{ { 0.f, FVector2D::ZeroVector, FVector2D(1.f) }, { HoverDuration, FVector2D::ZeroVector, FVector2D(HoverScale) } }, {});
	}
}

void UDemo0607_HoverButton::SetHovered(bool bHovered)
{
	if (bHovered == bHoveredState || !HoverAnim)
	{
		return;
	}
	bHoveredState = bHovered;
	// 지금 재생 위치에서 앞으로/거꾸로 이어서 재생 (중간에 마우스를 빼도 튀지 않는다)
	if (bHovered)
	{
		PlayAnimationForward(HoverAnim);
	}
	else
	{
		PlayAnimationReverse(HoverAnim);
	}
}

void UDemo0607_HoverButton::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	SetHovered(true);
}

void UDemo0607_HoverButton::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	SetHovered(false);
}

// ---------------------------------------------------------------- 메뉴 창

void UDemo0607_Menu::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	Window = MakeBorder(FLinearColor(0.03f, 0.05f, 0.1f, 0.92f), FMargin(24.f, 18.f));
	// 이름을 붙여야 애니메이션 바인딩(위젯 이름)으로 찾을 수 있다
	Window->Rename(TEXT("Window"));
	AddToCanvas(Canvas, Window, FAnchors(0.5f, 1.f), FVector2D(240.f, -40.f), FVector2D::ZeroVector, FVector2D(0.5f, 1.f));

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Window->SetContent(Col);
	Col->AddChildToVerticalBox(MakeText(TEXT("메뉴 창 (Widget Animation)"), 22.f, FLinearColor(1.f, 0.85f, 0.35f), true))
		->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	for (const TCHAR* Name : { TEXT("이어하기"), TEXT("설정"), TEXT("나가기") })
	{
		UDemo0607_HoverButton* Button = CreateWidget<UDemo0607_HoverButton>(this, UDemo0607_HoverButton::StaticClass());
		Button->Label = Name;
		Button->HoverScale = HoverScale;
		Buttons.Add(Button);
		Col->AddChildToVerticalBox(Button)->SetPadding(FMargin(0.f, 6.f));
	}
}

void UDemo0607_Menu::NativeConstruct()
{
	Super::NativeConstruct();
	if (!OpenAnim && Window)
	{
		// 창 열기: 아래로 SlideDistance 만큼 내려가 투명한 상태 → 제자리·불투명 (OpenDuration 초)
		OpenAnim = Demo0607::BuildAnimation(this, Window,
			{ { 0.f, FVector2D(0.f, SlideDistance), FVector2D(1.f) }, { OpenDuration, FVector2D::ZeroVector, FVector2D(1.f) } },
			{ { 0.f, 0.f }, { OpenDuration * 0.6f, 1.f } });
	}
	Open();
}

void UDemo0607_Menu::Open()
{
	bOpen = true;
	PlayAnimationForward(OpenAnim);
}

void UDemo0607_Menu::Close()
{
	bOpen = false;
	PlayAnimationReverse(OpenAnim);
}

bool UDemo0607_Menu::IsOpenAnimPlaying() const
{
	return OpenAnim && IsAnimationPlaying(OpenAnim);
}

float UDemo0607_Menu::GetOpenAnimTime() const
{
	return OpenAnim ? GetAnimationCurrentTime(OpenAnim) : 0.f;
}

// ---------------------------------------------------------------- 데모

ADemo0607_WidgetAnimation::ADemo0607_WidgetAnimation()
{
	DemoId = TEXT("0607");
	Controls = TEXT("버튼에 마우스 올리기 : 1.1배 확대   O : 창 열기   X : 창 닫기 (같은 애니메이션 역재생)\n")
		TEXT("1/2/3 : 버튼 호버 시연 (마우스 없이)\n")
		TEXT("관찰: 창이 아래에서 0.4초 동안 미끄러져 올라오며 나타나고, 버튼은 커졌다가 마우스를 떼면 되돌아간다");
	Showcase = TEXT("실행 중 조립한 UWidgetAnimation 으로 창 슬라이드-인(이동+투명도)과 버튼 호버 1.1배 확대");
	Implementation = EGlossaryImplementation::Alternative;
	AlternativeNote = TEXT("WBP Animations 탭 대신 같은 UWidgetAnimation(MovieScene 의 RenderTransform·RenderOpacity 트랙과 키)을 C++ 로 조립해 "
		"PlayAnimationForward/Reverse 로 재생. 실제로는 WBP 에서 키를 찍고 BindWidgetAnim 으로 C++ 에 연결한다");
	PlayerPawnClass = nullptr;
	bShowMouseCursor = true;
}

void ADemo0607_WidgetAnimation::BeginPlay()
{
	Super::BeginPlay();
	SpawnShape(GetWorld(), EShape::Sphere, FVector(-100.f, -150.f, 60.f), FVector(120.f), Colors::Purple);
	Menu = CreateWidget<UDemo0607_Menu>(GetPC(), UDemo0607_Menu::StaticClass());
	Menu->SlideDistance = SlideDistance;
	Menu->OpenDuration = OpenDuration;
	Menu->HoverScale = HoverScale;
	Menu->AddToViewport(0);
}

void ADemo0607_WidgetAnimation::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Menu)
	{
		Menu->RemoveFromParent();
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0607_WidgetAnimation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Menu)
	{
		return;
	}
	if (WasKeyPressed(EKeys::O)) { Menu->Open(); }
	if (WasKeyPressed(EKeys::X)) { Menu->Close(); }
	const FKey NumKeys[] = { EKeys::One, EKeys::Two, EKeys::Three };
	for (int32 i = 0; i < 3 && i < Menu->Buttons.Num(); ++i)
	{
		if (WasKeyPressed(NumKeys[i]))
		{
			// 키로 호버 상태를 바꿔 본다 (마우스 호버와 같은 함수)
			const bool bNew = !Menu->Buttons[i]->IsHoveredByDemo();
			Menu->Buttons[i]->SetHovered(bNew);
		}
	}

	FString Hover;
	for (UDemo0607_HoverButton* B : Menu->Buttons)
	{
		Hover += FString::Printf(TEXT("%.2f "), B->GetRenderTransform().Scale.X);
	}
	SetStatus(FString::Printf(TEXT("창 %s · OpenAnim %s %.2f / %.2fs\n버튼 Scale: %s"),
		Menu->bOpen ? TEXT("열림") : TEXT("닫힘"), Menu->IsOpenAnimPlaying() ? TEXT("재생 중") : TEXT("정지"),
		Menu->GetOpenAnimTime(), OpenDuration, *Hover));
}
