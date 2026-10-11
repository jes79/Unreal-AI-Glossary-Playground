#include "Demo0606_WidgetComponent.h"

#include "Blueprint/WidgetTree.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "GlossaryUIKit.h"
#include "Kismet/GameplayStatics.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- 체력바 위젯

void UDemo0606_EnemyBar::BuildTree()
{
	UBorder* Root = MakeBorder(FLinearColor(0.f, 0.f, 0.f, 0.55f), FMargin(4.f, 3.f));
	WidgetTree->RootWidget = Root;
	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Root->SetContent(Col);
	Bar = GlossaryUIKit::MakeBar(WidgetTree, FLinearColor(0.9f, 0.1f, 0.08f), FLinearColor(0.1f, 0.1f, 0.1f, 1.f));
	Col->AddChildToVerticalBox(GlossaryUIKit::WrapSize(WidgetTree, Bar, 150.f, 14.f));
	Info = MakeText(TEXT(""), 12.f, FLinearColor::White);
	Info->SetJustification(ETextJustify::Center);
	Col->AddChildToVerticalBox(Info);
}

void UDemo0606_EnemyBar::SetValues(float Percent, float DistanceMeters)
{
	if (Bar)
	{
		Bar->SetPercent(Percent);
		Info->SetText(FText::FromString(FString::Printf(TEXT("%.0f m"), DistanceMeters)));
	}
}

// ---------------------------------------------------------------- 적

ADemo0606_Enemy::ADemo0606_Enemy()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(RootComponent);
	InitShapeComponent(Body, EShape::Cylinder, FVector(90.f, 90.f, 180.f));

	HealthBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));
	HealthBarWidget->SetupAttachment(RootComponent);
	// 머리 위: 몸통 중심(높이 180 의 가운데)에서 +90 이 머리, 그 위 40cm
	HealthBarWidget->SetRelativeLocation(FVector(0.f, 0.f, 130.f));
	HealthBarWidget->SetWidgetSpace(EWidgetSpace::Screen);      // Space: Screen
	HealthBarWidget->SetWidgetClass(UDemo0606_EnemyBar::StaticClass());
	HealthBarWidget->SetDrawSize(FVector2D(160.f, 36.f));
	HealthBarWidget->SetPivot(FVector2D(0.5f, 1.f));           // 바의 아래 가운데가 머리 위 지점
}

void ADemo0606_Enemy::BeginPlay()
{
	Super::BeginPlay();
	SetColor(Body, Colors::Enemy);
	Origin = GetActorLocation();
}

void ADemo0606_Enemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	// 앞뒤로 왕복 (가까이 ↔ 멀리) 해서 숨김 거리를 넘나든다
	SetActorLocation(Origin + FVector(FMath::Sin(Time * 0.45f + Phase) * 1500.f, 0.f, 0.f));
	Health = 0.25f + 0.75f * (0.5f + 0.5f * FMath::Cos(Time * 0.7f + Phase * 2.f));

	// 카메라와의 거리로 숨김 판정
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		LastDistance = FVector::Dist(Cam->GetCameraLocation(), GetActorLocation());
	}
	bBarVisible = !bHideByDistance || LastDistance <= HideDistance;
	HealthBarWidget->SetVisibility(bBarVisible);
	if (UDemo0606_EnemyBar* BarWidget = Cast<UDemo0606_EnemyBar>(HealthBarWidget->GetUserWidgetObject()))
	{
		BarWidget->SetValues(Health, LastDistance / 100.f);
	}
}

// ---------------------------------------------------------------- 데모

ADemo0606_WidgetComponent::ADemo0606_WidgetComponent()
{
	DemoId = TEXT("0606");
	Controls = TEXT("WASD 이동 · 마우스 시점   C : 거리 숨김 ON/OFF\n")
		TEXT("관찰: 적이 15m 보다 멀어지면 머리 위 체력바가 사라진다. Screen 공간이라 멀어도 바 크기는 같다");
	Showcase = TEXT("적 머리 위 WidgetComponent(Screen) 체력바, 15m 밖이면 숨김 ON/OFF");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-900.f, 0.f, 100.f));
}

void ADemo0606_WidgetComponent::BeginPlay()
{
	Super::BeginPlay();
	const float Ys[] = { -600.f, -200.f, 200.f, 600.f };
	for (int32 i = 0; i < 4; ++i)
	{
		FActorSpawnParameters Params;
		ADemo0606_Enemy* Enemy = GetWorld()->SpawnActor<ADemo0606_Enemy>(FVector(1300.f, Ys[i], 90.f), FRotator::ZeroRotator, Params);
		Enemy->Phase = i * 1.7f;
		Enemy->HideDistance = HideDistance;
		Enemies.Add(Enemy);
	}
	// 숨김 경계를 바닥에 표시 (플레이어 시작점 기준 15m)
	SpawnShape(GetWorld(), EShape::Cube, FVector(-900.f + HideDistance, 0.f, 2.f), FVector(20.f, 1600.f, 4.f), Colors::Item);
	SpawnLabel(GetWorld(), TEXT("시작점에서 15m — 이보다 멀면 체력바 숨김"), FVector(-900.f + HideDistance, 0.f, 60.f), Colors::Item, 16.f);
}

void ADemo0606_WidgetComponent::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::C))
	{
		bHideByDistance = !bHideByDistance;
	}
	int32 Visible = 0;
	FString Dist;
	for (ADemo0606_Enemy* Enemy : Enemies)
	{
		Enemy->bHideByDistance = bHideByDistance;
		Enemy->HideDistance = HideDistance;
		Visible += Enemy->bBarVisible ? 1 : 0;
		Dist += FString::Printf(TEXT("%.0fm%s "), Enemy->LastDistance / 100.f, Enemy->bBarVisible ? TEXT("") : TEXT("(숨김)"));
	}
	SetStatus(FString::Printf(TEXT("거리 숨김 %s (HideDistance %.0fcm) · 보이는 체력바 %d / %d\n%s"),
		bHideByDistance ? TEXT("ON") : TEXT("OFF"), HideDistance, Visible, Enemies.Num(), *Dist));
}
