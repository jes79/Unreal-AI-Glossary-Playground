#include "Demo1109_ConsoleVariable.h"

#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

using namespace GlossaryKit;

// ================================================================ 콘솔 변수 · 콘솔 명령 (전역, 모듈이 로드될 때 등록)

/**
 * ~ 콘솔에서 "game.PlayerSpeed 900" 처럼 바꾸는 값. 값만 치면 현재 값을 보여 준다("game.PlayerSpeed").
 * ECVF_Cheat: Shipping 빌드에서는 바꿀 수 없다. DefaultEngine.ini [ConsoleVariables] 나 -ini 로 시작값을 줄 수도 있다.
 */
static TAutoConsoleVariable<float> CVarPlayerSpeed(
	TEXT("game.PlayerSpeed"),
	500.f,
	TEXT("플레이어 최대 걷기 속도 (cm/s). 1109 데모가 매 프레임 읽어 CharacterMovement 에 반영한다."),
	ECVF_Cheat);

/** Exec 와 달리 어느 클래스에도 속하지 않는 전역 명령. 월드가 필요하면 WithWorld 버전을 쓴다. */
static void HealPlayer(UWorld* World)
{
	if (AD1109_CheatCharacter* Player = Cast<AD1109_CheatCharacter>(UGameplayStatics::GetPlayerPawn(World, 0)))
	{
		Player->Health = 100.f;
		UE_LOG(LogGlossaryPatterns, Log, TEXT("game.Heal: 체력 100"));
	}
}
static FAutoConsoleCommandWithWorld CmdHeal(
	TEXT("game.Heal"),
	TEXT("1109 데모 플레이어 체력을 100 으로 회복"),
	FConsoleCommandWithWorldDelegate::CreateStatic(&HealPlayer),
	ECVF_Cheat);

// ================================================================ AD1109_CheatCharacter

AD1109_CheatCharacter::AD1109_CheatCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AD1109_CheatCharacter::GodMode()
{
	bGodMode = !bGodMode;
	UE_LOG(LogGlossaryPatterns, Log, TEXT("GodMode %s"), bGodMode ? TEXT("ON") : TEXT("OFF"));
}

void AD1109_CheatCharacter::SetHealth(float NewHealth)
{
	Health = FMath::Clamp(NewHealth, 1.f, 100.f);
}

void AD1109_CheatCharacter::ReceiveDamage(float Amount)
{
	if (bGodMode)
	{
		++BlockedHits; // 무적: 피해 무시
		return;
	}
	Health -= Amount;
	if (Health <= 0.f)
	{
		Health = 100.f; // 쓰러지면 바로 다시 시작
	}
}

// ================================================================ ADemo1109

ADemo1109_ConsoleVariable::ADemo1109_ConsoleVariable()
{
	DemoId = TEXT("1109");
	Controls = TEXT("1 / 2 / 3 : 콘솔 'game.PlayerSpeed 200 / 600 / 1500'   G : 'GodMode'   H : 'game.Heal'\n")
		TEXT("~ 키로 콘솔을 열어 직접 쳐도 같다 (예: game.PlayerSpeed 900, SetHealth 30). WASD 로 달려 속도 확인\n")
		TEXT("관찰: 독 구름이 1초마다 20 피해 — GodMode 를 켜면 HP 가 줄지 않고 '막은 피해' 가 는다");
	Showcase = TEXT("콘솔 변수 game.PlayerSpeed 로 이동 속도, Exec 함수 GodMode 로 무적 치트");
	PlayerPawnClass = AD1109_CheatCharacter::StaticClass();
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-600.f, 0.f, 100.f));
}

