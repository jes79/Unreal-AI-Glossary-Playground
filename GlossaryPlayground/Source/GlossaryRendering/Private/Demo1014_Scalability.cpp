#include "Demo1014_Scalability.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "D1014_ScalabilityWidget.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "RenderingAssetPaths.h"

using namespace GlossaryKit;

namespace
{
	const TCHAR* QualityNames[] = {TEXT("낮음(Low)"), TEXT("중간(Medium)"), TEXT("높음(High)"), TEXT("에픽(Epic)"), TEXT("시네마틱(Cinematic)")};

	FString FrameLimitText(float Limit) { return Limit <= 0.f ? FString(TEXT("무제한")) : FString::Printf(TEXT("%.0f fps"), Limit); }
}

// ------------------------------------------------------------------ 옵션 메뉴 위젯

void UD1014_ScalabilityWidget::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UBorder* Panel = MakeBorder(FLinearColor(0.03f, 0.04f, 0.06f, 0.85f), FMargin(14.f));
	AddToCanvas(Canvas, Panel, FAnchors(1.f, 0.f), FVector2D(-20.f, 20.f), FVector2D(430.f, 0.f), FVector2D(1.f, 0.f))->SetAutoSize(true);
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Box);

	auto AddRow = [this, Box](const FString& Title, const TArray<TPair<FString, FName>>& Buttons)
	{
		Box->AddChildToVerticalBox(MakeText(Title, 14.f, FLinearColor(0.7f, 0.8f, 1.f), true))->SetPadding(FMargin(0.f, 8.f, 0.f, 2.f));
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		for (const TPair<FString, FName>& B : Buttons)
		{
			UButton* Button = MakeButton(B.Key, 12.f);
			FScriptDelegate Delegate;
			Delegate.BindUFunction(this, B.Value);
			Button->OnClicked.Add(Delegate);
			Row->AddChildToHorizontalBox(Button)->SetPadding(FMargin(0.f, 0.f, 4.f, 0.f));
		}
		Box->AddChildToVerticalBox(Row);
	};

	Box->AddChildToVerticalBox(MakeText(TEXT("그래픽 옵션 (UGameUserSettings)"), 17.f, FLinearColor::White, true));
	AddRow(TEXT("전체 품질 — SetOverallScalabilityLevel"), {
		{TEXT("낮음"), GET_FUNCTION_NAME_CHECKED(UD1014_ScalabilityWidget, OnLow)},
		{TEXT("중간"), GET_FUNCTION_NAME_CHECKED(UD1014_ScalabilityWidget, OnMedium)},
		{TEXT("높음"), GET_FUNCTION_NAME_CHECKED(UD1014_ScalabilityWidget, OnHigh)},
		{TEXT("에픽"), GET_FUNCTION_NAME_CHECKED(UD1014_ScalabilityWidget, OnEpic)},
		{TEXT("시네마틱"), GET_FUNCTION_NAME_CHECKED(UD1014_ScalabilityWidget, OnCinematic)}});
	AddRow(TEXT("해상도 — SetScreenResolution"), {
		{TEXT("1280×720"), GET_FUNCTION_NAME_CHECKED(UD1014_ScalabilityWidget, OnRes720)},
		{TEXT("1600×900"), GET_FUNCTION_NAME_CHECKED(UD1014_ScalabilityWidget, OnRes900)},
		{TEXT("1920×1080"), GET_FUNCTION_NAME_CHECKED(UD1014_ScalabilityWidget, OnRes1080)}});
	AddRow(TEXT("프레임 제한 — SetFrameRateLimit"), {
		{TEXT("30"), GET_FUNCTION_NAME_CHECKED(UD1014_ScalabilityWidget, OnFps30)},
		{TEXT("60"), GET_FUNCTION_NAME_CHECKED(UD1014_ScalabilityWidget, OnFps60)},
		{TEXT("무제한"), GET_FUNCTION_NAME_CHECKED(UD1014_ScalabilityWidget, OnFpsUnlimited)}});

	PendingText = MakeText(TEXT(""), 13.f, FLinearColor(1.f, 0.85f, 0.3f));
	Box->AddChildToVerticalBox(PendingText)->SetPadding(FMargin(0.f, 10.f, 0.f, 4.f));
	AddRow(TEXT(""), {{TEXT("적용 (ApplySettings)"), GET_FUNCTION_NAME_CHECKED(UD1014_ScalabilityWidget, OnApply)}});
	AppliedText = MakeText(TEXT(""), 13.f, FLinearColor(0.6f, 1.f, 0.6f));
	Box->AddChildToVerticalBox(AppliedText)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
}

