#include "D08_Common.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- UD08_SidePanel

UD08_SidePanel* UD08_SidePanel::Create(APlayerController* PC, const FString& Title, EPlace Place, float Width)
{
	if (!PC)
	{
		return nullptr;
	}
	UD08_SidePanel* Panel = CreateWidget<UD08_SidePanel>(PC, UD08_SidePanel::StaticClass());
	Panel->Place = Place;
	Panel->Width = Width;
	Panel->PendingTitle = Title;
	Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
	Panel->AddToViewport(50);
	return Panel;
}

void UD08_SidePanel::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UBorder* Box = MakeBorder(FLinearColor(0.02f, 0.03f, 0.05f, 0.8f), FMargin(14.f, 10.f));

	switch (Place)
	{
	case EPlace::TopRight:     AddToCanvas(Canvas, Box, FAnchors(1.f, 0.f), FVector2D(-24.f, 24.f), FVector2D(Width, 0.f), FVector2D(1.f, 0.f)); break;
	case EPlace::BottomRight:  AddToCanvas(Canvas, Box, FAnchors(1.f, 1.f), FVector2D(-24.f, -24.f), FVector2D(Width, 0.f), FVector2D(1.f, 1.f)); break;
	case EPlace::BottomCenter: AddToCanvas(Canvas, Box, FAnchors(0.5f, 1.f), FVector2D(0.f, -40.f), FVector2D(Width, 0.f), FVector2D(0.5f, 1.f)); break;
	case EPlace::Center:       AddToCanvas(Canvas, Box, FAnchors(0.5f, 0.5f), FVector2D(0.f, 0.f), FVector2D(Width, 0.f), FVector2D(0.5f, 0.5f)); break;
	}
	// 높이는 내용에 맞춘다(가로만 고정)
	if (UCanvasPanelSlot* BoxSlot = Cast<UCanvasPanelSlot>(Box->Slot))
	{
		BoxSlot->SetAutoSize(true);
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Box->SetContent(Column);

	TitleText = MakeText(PendingTitle, 18.f, FLinearColor(1.f, 0.85f, 0.35f), true);
	Column->AddChildToVerticalBox(TitleText);

	Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
	Bar->SetPercent(FMath::Max(0.f, PendingPercent));
	Bar->SetFillColorAndOpacity(PendingColor);
	Bar->SetVisibility(PendingPercent < 0.f ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	if (UVerticalBoxSlot* BarSlot = Column->AddChildToVerticalBox(Bar))
	{
		BarSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 2.f));
	}

	BodyText = MakeText(PendingBody, 15.f, FLinearColor(0.95f, 0.95f, 0.95f));
	BodyText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* BodySlot = Column->AddChildToVerticalBox(BodyText))
	{
		BodySlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	}
	// AutoSize 가 가로 폭을 정하지 않으므로 줄바꿈 폭을 직접 준다
	BodyText->SetWrapTextAt(Width - 28.f);
	TitleText->SetAutoWrapText(true);
	TitleText->SetWrapTextAt(Width - 28.f);
	Bar->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
}

void UD08_SidePanel::SetTitle(const FString& InTitle)
{
	PendingTitle = InTitle;
	if (TitleText)
	{
		TitleText->SetText(FText::FromString(InTitle));
	}
}

void UD08_SidePanel::SetBody(const FString& InBody)
{
	PendingBody = InBody;
	if (BodyText)
	{
		BodyText->SetText(FText::FromString(InBody));
	}
}

void UD08_SidePanel::SetProgress(float Percent, const FLinearColor& Color)
{
	PendingPercent = Percent;
	PendingColor = Color;
	if (Bar)
	{
		Bar->SetVisibility(Percent < 0.f ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		Bar->SetPercent(FMath::Clamp(Percent, 0.f, 1.f));
		Bar->SetFillColorAndOpacity(Color);
	}
}

// ---------------------------------------------------------------- AD08_FloatingText

AD08_FloatingText::AD08_FloatingText()
{
	PrimaryActorTick.bCanEverTick = true;
}

AD08_FloatingText* AD08_FloatingText::Spawn(UWorld* World, const FString& Text, const FVector& Location,
	const FLinearColor& Color, float FontSize, float LifeTime)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AD08_FloatingText* Label = World->SpawnActor<AD08_FloatingText>(Location, FRotator::ZeroRotator, Params);
	Label->SetText(Text);
	Label->SetColor(Color);
	Label->SetFontSize(FontSize);
	Label->SetLifeSpan(LifeTime);
	return Label;
}

void AD08_FloatingText::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AddActorWorldOffset(FVector(0.f, 0.f, RiseSpeed * DeltaSeconds));
}

// ---------------------------------------------------------------- AD08_Dummy

AD08_Dummy::AD08_Dummy()
{
	PrimaryActorTick.bCanEverTick = true;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	RootComponent = Body;
	InitShapeComponent(Body, EShape::Cylinder, FVector(90.f, 90.f, 180.f));
}

void AD08_Dummy::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	BodyMID = SetColor(Body, BaseColor);
	HpLabel = SpawnLabel(GetWorld(), TEXT(""), GetActorLocation() + FVector(0.f, 0.f, 150.f), FLinearColor::White, 18.f);
	HpLabel->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	RefreshLabel();
}

