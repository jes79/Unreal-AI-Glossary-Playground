#include "GlossaryGameMode.h"

#include "EngineUtils.h"
#include "GameFramework/DefaultPawn.h"
#include "GlossaryCatalog.h"
#include "GlossaryCharacter.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "Kismet/GameplayStatics.h"
#include "InputKeyEventArgs.h"
#include "UnrealClient.h"

AGlossaryGameMode::AGlossaryGameMode()
{
	DefaultPawnClass = AGlossaryCharacter::StaticClass();
	PlayerControllerClass = AGlossaryPlayerController::StaticClass();
}

UClass* AGlossaryGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	for (TActorIterator<AGlossaryDemoBase> It(GetWorld()); It; ++It)
	{
		// 데모가 폰을 null 로 두면 화면만 보는 고정 카메라 폰
		return It->PlayerPawnClass ? It->PlayerPawnClass.Get() : APawn::StaticClass();
	}
	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void AGlossaryPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (FParse::Value(FCommandLine::Get(), TEXT("GlossaryShot="), ShotPath))
	{
		FParse::Value(FCommandLine::Get(), TEXT("GlossaryShotDelay="), ShotDelay);
		FString KeyList;
		if (FParse::Value(FCommandLine::Get(), TEXT("GlossaryShotKeys="), KeyList, false)) // false: 쉼표에서 끊지 않음
		{
			TArray<FString> Names;
			KeyList.ParseIntoArray(Names, TEXT(","), true);
			for (const FString& Name : Names)
			{
				ShotKeys.Add(FKey(*Name));
			}
		}
	}
}

void AGlossaryPlayerController::TickAutoShot(float DeltaTime)
{
	if (ShotPath.IsEmpty())
	{
		return;
	}
	ShotTimer += DeltaTime;
	const FInputDeviceId Device = FInputDeviceId::CreateFromInternalId(0);
	// 누른 키는 몇 프레임 뒤에 뗀다 (같은 프레임에 누르고 떼면 '방금 눌림'/Enhanced Input 트리거를 놓치는 데모가 있다)
	static FKey PendingRelease;
	static int32 PendingFrames = 0;
	if (PendingRelease.IsValid() && --PendingFrames <= 0)
	{
		InputKey(FInputKeyEventArgs(nullptr, Device, PendingRelease, IE_Released, FPlatformTime::Cycles64()));
		PendingRelease = FKey();
	}
	// 캡처 전에 지정한 키를 1초 간격으로 누른다 (토글 데모 상태 확인용)
	const float KeyStart = FMath::Max(1.f, ShotDelay - ShotKeys.Num() - 0.5f);
	if (ShotStage == 0 && !PendingRelease.IsValid() && ShotKeyIndex < ShotKeys.Num() && ShotTimer >= KeyStart + ShotKeyIndex)
	{
		const FKey Key = ShotKeys[ShotKeyIndex++];
		InputKey(FInputKeyEventArgs(nullptr, Device, Key, IE_Pressed, FPlatformTime::Cycles64()));
		PendingRelease = Key;
		PendingFrames = 3;
	}
	if (ShotStage == 0 && ShotTimer >= ShotDelay)
	{
		FScreenshotRequest::RequestScreenshot(ShotPath, true, false);
		ShotStage = 1;
		ShotTimer = 0.f;
	}
	else if (ShotStage == 1 && ShotTimer >= 1.5f)
	{
		ShotStage = 2;
		ConsoleCommand(TEXT("quit"));
	}
}

void AGlossaryPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	TickAutoShot(FApp::GetDeltaTime()); // 실제 시간 기준: 일시정지·시간 감속 중에도 캡처가 진행되게

	if (WasInputKeyJustPressed(EKeys::F1))
	{
		for (TActorIterator<AGlossaryDemoBase> It(GetWorld()); It; ++It)
		{
			if (UGlossaryInfoPanel* Panel = It->GetInfoPanel())
			{
				Panel->ToggleCollapsed();
			}
		}
	}
	if (WasInputKeyJustPressed(EKeys::F2))
	{
		UGameplayStatics::OpenLevel(this, FName(UGlossaryCatalog::HubLevelPackage()));
	}
	if (WasInputKeyJustPressed(EKeys::F3))
	{
		SetCursorMode(!bShowMouseCursor);
	}
}

void AGlossaryPlayerController::SetCursorMode(bool bShowCursor)
{
	SetShowMouseCursor(bShowCursor);
	if (bShowCursor)
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Mode);
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
}
