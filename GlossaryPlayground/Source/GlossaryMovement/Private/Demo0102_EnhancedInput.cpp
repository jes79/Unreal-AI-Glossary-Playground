#include "Demo0102_EnhancedInput.h"

#include "D0102_InputCharacter.h"
#include "GlossaryKit.h"
#include "MovementSidePanel.h"

using namespace GlossaryKit;

ADemo0102_EnhancedInput::ADemo0102_EnhancedInput()
{
	DemoId = TEXT("0102");
	Controls = TEXT("WASD/왼스틱 이동 · Space/패드A 점프 · 좌클릭/패드X 공격 · 우클릭/패드RT 0.5초 홀드 강공격\n")
		TEXT("관찰: 오른쪽 로그에 Started → Triggered → Completed (홀드를 일찍 떼면 Canceled)\n")
		TEXT("템플릿 에셋 /Game/Input/Actions/IA_Move·IA_Jump, /Game/Input/IMC_Default 와 같은 구조를 코드로 조립");
	Showcase = TEXT("실행 중 NewObject 로 만든 IA_Move·IA_Jump·IA_Attack + IMC_Default(키보드+게임패드, Negate·Swizzle·DeadZone·Pressed·Hold), 트리거 이벤트 로그");
	PlayerPawnClass = AD0102_InputCharacter::StaticClass();
}

void ADemo0102_EnhancedInput::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 공격 대상 허수아비 몇 개
	for (int32 i = 0; i < 3; ++i)
	{
		const FVector Pos(200.f, -300.f + i * 300.f, 0.f);
		SpawnBlock(World, Pos, FVector(60.f, 60.f, 160.f), Colors::Enemy);
		SpawnShape(World, EShape::Sphere, Pos + FVector(0.f, 0.f, 200.f), FVector(70.f), Colors::Enemy);
	}
	SpawnLabel(World, TEXT("IA_Attack / IA_HeavyAttack 연습용"), FVector(200.f, 0.f, 330.f), Colors::Item, 18.f);

	Panel = UMovementSidePanel::Create(GetPC(), TEXT("IMC_Default (실행 중 생성) · 트리거 이벤트 로그"), 520.f);
}

void ADemo0102_EnhancedInput::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Panel)
	{
		Panel->RemoveFromParent();
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0102_EnhancedInput::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AD0102_InputCharacter* Character = Cast<AD0102_InputCharacter>(GetPlayerPawn());
	if (!Character || !Panel)
	{
		return;
	}

	FString Body = TEXT("IA_Move (Axis2D) ← WASD [Negate·Swizzle] · 패드 왼스틱 [DeadZone]\n")
		TEXT("IA_Jump (Bool) ← Space · 패드 A\n")
		TEXT("IA_Attack (Bool) ← 좌클릭 · 패드 X  [Pressed]\n")
		TEXT("IA_HeavyAttack (Bool) ← 우클릭 · 패드 RT  [Hold 0.5초]\n")
		TEXT("─────────── 이벤트 로그 (새 것이 위) ───────────\n");
	const TArray<FString>& Log = Character->GetEventLog();
	if (Log.Num() == 0)
	{
		Body += TEXT("(아직 이벤트 없음 — 키를 눌러 보세요)");
	}
	for (const FString& Line : Log)
	{
		Body += Line + TEXT("\n");
	}
	Panel->SetBody(Body.TrimEnd());

	SetStatus(FString::Printf(TEXT("IA_Move 값 (%.2f, %.2f) · Triggered 누적 %d회\n강공격 홀드 %.2f / %.2f초"),
		Character->GetLastMove().X, Character->GetLastMove().Y, Character->GetMoveTriggeredCount(),
		Character->GetHeavyHoldTime(), Character->HeavyAttackHoldTime));
}
