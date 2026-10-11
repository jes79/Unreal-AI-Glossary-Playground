#include "Demo1104_Delegate.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

using namespace GlossaryKit;

// ================================================================ AD1104_Enemy (방송하는 쪽)

AD1104_Enemy::AD1104_Enemy()
{
	PrimaryActorTick.bCanEverTick = true;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	RootComponent = Body;
	InitShapeComponent(Body, EShape::Cylinder, FVector(110.f, 110.f, 180.f));
}

void AD1104_Enemy::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	SetColor(Body, Colors::Enemy);
}

void AD1104_Enemy::TakeHit(float Damage)
{
	if (!IsAlive())
	{
		return;
	}
	Health = FMath::Max(0.f, Health - Damage);
	if (Health <= 0.f)
	{
		SetColor(Body, Colors::Dark);
		SetActorRotation(FRotator(0.f, 0.f, 80.f));
		RespawnTimer = 2.5f;

		// ▼ 핵심: "죽었다"고 알리기만 한다. 누가 듣는지, 몇 명인지, 무엇을 할지는 모른다.
		OnDied.Broadcast(this);
	}
}

void AD1104_Enemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (RespawnTimer > 0.f)
	{
		RespawnTimer -= DeltaSeconds;
		if (RespawnTimer <= 0.f)
		{
			Health = MaxHealth;
			SetActorRotation(FRotator::ZeroRotator);
			SetColor(Body, Colors::Enemy);
		}
	}
}

// ================================================================ UD1104_KillBanner (UI 구독자)

void UD1104_KillBanner::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	Banner = MakeText(TEXT(""), 40.f, Colors::Item, true);
	Banner->SetJustification(ETextJustify::Center);
	// 화면 아래 가운데
	AddToCanvas(Canvas, Banner, FAnchors(0.5f, 1.f), FVector2D(0.f, -120.f), FVector2D::ZeroVector, FVector2D(0.5f, 1.f));
}

void UD1104_KillBanner::HandleDied(AActor* DeadActor)
{
	++Reactions;
	ShowTimer = 1.8f;
	if (Banner)
	{
		Banner->SetText(FText::FromString(FString::Printf(TEXT("적 처치!  (UI 반응 #%d)"), Reactions)));
		Banner->SetRenderOpacity(1.f);
	}
}

void UD1104_KillBanner::Advance(float DeltaSeconds)
{
	if (ShowTimer > 0.f && Banner)
	{
		ShowTimer -= DeltaSeconds;
		Banner->SetRenderOpacity(FMath::Clamp(ShowTimer / 0.6f, 0.f, 1.f));
	}
}

// ================================================================ AD1104_SoundPlayer (사운드 구독자)

AD1104_SoundPlayer::AD1104_SoundPlayer()
{
	PrimaryActorTick.bCanEverTick = true;
	Speaker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Speaker"));
	RootComponent = Speaker;
	InitShapeComponent(Speaker, EShape::Cone, FVector(90.f, 90.f, 110.f));
}

void AD1104_SoundPlayer::HandleDied(AActor* DeadActor)
{
	++Reactions;
	Ring = 1.f;
	if (DiedSound)
	{
		UGameplayStatics::PlaySound2D(this, DiedSound);
	}
}

void AD1104_SoundPlayer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// 소리는 스크린샷에 안 보이므로 스피커가 커졌다 작아지며 빛나는 것으로 "울렸다"를 표시
	Ring = FMath::Max(0.f, Ring - DeltaSeconds);
	SetActorScale3D(FVector(0.9f, 0.9f, 1.1f) * (1.f + 0.4f * FMath::Sin(Ring * 30.f) * Ring));
	SetColor(Speaker, Colors::Purple, 0.4f, 0.f, Ring * 8.f);
}

// ================================================================ AD1104_ScoreBoard (점수 구독자)

void AD1104_ScoreBoard::HandleDied(AActor* DeadActor)
{
	++Reactions;
	Score += 100;
	if (Label)
	{
		Label->SetText(FString::Printf(TEXT("SCORE  %d"), Score));
	}
}

// ================================================================ ADemo1104

ADemo1104_Delegate::ADemo1104_Delegate()
{
	DemoId = TEXT("1104");
	Controls = TEXT("1 : 적에게 35 피해 (3번이면 쓰러짐)\n")
		TEXT("2 / 3 / 4 : UI · 사운드 · 점수의 구독 끊기/다시 붙이기 (AddDynamic / RemoveDynamic)\n")
		TEXT("관찰: 적은 OnDied.Broadcast 만 한다. 구독을 끊은 쪽만 반응하지 않고 나머지는 그대로");
	Showcase = TEXT("적 OnDied 방송에 UI 배너·사운드·점수판이 각자 반응, 구독 끊기/붙이기 비교");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-500.f, 0.f, 100.f));
}

