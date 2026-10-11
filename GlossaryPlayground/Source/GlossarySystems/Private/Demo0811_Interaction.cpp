#include "Demo0811_Interaction.h"

#include "Components/StaticMeshComponent.h"
#include "D08_Common.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryLabel.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace GlossaryKit;

// Interact 트레이스 채널 (DefaultEngine.ini: ECC_GameTraceChannel2 = Interact)
static constexpr ECollisionChannel InteractChannel = ECC_GameTraceChannel2;

// ---------------------------------------------------------------- 공통 부모

AD0811_InteractableBase::AD0811_InteractableBase()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void AD0811_InteractableBase::BeginPlay()
{
	Super::BeginPlay();
}

UStaticMeshComponent* AD0811_InteractableBase::AddPart(EShape Shape, const FVector& Location, const FVector& Size, const FLinearColor& Color, USceneComponent* Parent)
{
	UStaticMeshComponent* Part = AddShape(this, Shape, Location, Size, Color, Parent);
	// 트레이스에 걸리도록 Interact 채널을 Block
	Part->SetCollisionResponseToChannel(InteractChannel, ECR_Block);
	// 테두리 후처리용 Custom Depth Stencil (값 1). 후처리 머티리얼이 이 마스크로 외곽선을 그린다
	Part->SetCustomDepthStencilValue(1);
	Parts.Add(Part);
	PartColors.Add(Color);
	return Part;
}

void AD0811_InteractableBase::SetFocused(bool bFocused)
{
	bIsFocused = bFocused;
	for (int32 i = 0; i < Parts.Num(); ++i)
	{
		Parts[i]->SetRenderCustomDepth(bFocused);
		// 하이라이트: 밝은 노랑 발광으로 바꾼다 (Custom Depth 외곽선 후처리가 없을 때의 간단한 표시)
		SetColor(Parts[i], bFocused ? FMath::Lerp(PartColors[i], Colors::Item, 0.5f) : PartColors[i], 0.5f, 0.f, bFocused ? 1.2f : 0.f);
	}
}

// ---------------------------------------------------------------- 문

AD0811_Door::AD0811_Door() {}

void AD0811_Door::BeginPlay()
{
	Super::BeginPlay();
	// 문틀 (하이라이트 대상 아님)
	AddShape(this, EShape::Cube, FVector(0.f, -130.f, 160.f), FVector(30.f, 30.f, 320.f), Colors::Wall);
	AddShape(this, EShape::Cube, FVector(0.f, 130.f, 160.f), FVector(30.f, 30.f, 320.f), Colors::Wall);
	AddShape(this, EShape::Cube, FVector(0.f, 0.f, 330.f), FVector(30.f, 290.f, 30.f), Colors::Wall);
	// 경첩을 축으로 도는 문짝
	Hinge = NewObject<USceneComponent>(this, TEXT("Hinge"));
	Hinge->SetupAttachment(Root);
	Hinge->SetRelativeLocation(FVector(0.f, -115.f, 0.f));
	Hinge->RegisterComponent();
	AddPart(EShape::Cube, FVector(0.f, 115.f, 155.f), FVector(16.f, 230.f, 310.f), FLinearColor(0.45f, 0.25f, 0.1f), Hinge);
	AddPart(EShape::Sphere, FVector(-14.f, 200.f, 150.f), FVector(16.f), Colors::Item, Hinge);
}

void AD0811_Door::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Angle = FMath::FInterpTo(Angle, bOpen ? 100.f : 0.f, DeltaSeconds, 5.f);
	Hinge->SetRelativeRotation(FRotator(0.f, Angle, 0.f));
}

// ---------------------------------------------------------------- 상자

AD0811_Chest::AD0811_Chest() {}

void AD0811_Chest::BeginPlay()
{
	Super::BeginPlay();
	AddPart(EShape::Cube, FVector(0.f, 0.f, 45.f), FVector(110.f, 160.f, 90.f), FLinearColor(0.5f, 0.3f, 0.1f));
	LidHinge = NewObject<USceneComponent>(this, TEXT("LidHinge"));
	LidHinge->SetupAttachment(Root);
	LidHinge->SetRelativeLocation(FVector(55.f, 0.f, 90.f));
	LidHinge->RegisterComponent();
	AddPart(EShape::Cube, FVector(-55.f, 0.f, 12.f), FVector(110.f, 160.f, 24.f), FLinearColor(0.6f, 0.38f, 0.12f), LidHinge);
	AddPart(EShape::Cube, FVector(-110.f, 0.f, 0.f), FVector(8.f, 30.f, 30.f), Colors::Item, LidHinge);
}

void AD0811_Chest::Interact(AActor* InteractInstigator)
{
	if (bOpen)
	{
		AD08_FloatingText::Spawn(GetWorld(), TEXT("비어 있다"), GetActorLocation() + FVector(0, 0, 160), Colors::Neutral, 22.f);
		return;
	}
	bOpen = true;
	AD08_FloatingText::Spawn(GetWorld(), TEXT("+100 골드!"), GetActorLocation() + FVector(0, 0, 160), Colors::Item, 28.f, 2.f);
}

void AD0811_Chest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Angle = FMath::FInterpTo(Angle, bOpen ? -110.f : 0.f, DeltaSeconds, 4.f);
	LidHinge->SetRelativeRotation(FRotator(Angle, 0.f, 0.f));
}

