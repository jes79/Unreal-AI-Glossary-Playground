#include "GlossaryDemoBase.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryCharacter.h"
#include "GlossaryWidgets.h"
#include "Kismet/GameplayStatics.h"

AGlossaryDemoBase::AGlossaryDemoBase()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	PlayerPawnClass = AGlossaryCharacter::StaticClass();
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-600.f, 0.f, 100.f));
}

const FGlossaryEntry* AGlossaryDemoBase::GetEntry() const
{
	return UGlossaryCatalog::Find(DemoId);
}

void AGlossaryDemoBase::BeginPlay()
{
	Super::BeginPlay();
	CreateInfoPanel();

	if (APlayerController* PC = GetPC())
	{
		if (bShowMouseCursor)
		{
			PC->SetShowMouseCursor(true);
			FInputModeGameAndUI Mode;
			Mode.SetHideCursorDuringCapture(false);
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(Mode);
		}
	}
}

void AGlossaryDemoBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (InfoPanel)
	{
		InfoPanel->RemoveFromParent();
		InfoPanel = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AGlossaryDemoBase::CreateInfoPanel()
{
	APlayerController* PC = GetPC();
	if (!PC)
	{
		return;
	}
	InfoPanel = CreateWidget<UGlossaryInfoPanel>(PC, UGlossaryInfoPanel::StaticClass());
	// 패널은 클릭을 막지 않는다(데모 UI 와 월드 클릭이 그대로 동작하도록)
	InfoPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	InfoPanel->AddToViewport(100);

	FString Title = DemoId, Plain, Request;
	if (const FGlossaryEntry* Entry = GetEntry())
	{
		Title = FString::Printf(TEXT("%s  %s"), *Entry->Id, *Entry->Term);
		Plain = Entry->Plain;
		Request = Entry->Request;
	}
	FString Impl;
	if (Implementation == EGlossaryImplementation::Alternative)
	{
		Impl = AlternativeNote.IsEmpty() ? TEXT("설명 + 대체 구현") : FString::Printf(TEXT("대체 구현: %s"), *AlternativeNote);
	}
	else if (Implementation == EGlossaryImplementation::EditorTool)
	{
		Impl = AlternativeNote.IsEmpty() ? TEXT("에디터 도구 데모") : FString::Printf(TEXT("에디터 도구: %s"), *AlternativeNote);
	}
	InfoPanel->Setup(Title, Plain, Request, Controls, Impl);
}

void AGlossaryDemoBase::SetStatus(const FString& Status)
{
	if (InfoPanel)
	{
		InfoPanel->SetStatus(Status);
	}
}

APlayerController* AGlossaryDemoBase::GetPC() const
{
	return UGameplayStatics::GetPlayerController(this, 0);
}

APawn* AGlossaryDemoBase::GetPlayerPawn() const
{
	APlayerController* PC = GetPC();
	return PC ? PC->GetPawn() : nullptr;
}

ACharacter* AGlossaryDemoBase::GetPlayerCharacter() const
{
	return Cast<ACharacter>(GetPlayerPawn());
}

bool AGlossaryDemoBase::WasKeyPressed(const FKey& Key) const
{
	APlayerController* PC = GetPC();
	return PC && PC->WasInputKeyJustPressed(Key);
}

bool AGlossaryDemoBase::WasKeyReleased(const FKey& Key) const
{
	APlayerController* PC = GetPC();
	return PC && PC->WasInputKeyJustReleased(Key);
}

bool AGlossaryDemoBase::IsKeyDown(const FKey& Key) const
{
	APlayerController* PC = GetPC();
	return PC && PC->IsInputKeyDown(Key);
}

bool AGlossaryDemoBase::GetMouseWorldHit(FHitResult& OutHit, ECollisionChannel Channel) const
{
	APlayerController* PC = GetPC();
	if (!PC)
	{
		return false;
	}
	if (PC->bShowMouseCursor)
	{
		return PC->GetHitResultUnderCursor(Channel, false, OutHit);
	}
	int32 SizeX = 0, SizeY = 0;
	PC->GetViewportSize(SizeX, SizeY);
	return PC->GetHitResultAtScreenPosition(FVector2D(SizeX * 0.5f, SizeY * 0.5f), Channel, false, OutHit);
}