void ADemo1109_ConsoleVariable::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	// 독 구름(연출) + 속도 측정용 트랙
	AActor* Cloud = SpawnShape(World, EShape::Sphere, FVector(-100.f, 500.f, 260.f), FVector(400.f, 400.f, 160.f), Colors::Purple);
	Cloud->SetActorEnableCollision(false);
	SpawnLabel(World, TEXT("독 구름: 1초마다 20 피해"), FVector(-100.f, 500.f, 400.f), Colors::Purple, 16.f);
	for (int32 i = 0; i < 6; ++i)
	{
		SpawnBlock(World, FVector(i * 500.f, -350.f, 0.f), FVector(20.f, 20.f, 120.f), Colors::Wall);
		SpawnLabel(World, FString::Printf(TEXT("%dm"), i * 5 + 6), FVector(i * 500.f, -350.f, 170.f), FLinearColor::White, 14.f);
	}
	HealthLabel = SpawnLabel(World, TEXT(""), FVector::ZeroVector, Colors::Ally, 18.f);
	Panel = UD11_SidePanel::Create(GetPC(), TEXT("콘솔"), FVector2D(1.f, 0.f), 14.f);
	Log.Add(TEXT("시작: game.PlayerSpeed = 500 (기본값)"));
}

void ADemo1109_ConsoleVariable::RunConsole(const FString& Command)
{
	Log.Add(TEXT("> ") + Command);
	if (APlayerController* PC = GetPC())
	{
		// 콘솔 창에 입력한 것과 같은 경로: 콘솔 변수 → Exec 함수(PC·Pawn·HUD·GameMode·CheatManager…) 순으로 처리
		PC->ConsoleCommand(Command, /*bWriteToLog*/ true);
	}
}

void ADemo1109_ConsoleVariable::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (WasKeyPressed(EKeys::One))   { RunConsole(TEXT("game.PlayerSpeed 200")); }
	if (WasKeyPressed(EKeys::Two))   { RunConsole(TEXT("game.PlayerSpeed 600")); }
	if (WasKeyPressed(EKeys::Three)) { RunConsole(TEXT("game.PlayerSpeed 1500")); }
	if (WasKeyPressed(EKeys::G))     { RunConsole(TEXT("GodMode")); }
	if (WasKeyPressed(EKeys::H))     { RunConsole(TEXT("game.Heal")); }

	AD1109_CheatCharacter* Player = Cast<AD1109_CheatCharacter>(GetPlayerPawn());
	if (!Player)
	{
		return;
	}

	// ▼ 콘솔 변수 읽기: 게임 스레드에서는 GetValueOnGameThread(). 값이 바뀌었을 때만 적용(로그용)
	const float Speed = CVarPlayerSpeed.GetValueOnGameThread();
	if (Speed != LastAppliedSpeed)
	{
		Player->GetCharacterMovement()->MaxWalkSpeed = Speed;
		if (LastAppliedSpeed >= 0.f)
		{
			Log.Add(FString::Printf(TEXT("CVar 변경 감지 → MaxWalkSpeed = %.0f"), Speed));
		}
		LastAppliedSpeed = Speed;
	}

	HazardTimer += DeltaSeconds;
	if (HazardTimer >= 1.f)
	{
		HazardTimer -= 1.f;
		Player->ReceiveDamage(HazardDamage);
	}

	HealthLabel->SetActorLocation(Player->GetActorLocation() + FVector(0.f, 0.f, 130.f));
	HealthLabel->SetText(FString::Printf(TEXT("HP %.0f%s"), Player->Health, Player->bGodMode ? TEXT("  [GOD]") : TEXT("")));
	HealthLabel->SetColor(Player->bGodMode ? Colors::Item : Colors::Ally);

	if (Panel)
	{
		Panel->SetBody(FString::Printf(
			TEXT("game.PlayerSpeed = %.0f   (TAutoConsoleVariable)\n")
			TEXT("실제 속도 %.0f cm/s\n")
			TEXT("GodMode = %s   (UFUNCTION(Exec), 막은 피해 %d회)\n\n")
			TEXT("Exec 가 동작하는 클래스:\n  PlayerController · Pawn · HUD · GameMode\n  CheatManager · GameInstance\n")
			TEXT("→ 그 외 액터의 Exec 는 콘솔이 찾지 않는다\n\n%s"),
			Speed, Player->GetVelocity().Size2D(), Player->bGodMode ? TEXT("ON") : TEXT("OFF"), Player->BlockedHits, *Log.Join()));
	}
	SetStatus(FString::Printf(TEXT("HP %.0f · GodMode %s · PlayerSpeed %.0f"), Player->Health, Player->bGodMode ? TEXT("ON") : TEXT("OFF"), Speed));
}
