#include "Demo0816_Timer.h"

#include "Components/StaticMeshComponent.h"
#include "D08_Common.h"
#include "Engine/World.h"
#include "GlossaryKit.h"
#include "TimerManager.h"

using namespace GlossaryKit;

ADemo0816_Timer::ADemo0816_Timer()
{
	DemoId = TEXT("0816");
	// 플레이어 컨트롤러(입력 처리) 다음에 Tick 해서 WasKeyPressed 를 놓치지 않게
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	Controls = TEXT("1 : 독 시작 (SetTimer 1초 반복) · 2 : 해독 (ClearTimer)\n")
		TEXT("관찰: 1초마다 -10 이 뜨고 머리 위 램프가 하나씩 켜진다. 5번째에 타이머가 스스로 ClearTimer — 그 사이 Tick 은 수백 번");
	Showcase = TEXT("SetTimer 로 1초마다 독 데미지 5번, 다섯 번째에 ClearTimer — 같은 시간 Tick 호출 수와 비교");
}

void ADemo0816_Timer::BeginPlay()
{
	Super::BeginPlay();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Target = GetWorld()->SpawnActor<AD08_Dummy>(FVector(200.f, 350.f, 90.f), FRotator::ZeroRotator, Params);
	Target->Configure(TEXT("표적"), 100.f, Colors::Enemy);
	Target->RespawnDelay = 1.f;

	// 틱 램프 5개 (타이머 콜백 한 번 = 램프 하나)
	for (int32 i = 0; i < PoisonTicks; ++i)
	{
		UStaticMeshComponent* Lamp = AddShape(this, EShape::Sphere, FVector(200.f, 190.f + i * 80.f, 330.f), FVector(50.f), Colors::Dark);
		Lamp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TickLamps.Add(Lamp);
	}
	SpawnLabel(GetWorld(), TEXT("타이머 콜백 램프"), FVector(200.f, 350.f, 400.f), Colors::Ally, 18.f);
	Panel = UD08_SidePanel::Create(GetPC(), TEXT("FTimerHandle PoisonTimer"), UD08_SidePanel::EPlace::TopRight, 440.f);
	StartPoison();
}

void ADemo0816_Timer::StartPoison()
{
	TicksDone = 0;
	FrameTicksWhilePoisoned = 0;
	for (UStaticMeshComponent* Lamp : TickLamps)
	{
		SetColor(Lamp, Colors::Dark);
	}
	// 핵심: 1초 간격, 반복(true). 첫 호출은 1초 뒤
	GetWorldTimerManager().SetTimer(PoisonTimer, this, &ADemo0816_Timer::PoisonTick, PoisonInterval, /*bLoop*/ true);
	LastEvent = TEXT("SetTimer(PoisonTimer, 1.0초, 반복)");
}

void ADemo0816_Timer::PoisonTick()
{
	++TicksDone;
	Target->ApplyHit(PoisonDamage, Colors::Ally, FString::Printf(TEXT("독 %d/%d"), TicksDone, PoisonTicks));
	if (TickLamps.IsValidIndex(TicksDone - 1))
	{
		SetColor(TickLamps[TicksDone - 1], Colors::Ally, 0.3f, 0.f, 3.f);
	}
	if (TicksDone >= PoisonTicks)
	{
		StopPoison(TEXT("5번 완료"));
	}
}

void ADemo0816_Timer::StopPoison(const TCHAR* Reason)
{
	// 핵심: 반복 타이머는 스스로 멈추지 않는다 → ClearTimer
	GetWorldTimerManager().ClearTimer(PoisonTimer);
	LastEvent = FString::Printf(TEXT("ClearTimer (%s) · 콜백 %d번 · 그동안 Tick %d번"), Reason, TicksDone, FrameTicksWhilePoisoned);
}

void ADemo0816_Timer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	FTimerManager& Timers = GetWorldTimerManager();
	const bool bActive = Timers.IsTimerActive(PoisonTimer);
	if (bActive)
	{
		++FrameTicksWhilePoisoned; // 비교용: Tick 은 매 프레임
	}
	if (WasKeyPressed(EKeys::One))
	{
		StartPoison();
	}
	if (WasKeyPressed(EKeys::Two) && bActive)
	{
		StopPoison(TEXT("해독"));
	}

	const float Remaining = bActive ? Timers.GetTimerRemaining(PoisonTimer) : 0.f;
	const float Elapsed = bActive ? Timers.GetTimerElapsed(PoisonTimer) : 0.f;
	// 중독 중에는 초록빛 (피격 깜빡임이 끝나면 BaseColor 로 돌아간다)
	const FLinearColor Wanted = bActive ? FMath::Lerp(Colors::Enemy, Colors::Ally, 0.6f) : Colors::Enemy;
	if (!Target->BaseColor.Equals(Wanted))
	{
		Target->BaseColor = Wanted;
		Target->SetBodyColor(Wanted);
	}
	if (Panel)
	{
		Panel->SetTitle(FString::Printf(TEXT("IsTimerActive = %s"), bActive ? TEXT("true") : TEXT("false")));
		Panel->SetProgress(bActive ? Elapsed / PoisonInterval : -1.f, Colors::Ally);
		Panel->SetBody(FString::Printf(TEXT("콜백 %d / %d  %s\n다음 콜백까지 %.2f초 (GetTimerRemaining)\n같은 시간 Tick 호출 %d번"),
			TicksDone, PoisonTicks, *D08::Bar(float(TicksDone) / PoisonTicks, PoisonTicks), Remaining, FrameTicksWhilePoisoned));
	}
	SetStatus(LastEvent);
}