void UD1014_ScalabilityWidget::Refresh(int32 PendingQuality, FIntPoint PendingResolution, float PendingFrameLimit, const FString& InApplied)
{
	if (PendingText)
	{
		PendingText->SetText(FText::FromString(FString::Printf(TEXT("고른 값: %s · %d×%d · %s"),
			QualityNames[FMath::Clamp(PendingQuality, 0, 4)], PendingResolution.X, PendingResolution.Y, *FrameLimitText(PendingFrameLimit))));
	}
	if (AppliedText)
	{
		AppliedText->SetText(FText::FromString(InApplied));
	}
}

#define D1014_SET(Func, Body) void UD1014_ScalabilityWidget::Func() { if (ADemo1014_Scalability* D = Demo.Get()) { Body; D->RefreshWidget(); } }
D1014_SET(OnLow, D->PendingQuality = 0)
D1014_SET(OnMedium, D->PendingQuality = 1)
D1014_SET(OnHigh, D->PendingQuality = 2)
D1014_SET(OnEpic, D->PendingQuality = 3)
D1014_SET(OnCinematic, D->PendingQuality = 4)
D1014_SET(OnRes720, D->PendingResolution = FIntPoint(1280, 720))
D1014_SET(OnRes900, D->PendingResolution = FIntPoint(1600, 900))
D1014_SET(OnRes1080, D->PendingResolution = FIntPoint(1920, 1080))
D1014_SET(OnFps30, D->PendingFrameLimit = 30.f)
D1014_SET(OnFps60, D->PendingFrameLimit = 60.f)
D1014_SET(OnFpsUnlimited, D->PendingFrameLimit = 0.f)
D1014_SET(OnApply, D->ApplyPending())
#undef D1014_SET

// ------------------------------------------------------------------ 데모

ADemo1014_Scalability::ADemo1014_Scalability()
{
	DemoId = TEXT("1014");
	Controls = TEXT("오른쪽 메뉴에서 고르고 [적용]  ·  단축키 1~5 : 낮음~시네마틱 바로 적용   F3 : 마우스 커서\n")
		TEXT("관찰: 낮음은 그림자·GI·반사·안개 품질이 크게 떨어지고(sg.* 값) GPU 시간이 줄어든다.\n")
		TEXT("시네마틱은 가장 좋지만 가장 느리다. 레벨을 나가면 원래 설정으로 되돌린다.");
	Showcase = TEXT("UGameUserSettings 옵션 메뉴(품질 낮음~시네마틱·해상도·프레임 제한) + ApplySettings, 숲 장면 품질·GPU 시간 비교");
	PlayerPawnClass = nullptr;
	bShowMouseCursor = true;
	Environment.Sky = EGlossarySky::Sunset;
	Environment.bClouds = true;
}