void AD08_Dummy::EndPlay(const EEndPlayReason::Type Reason)
{
	if (HpLabel)
	{
		HpLabel->Destroy();
	}
	Super::EndPlay(Reason);
}

void AD08_Dummy::ApplyHit(float Amount, const FLinearColor& Color, const FString& Note)
{
	if (IsDead())
	{
		return;
	}
	Health = FMath::Max(0.f, Health - Amount);
	DamageLog.Add(TPair<float, float>(GetWorld()->GetTimeSeconds(), Amount));
	const FString Text = Note.IsEmpty() ? FString::Printf(TEXT("-%.0f"), Amount) : FString::Printf(TEXT("-%.0f  %s"), Amount, *Note);
	const FVector Jitter(FMath::FRandRange(-30.f, 30.f), FMath::FRandRange(-30.f, 30.f), 0.f);
	AD08_FloatingText::Spawn(GetWorld(), Text, GetActorLocation() + FVector(0.f, 0.f, 120.f) + Jitter, Color, 24.f);
	Flash(Color);
	if (IsDead())
	{
		DeadTimer = RespawnDelay;
		SetActorRotation(FRotator(0.f, 0.f, 70.f));
	}
	RefreshLabel();
}

void AD08_Dummy::ResetHealth()
{
	Health = MaxHealth;
	SetActorRotation(FRotator::ZeroRotator);
	RefreshLabel();
}

void AD08_Dummy::Flash(const FLinearColor& Color, float Duration)
{
	SetBodyColor(Color, 3.f);
	FlashTimer = Duration;
}

void AD08_Dummy::SetBodyColor(const FLinearColor& Color, float Emissive)
{
	if (BodyMID)
	{
		BodyMID->SetVectorParameterValue(TEXT("Color"), Color);
		BodyMID->SetScalarParameterValue(TEXT("Emissive"), Emissive);
	}
}

float AD08_Dummy::GetRecentDamage(float Window) const
{
	const float Now = GetWorld()->GetTimeSeconds();
	float Sum = 0.f;
	for (const TPair<float, float>& Entry : DamageLog)
	{
		if (Now - Entry.Key <= Window)
		{
			Sum += Entry.Value;
		}
	}
	return Sum;
}

void AD08_Dummy::RefreshLabel()
{
	if (HpLabel)
	{
		HpLabel->SetText(FString::Printf(TEXT("%s  HP %.0f / %.0f"), *DisplayName, Health, MaxHealth));
		HpLabel->SetColor(IsDead() ? FLinearColor(1.f, 0.4f, 0.4f) : FLinearColor::White);
	}
}

void AD08_Dummy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (FlashTimer > 0.f)
	{
		FlashTimer -= DeltaSeconds;
		if (FlashTimer <= 0.f)
		{
			SetBodyColor(BaseColor, 0.f);
		}
	}
	if (DeadTimer > 0.f)
	{
		DeadTimer -= DeltaSeconds;
		if (DeadTimer <= 0.f)
		{
			ResetHealth();
		}
	}
	// 오래된 기록 정리
	const float Now = GetWorld()->GetTimeSeconds();
	DamageLog.RemoveAll([Now](const TPair<float, float>& Entry) { return Now - Entry.Key > 10.f; });
}

// ---------------------------------------------------------------- AD08_Projectile

AD08_Projectile::AD08_Projectile()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	InitShapeComponent(Mesh, EShape::Sphere, FVector(20.f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
}

AD08_Projectile* AD08_Projectile::Launch(UWorld* World, const FVector& From, AActor* Target, const FLinearColor& Color,
	float Size, float Speed, TFunction<void(AActor*)> OnArrive)
{
	if (!World || !Target)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AD08_Projectile* P = World->SpawnActor<AD08_Projectile>(From, FRotator::ZeroRotator, Params);
	P->SetActorScale3D(FVector(Size / 20.f));
	SetColor(P->Mesh, Color, 0.4f, 0.f, 4.f);
	P->Target = Target;
	P->Speed = Speed;
	P->OnArrive = MoveTemp(OnArrive);
	P->SetLifeSpan(5.f);
	return P;
}

void AD08_Projectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AActor* T = Target.Get();
	if (!T)
	{
		Destroy();
		return;
	}
	const FVector Goal = T->GetActorLocation() + FVector(0.f, 0.f, 40.f);
	const FVector ToGoal = Goal - GetActorLocation();
	const float Step = Speed * DeltaSeconds;
	if (ToGoal.Size() <= Step)
	{
		if (OnArrive)
		{
			OnArrive(T);
		}
		Destroy();
		return;
	}
	AddActorWorldOffset(ToGoal.GetSafeNormal() * Step);
}

// ---------------------------------------------------------------- D08

FString D08::Bar(float Percent, int32 Cells)
{
	const int32 Filled = FMath::Clamp(FMath::RoundToInt(Percent * Cells), 0, Cells);
	FString Out;
	for (int32 i = 0; i < Cells; ++i)
	{
		Out += i < Filled ? TEXT("■") : TEXT("□");
	}
	return Out;
}

void AD08_Dummy::Configure(const FString& InName, float InMaxHealth, const FLinearColor& InColor)
{
	DisplayName = InName;
	MaxHealth = InMaxHealth;
	BaseColor = InColor;
	SetBodyColor(BaseColor);
	ResetHealth();
}