// ---------------------------------------------------------------- NPC

AD0811_NPC::AD0811_NPC() {}

void AD0811_NPC::BeginPlay()
{
	Super::BeginPlay();
	AddPart(EShape::Cylinder, FVector(0.f, 0.f, 80.f), FVector(70.f, 70.f, 160.f), Colors::Ally);
	AddPart(EShape::Sphere, FVector(0.f, 0.f, 190.f), FVector(60.f), FLinearColor(0.95f, 0.8f, 0.65f));
	Speech = SpawnLabel(GetWorld(), TEXT("..."), GetActorLocation() + FVector(0.f, 0.f, 270.f), FLinearColor::White, 20.f);
}

void AD0811_NPC::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Speech)
	{
		Speech->Destroy();
	}
	Super::EndPlay(Reason);
}

void AD0811_NPC::Interact(AActor* InteractInstigator)
{
	static const TCHAR* Lines[] = {
		TEXT("안녕, 여행자!"),
		TEXT("저 문은 E 로 열 수 있어."),
		TEXT("상자도 같은 E 키로 열리지."),
		TEXT("우리 셋 다 같은 인터페이스를 쓰거든."),
	};
	Speech->SetText(Lines[LineIndex % UE_ARRAY_COUNT(Lines)]);
	++LineIndex;
}

// ---------------------------------------------------------------- 데모

ADemo0811_Interaction::ADemo0811_Interaction()
{
	DemoId = TEXT("0811");
	Controls = TEXT("WASD 이동 · 마우스로 화면 가운데(+)를 문·상자·NPC 에 맞춘다 · E : 상호작용\n")
		TEXT("관찰: 보고 있는 물체만 노랗게 강조되고 아래에 'E : …' 안내가 뜬다. 셋은 다른 클래스지만 같은 Interact 호출");
	Showcase = TEXT("카메라 Line Trace 로 바라보는 문·상자·NPC 를 강조하고 E 로 Interactable 인터페이스 Interact 호출");
}

void ADemo0811_Interaction::BeginPlay()
{
	Super::BeginPlay();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// 플레이어(-600,0) 정면에 문, 좌우에 상자와 NPC
	GetWorld()->SpawnActor<AD0811_Door>(FVector(-150.f, 0.f, 0.f), FRotator::ZeroRotator, Params);
	GetWorld()->SpawnActor<AD0811_Chest>(FVector(-150.f, -450.f, 0.f), FRotator::ZeroRotator, Params);
	GetWorld()->SpawnActor<AD0811_NPC>(FVector(-150.f, 450.f, 0.f), FRotator(0.f, 180.f, 0.f), Params);

	Prompt = UD08_SidePanel::Create(GetPC(), TEXT(""), UD08_SidePanel::EPlace::BottomCenter, 300.f);
	Prompt->SetVisibility(ESlateVisibility::Collapsed);
	Crosshair = UD08_SidePanel::Create(GetPC(), TEXT("+"), UD08_SidePanel::EPlace::Center, 40.f);
}

void ADemo0811_Interaction::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APlayerController* PC = GetPC();
	if (!PC)
	{
		return;
	}
	// 1) 카메라 위치·방향에서 Line Trace
	FVector ViewLoc;
	FRotator ViewRot;
	PC->GetPlayerViewPoint(ViewLoc, ViewRot);
	const FVector End = ViewLoc + ViewRot.Vector() * TraceDistance;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(Interact), false);
	if (APawn* Pawn = GetPlayerPawn())
	{
		Query.AddIgnoredActor(Pawn);
	}
	FHitResult Hit;
	AActor* NewFocus = nullptr;
	if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLoc, End, InteractChannel, Query))
	{
		// 2) 맞은 액터가 인터페이스를 구현했는지 (클래스 종류와 무관)
		if (Hit.GetActor() && Hit.GetActor()->Implements<UD0811_Interactable>())
		{
			NewFocus = Hit.GetActor();
		}
	}

	// 3) 포커스가 바뀌면 이전 것은 강조 해제, 새 것은 강조
	if (NewFocus != Focused.Get())
	{
		if (ID0811_Interactable* Old = Cast<ID0811_Interactable>(Focused.Get()))
		{
			Old->SetFocused(false);
		}
		if (ID0811_Interactable* New = Cast<ID0811_Interactable>(NewFocus))
		{
			New->SetFocused(true);
		}
		Focused = NewFocus;
	}

	ID0811_Interactable* Target = Cast<ID0811_Interactable>(Focused.Get());
	// 4) E → Interact (어떤 클래스인지 몰라도 된다)
	if (Target && WasKeyPressed(EKeys::E))
	{
		LastInteraction = FString::Printf(TEXT("%s → Interact() 호출"), *Focused->GetClass()->GetName());
		Target->Interact(GetPlayerPawn());
	}

	if (Prompt)
	{
		Prompt->SetVisibility(Target ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (Target)
		{
			Prompt->SetTitle(FString::Printf(TEXT("E : %s"), *Target->GetPrompt()));
		}
	}
	SetStatus(FString::Printf(TEXT("트레이스 %.0fcm · 보고 있는 것: %s\n%s"), TraceDistance,
		Target ? *Focused->GetClass()->GetName() : TEXT("없음"), *LastInteraction));
}