void ADemo1104_Delegate::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Enemy = World->SpawnActor<AD1104_Enemy>(FVector(350.f, 0.f, 90.f), FRotator::ZeroRotator, Params);
	EnemyLabel = SpawnLabel(World, TEXT(""), FVector(350.f, 0.f, 260.f), Colors::Enemy, 18.f);

	// --- 구독자 셋은 서로를, 그리고 적의 내부를 모른다. 적의 OnDied 에 자기 함수를 등록할 뿐.
	BannerUI = CreateWidget<UD1104_KillBanner>(GetPC(), UD1104_KillBanner::StaticClass());
	BannerUI->SetVisibility(ESlateVisibility::HitTestInvisible);
	BannerUI->AddToViewport(40);

	SoundPlayer = World->SpawnActor<AD1104_SoundPlayer>(FVector(250.f, -550.f, 55.f), FRotator::ZeroRotator, Params);
	SoundPlayer->DiedSound = LoadAsset<USoundBase>(TEXT("/Game/Glossary/11_Patterns/S_Died"));
	SpawnLabel(World, TEXT("사운드 구독자"), FVector(250.f, -550.f, 200.f), Colors::Purple, 15.f);

	ScoreBoard = World->SpawnActor<AD1104_ScoreBoard>(FVector(250.f, 550.f, 0.f), FRotator::ZeroRotator, Params);
	SpawnBlock(World, FVector(250.f, 550.f, 0.f), FVector(40.f, 260.f, 150.f), Colors::Dark);
	ScoreBoard->Label = SpawnLabel(World, TEXT("SCORE  0"), FVector(220.f, 550.f, 110.f), Colors::Ally, 26.f);
	SpawnLabel(World, TEXT("점수 구독자"), FVector(250.f, 550.f, 230.f), Colors::Ally, 15.f);

	// ▼ 구독: AddDynamic(받을 객체, &클래스::UFUNCTION). 함수는 반드시 UFUNCTION 이어야 한다(이름으로 찾기 때문)
	Enemy->OnDied.AddDynamic(BannerUI, &UD1104_KillBanner::HandleDied);
	Enemy->OnDied.AddDynamic(SoundPlayer, &AD1104_SoundPlayer::HandleDied);
	Enemy->OnDied.AddDynamic(ScoreBoard, &AD1104_ScoreBoard::HandleDied);
	Enemy->OnDied.AddDynamic(this, &ADemo1104_Delegate::HandleDiedForLog);

	Panel = UD11_SidePanel::Create(GetPC(), TEXT("OnDied 구독 상태"), FVector2D(1.f, 0.f), 14.f);
}

void ADemo1104_Delegate::HandleDiedForLog(AActor* DeadActor)
{
	++BroadcastCount;
	TArray<FString> Who;
	if (bSubscribed[0]) Who.Add(TEXT("UI"));
	if (bSubscribed[1]) Who.Add(TEXT("사운드"));
	if (bSubscribed[2]) Who.Add(TEXT("점수"));
	Log.Add(FString::Printf(TEXT("Broadcast #%d → 반응: %s"), BroadcastCount, Who.Num() ? *FString::Join(Who, TEXT(", ")) : TEXT("(로그만)")));
}

void ADemo1104_Delegate::ToggleListener(int32 Index)
{
	bSubscribed[Index] = !bSubscribed[Index];
	const bool bOn = bSubscribed[Index];
	// 구독/해제 코드는 듣는 쪽(혹은 조립하는 쪽)에만 있다. 적 클래스는 바뀌지 않는다.
	if (Index == 0)
	{
		if (bOn) { Enemy->OnDied.AddDynamic(BannerUI, &UD1104_KillBanner::HandleDied); }
		else     { Enemy->OnDied.RemoveDynamic(BannerUI, &UD1104_KillBanner::HandleDied); }
	}
	else if (Index == 1)
	{
		if (bOn) { Enemy->OnDied.AddDynamic(SoundPlayer, &AD1104_SoundPlayer::HandleDied); }
		else     { Enemy->OnDied.RemoveDynamic(SoundPlayer, &AD1104_SoundPlayer::HandleDied); }
	}
	else
	{
		if (bOn) { Enemy->OnDied.AddDynamic(ScoreBoard, &AD1104_ScoreBoard::HandleDied); }
		else     { Enemy->OnDied.RemoveDynamic(ScoreBoard, &AD1104_ScoreBoard::HandleDied); }
	}
	static const TCHAR* Names[] = { TEXT("UI"), TEXT("사운드"), TEXT("점수") };
	Log.Add(FString::Printf(TEXT("%s 구독 %s"), Names[Index], bOn ? TEXT("AddDynamic") : TEXT("RemoveDynamic")));
}

void ADemo1104_Delegate::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Enemy)
	{
		return;
	}
	if (WasKeyPressed(EKeys::One)) { Enemy->TakeHit(HitDamage); }
	if (WasKeyPressed(EKeys::Two)) { ToggleListener(0); }
	if (WasKeyPressed(EKeys::Three)) { ToggleListener(1); }
	if (WasKeyPressed(EKeys::Four)) { ToggleListener(2); }

	BannerUI->Advance(DeltaSeconds);
	EnemyLabel->SetText(Enemy->IsAlive() ? FString::Printf(TEXT("적 HP %.0f / %.0f"), Enemy->GetHealth(), Enemy->MaxHealth) : TEXT("쓰러짐 → OnDied 방송"));

	if (Panel)
	{
		auto Row = [](const TCHAR* Name, bool bOn, int32 Count)
		{
			return FString::Printf(TEXT("  %s %-6s 반응 %d회\n"), bOn ? TEXT("[ON ]") : TEXT("[OFF]"), Name, Count);
		};
		FString Body = TEXT("Enemy->OnDied 구독자\n");
		Body += Row(TEXT("UI"), bSubscribed[0], BannerUI->Reactions);
		Body += Row(TEXT("사운드"), bSubscribed[1], SoundPlayer->Reactions);
		Body += Row(TEXT("점수"), bSubscribed[2], ScoreBoard->Reactions);
		Body += FString::Printf(TEXT("  IsBound() = %s\n\n"), Enemy->OnDied.IsBound() ? TEXT("true") : TEXT("false"));
		Body += TEXT("적 코드: OnDied.Broadcast(this);\n구독: OnDied.AddDynamic(UI, &UD1104_KillBanner::HandleDied);\n\n");
		Body += Log.Join();
		Panel->SetBody(Body);
	}
	SetStatus(FString::Printf(TEXT("방송 %d회 · 점수 %d\n적은 UI·사운드·점수 클래스를 include 하지도 않는다"), BroadcastCount, ScoreBoard->Score));
}