void ADemo1014_Scalability::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 품질 차이가 잘 보이는 장면: 숲(그림자) + 반사 구 + 안개 + 하늘
	if (UStaticMesh* Tree = LoadAsset<UStaticMesh>(RenderingAssets::Tree))
	{
		FRandomStream Rand(1014);
		for (int32 i = 0; i < 300; ++i)
		{
			AStaticMeshActor* A = World->SpawnActor<AStaticMeshActor>(FVector(Rand.FRandRange(900.f, 6000.f), Rand.FRandRange(-3000.f, 3000.f), 0.f), FRotator(0.f, Rand.FRandRange(0.f, 360.f), 0.f));
			A->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
			A->GetStaticMeshComponent()->SetStaticMesh(Tree);
		}
	}
	for (int32 i = 0; i < 4; ++i)
	{
		AStaticMeshActor* S = SpawnShape(World, EShape::Sphere, FVector(400.f, -450.f + i * 300.f, 80.f), FVector(160.f), FLinearColor::White);
		SetColor(S->GetStaticMeshComponent(), FLinearColor(0.9f, 0.9f, 0.9f), 0.05f + i * 0.2f, 1.f);
	}
	RenderKit::SpawnViewCamera(this, FVector(-300.f, -200.f, 260.f), FRotator(-8.f, 10.f, 0.f), 80.f);

	// 처음 설정을 기억해 두었다가 EndPlay 에서 되돌린다
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	OriginalQuality = Settings->GetOverallScalabilityLevel();
	OriginalResolution = Settings->GetScreenResolution();
	OriginalFrameLimit = Settings->GetFrameRateLimit();
	PendingQuality = OriginalQuality < 0 ? 3 : OriginalQuality;
	PendingResolution = OriginalResolution;
	PendingFrameLimit = OriginalFrameLimit;

	if (APlayerController* PC = GetPC())
	{
		Menu = CreateWidget<UD1014_ScalabilityWidget>(PC, UD1014_ScalabilityWidget::StaticClass());
		Menu->Demo = this;
		Menu->AddToViewport(50);
	}
	RefreshWidget();
}

void ADemo1014_Scalability::ApplyPending()
{
	// 핵심: 값을 넣고 ApplySettings (내부에서 Scalability·해상도·프레임 제한을 적용하고 GameUserSettings.ini 에 저장)
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	Settings->SetOverallScalabilityLevel(PendingQuality);
	Settings->SetFrameRateLimit(PendingFrameLimit);
	if (Settings->GetScreenResolution() != PendingResolution)
	{
		Settings->SetFullscreenMode(EWindowMode::Windowed);
		Settings->SetScreenResolution(PendingResolution);
		Settings->ApplySettings(false);   // 해상도까지 전부 적용 + 저장
	}
	else
	{
		// 해상도는 그대로 두고 나머지만 적용 (ApplySettings 의 해상도 이외 부분) + 저장
		Settings->ApplyNonResolutionSettings();
		Settings->SaveSettings();
	}
	bChangedSettings = true;
	RefreshWidget();
}

void ADemo1014_Scalability::RefreshWidget()
{
	if (!Menu)
	{
		return;
	}
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	const int32 Level = Settings->GetOverallScalabilityLevel();
	const FIntPoint Res = Settings->GetScreenResolution();
	Menu->Refresh(PendingQuality, PendingResolution, PendingFrameLimit, FString::Printf(TEXT("적용됨: %s · %d×%d · %s"),
		Level < 0 ? TEXT("사용자 지정") : QualityNames[FMath::Clamp(Level, 0, 4)], Res.X, Res.Y, *FrameLimitText(Settings->GetFrameRateLimit())));
}

void ADemo1014_Scalability::EndPlay(const EEndPlayReason::Type Reason)
{
	if (bChangedSettings)
	{
		UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
		if (OriginalQuality >= 0)
		{
			Settings->SetOverallScalabilityLevel(OriginalQuality);
		}
		Settings->SetScreenResolution(OriginalResolution);
		Settings->SetFrameRateLimit(OriginalFrameLimit);
		Settings->ApplyNonResolutionSettings();
		Settings->SaveSettings();
	}
	if (Menu)
	{
		Menu->RemoveFromParent();
	}
	Super::EndPlay(Reason);
}

void ADemo1014_Scalability::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const FKey Keys[] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five};
	for (int32 i = 0; i < 5; ++i)
	{
		if (WasKeyPressed(Keys[i]))
		{
			PendingQuality = i;
			ApplyPending();
		}
	}

	Stats.Update(DeltaSeconds);
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	const int32 Level = Settings->GetOverallScalabilityLevel();
	SetStatus(FString::Printf(TEXT("전체 품질 %s · sg.ShadowQuality %s · sg.GlobalIlluminationQuality %s · sg.ReflectionQuality %s\n%s"),
		Level < 0 ? TEXT("사용자 지정") : QualityNames[FMath::Clamp(Level, 0, 4)],
		*RenderKit::GetCVar(TEXT("sg.ShadowQuality")), *RenderKit::GetCVar(TEXT("sg.GlobalIlluminationQuality")),
		*RenderKit::GetCVar(TEXT("sg.ReflectionQuality")), *Stats.ToString()));
}
