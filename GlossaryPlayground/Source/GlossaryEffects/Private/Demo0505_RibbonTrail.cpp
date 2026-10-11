#include "Demo0505_RibbonTrail.h"

#include "Components/SceneComponent.h"
#include "EffectsDemoKit.h"
#include "GlossaryKit.h"
#include "NiagaraComponent.h"

using namespace GlossaryKit;

ADemo0505_RibbonTrail::ADemo0505_RibbonTrail()
{
	DemoId = TEXT("0505");
	Controls = TEXT("C : 검기 트레일 ON/OFF\n")
		TEXT("관찰: 칼끝(SwordTip)에 붙은 Ribbon 이미터가 지나간 위치마다 입자를 남기고,\n")
		TEXT("Ribbon Renderer 가 그 입자들을 띠로 이어 검기를 그린다. 입자가 수명을 다하면 꼬리부터 사라진다");
	Showcase = TEXT("휘두르는 칼의 칼끝에 Ribbon 이미터를 붙여 검기 트레일, ON/OFF 비교");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Sunset;
}

void ADemo0505_RibbonTrail::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 칼: 회전축(Pivot) → 손잡이 + 칼날, 칼끝에 소켓 역할의 SwordTip 컴포넌트
	Pivot = NewObject<USceneComponent>(this, TEXT("Pivot"));
	Pivot->SetupAttachment(GetRootComponent());
	Pivot->SetRelativeLocation(FVector(0.f, 0.f, 160.f));
	Pivot->RegisterComponent();
	AddShape(this, EShape::Cylinder, FVector(10.f, 0.f, 0.f), FVector(10.f, 10.f, 40.f), Colors::Dark, Pivot, FRotator(90.f, 0.f, 0.f));
	AddShape(this, EShape::Cube, FVector(32.f, 0.f, 0.f), FVector(6.f, 40.f, 6.f), Colors::Item, Pivot);
	AddShape(this, EShape::Cube, FVector(125.f, 0.f, 0.f), FVector(180.f, 4.f, 16.f), FLinearColor(0.8f, 0.85f, 0.9f), Pivot);
	SwordTip = NewObject<USceneComponent>(this, TEXT("SwordTip"));
	SwordTip->SetupAttachment(Pivot);
	SwordTip->SetRelativeLocation(FVector(210.f, 0.f, 0.f));
	SwordTip->RegisterComponent();

	// 핵심: 칼끝에 리본 이미터를 붙인다 (월드 공간 시뮬레이션이라 지나간 자리가 남는다)
	Trail = EffectsKit::AddNiagara(this, TEXT("NS_SwordTrail"), FVector::ZeroVector, SwordTip);

	SpawnShape(World, EShape::Cylinder, FVector(0.f, 0.f, 75.f), FVector(30.f, 30.f, 150.f), Colors::Wall);
	SpawnLabel(World, TEXT("칼끝 → Ribbon 트레일"), FVector(0.f, 0.f, 420.f), Colors::Item, 20.f);
	EffectsKit::SetupFixedCamera(this, FVector(-520.f, -260.f, 520.f), FVector(60.f, 0.f, 150.f), 70.f);
}

void ADemo0505_RibbonTrail::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::C))
	{
		bTrailOn = !bTrailOn;
		if (Trail)
		{
			bTrailOn ? Trail->Activate(true) : Trail->Deactivate();
		}
	}

	// 휘두르기: 빠르게 한 번 휘두르고(SwingDuration) 잠깐 멈췄다가 반대 방향으로
	if (Pause > 0.f)
	{
		Pause -= DeltaSeconds;
	}
	else
	{
		SwingTime += DeltaSeconds;
		if (SwingTime >= SwingDuration)
		{
			SwingTime = 0.f;
			Direction = -Direction;
			Pause = 0.05f;
		}
	}
	const float Alpha = FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp(SwingTime / SwingDuration, 0.f, 1.f), 2.f);
	const float From = -SwingArc * 0.5f * Direction, To = SwingArc * 0.5f * Direction;
	const float Yaw = FMath::Lerp(From, To, Alpha);   // 멈춰 있는 동안은 SwingTime=0 → 시작 각도
	// 대각선으로 내려 베는 궤적: Roll 로 기울인 평면에서 Yaw 회전
	Pivot->SetRelativeRotation(FRotator(0.f, 0.f, -35.f).Quaternion() * FRotator(0.f, Yaw, 0.f).Quaternion());

	SetStatus(FString::Printf(TEXT("트레일 %s · 휘두르기 %.2f초 · 각도 %.0f°"), bTrailOn ? TEXT("ON") : TEXT("OFF"), SwingDuration, Yaw));
}
