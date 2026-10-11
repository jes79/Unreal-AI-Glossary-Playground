#include "Demo1108_TickTimerEvent.h"

#include "Camera/CameraActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "TimerManager.h"

using namespace GlossaryKit;

int64 AD1108_SensorBase::Checks[3] = { 0, 0, 0 };

static const FLinearColor LaneColors[3] = { FLinearColor(1.f, 0.3f, 0.2f), FLinearColor(1.f, 0.8f, 0.1f), FLinearColor(0.2f, 0.9f, 0.4f) };

// ================================================================ 센서 공통

AD1108_SensorBase::AD1108_SensorBase()
{
	Lamp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lamp"));
	RootComponent = Lamp;
	InitShapeComponent(Lamp, EShape::Cylinder, FVector(60.f, 60.f, 120.f));
	Lamp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AD1108_SensorBase::SetLit(bool bLit)
{
	if (bLit == bIsLit)
	{
		return; // 상태가 바뀔 때만 머티리얼을 갱신
	}
	bIsLit = bLit;
	SetColor(Lamp, bLit ? FLinearColor(1.f, 0.95f, 0.6f) : Colors::Dark, 0.5f, 0.f, bLit ? 6.f : 0.f);
}

// ================================================================ 방식 1: Tick

AD1108_TickSensor::AD1108_TickSensor()
{
	PrimaryActorTick.bCanEverTick = true; // 매 프레임 호출
}

void AD1108_TickSensor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	++Checks[0];
	// 공이 멀리 있어도, 화면 밖이어도 매 프레임 거리 계산
	SetLit(Target && FVector::DistSquared(Target->GetActorLocation(), GetActorLocation()) < FMath::Square(DetectRadius));
}

// ================================================================ 방식 2: Timer

AD1108_TimerSensor::AD1108_TimerSensor()
{
	PrimaryActorTick.bCanEverTick = false; // Tick 함수 자체가 등록되지 않는다
}

void AD1108_TimerSensor::BeginPlay()
{
	Super::BeginPlay();
	SetInterval(CheckInterval);
}

void AD1108_TimerSensor::SetInterval(float NewInterval)
{
	CheckInterval = NewInterval;
	// 0.2초마다 반복. 첫 호출을 무작위로 늦춰 여러 센서가 같은 프레임에 몰리지 않게(부하 분산)
	GetWorldTimerManager().SetTimer(CheckTimer, this, &AD1108_TimerSensor::CheckDistance, CheckInterval, true, FMath::FRandRange(0.f, CheckInterval));
}

void AD1108_TimerSensor::CheckDistance()
{
	++Checks[1];
	SetLit(Target && FVector::DistSquared(Target->GetActorLocation(), GetActorLocation()) < FMath::Square(DetectRadius));
}

// ================================================================ 방식 3: Overlap 이벤트

AD1108_OverlapSensor::AD1108_OverlapSensor()
{
	PrimaryActorTick.bCanEverTick = false;
	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Lamp);
	Trigger->SetAbsolute(false, false, true); // 기둥 스케일에 영향받지 않게 → 반지름이 그대로 cm
	Trigger->SetSphereRadius(DetectRadius);
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Trigger->SetGenerateOverlapEvents(true);
}

void AD1108_OverlapSensor::BeginPlay()
{
	Super::BeginPlay();
	// 물리 엔진이 겹침을 감지해서 "들어왔다/나갔다" 순간에만 불러 준다 — 그 사이엔 아무 비용도 없다
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AD1108_OverlapSensor::HandleBeginOverlap);
	Trigger->OnComponentEndOverlap.AddDynamic(this, &AD1108_OverlapSensor::HandleEndOverlap);
}

void AD1108_OverlapSensor::HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->IsA<AD1108_Probe>())
	{
		++Checks[2];
		++OverlapCount;
		SetLit(true);
	}
}

void AD1108_OverlapSensor::HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor->IsA<AD1108_Probe>())
	{
		++Checks[2];
		OverlapCount = FMath::Max(0, OverlapCount - 1);
		SetLit(OverlapCount > 0);
	}
}

// ================================================================ 탐지 대상 공

AD1108_Probe::AD1108_Probe()
{
	PrimaryActorTick.bCanEverTick = true; // 움직이는 물체는 Tick 이 맞다(매 프레임 위치가 바뀌므로)
	Ball = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ball"));
	RootComponent = Ball;
	InitShapeComponent(Ball, EShape::Sphere, FVector(80.f));
	Ball->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Ball->SetGenerateOverlapEvents(true);
}

void AD1108_Probe::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	FVector Loc = GetActorLocation();
	Loc.X += Dir * Speed * DeltaSeconds;
	if (Loc.X > MaxX) { Loc.X = MaxX; Dir = -1.f; }
	if (Loc.X < MinX) { Loc.X = MinX; Dir = 1.f; }
	// bSweep=false 여도 이동 후 겹침 갱신은 일어난다(UpdateOverlaps)
	SetActorLocation(Loc);
}

// ================================================================ ADemo1108

ADemo1108_TickTimerEvent::ADemo1108_TickTimerEvent()
{
	DemoId = TEXT("1108");
	Controls = TEXT("T : Timer 간격 0.2초 ↔ 1초 (길게 하면 반응이 늦어지는 것 확인)\n")
		TEXT("관찰: 세 레인 모두 공이 3m 안에 오면 불이 켜진다. 결과는 같지만\n")
		TEXT("초당 검사 횟수는 Tick ≫ Timer ≫ Overlap. 장식 기둥은 bCanEverTick=false 로 0");
	Showcase = TEXT("같은 거리 감지를 Tick·0.2초 Timer·Overlap 이벤트로 8개씩, 초당 검사 횟수 비교");
	PlayerPawnClass = nullptr;
}

void ADemo1108_TickTimerEvent::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	FMemory::Memzero(AD1108_SensorBase::Checks, sizeof(AD1108_SensorBase::Checks));

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const TCHAR* LaneNames[3] = { TEXT("Tick (매 프레임)"), TEXT("Timer 0.2초"), TEXT("Overlap 이벤트") };
	const float LaneY[3] = { -600.f, 0.f, 600.f };

	for (int32 Lane = 0; Lane < 3; ++Lane)
	{
		AD1108_Probe* Probe = World->SpawnActor<AD1108_Probe>(FVector(-200.f + Lane * 300.f, LaneY[Lane] - 180.f, 60.f), FRotator::ZeroRotator, Params);
		SetColor(Probe->FindComponentByClass<UStaticMeshComponent>(), Colors::Player, 0.3f, 0.f, 1.f);

		for (int32 i = 0; i < SensorsPerLane; ++i)
		{
			const FVector Loc(i * 250.f, LaneY[Lane], 60.f);
			AD1108_SensorBase* Sensor = nullptr;
			switch (Lane)
			{
			case 0: Sensor = World->SpawnActor<AD1108_TickSensor>(Loc, FRotator::ZeroRotator, Params); break;
			case 1: Sensor = World->SpawnActor<AD1108_TimerSensor>(Loc, FRotator::ZeroRotator, Params); TimerSensors.Add(Cast<AD1108_TimerSensor>(Sensor)); break;
			default: Sensor = World->SpawnActor<AD1108_OverlapSensor>(Loc, FRotator::ZeroRotator, Params); break;
			}
			Sensor->Target = Probe;
			SetColor(Sensor->FindComponentByClass<UStaticMeshComponent>(), Colors::Dark);
		}
		SpawnBlock(World, FVector(875.f, LaneY[Lane] - 180.f, 0.f), FVector(2300.f, 60.f, 4.f), LaneColors[Lane]);
		SpawnLabel(World, LaneNames[Lane], FVector(-450.f, LaneY[Lane], 200.f), LaneColors[Lane], 20.f);
	}

	// 장식 기둥 30개: 아무것도 검사하지 않으니 Tick 이 필요 없다 → bCanEverTick=false (StaticMeshActor 기본값)
	for (int32 i = 0; i < 30; ++i)
	{
		AStaticMeshActor* Deco = SpawnShape(World, EShape::Cube, FVector(2200.f + (i % 3) * 120.f, -900.f + (i / 3) * 200.f, 40.f), FVector(60.f, 60.f, 80.f), Colors::Wall);
		IdleActors += Deco->PrimaryActorTick.bCanEverTick ? 0 : 1;
	}
	SpawnLabel(World, TEXT("장식 30개 (bCanEverTick=false)"), FVector(2320.f, 0.f, 220.f), Colors::Neutral, 16.f);

	if (APlayerController* PC = GetPC())
	{
		ACameraActor* Cam = World->SpawnActor<ACameraActor>(FVector(-1300.f, 0.f, 1500.f), FRotator(-42.f, 0.f, 0.f));
		PC->SetViewTarget(Cam);
	}
	Panel = UD11_SidePanel::Create(GetPC(), TEXT("초당 검사 횟수"), FVector2D(1.f, 0.f), 15.f);
}

void ADemo1108_TickTimerEvent::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (WasKeyPressed(EKeys::T))
	{
		TimerInterval = TimerInterval < 0.5f ? 1.f : 0.2f;
		for (AD1108_TimerSensor* Sensor : TimerSensors)
		{
			Sensor->SetInterval(TimerInterval);
		}
	}

	// 1초 창으로 초당 횟수 계산
	WindowTime += DeltaSeconds;
	++FramesInWindow;
	if (WindowTime >= 1.f)
	{
		for (int32 i = 0; i < 3; ++i)
		{
			PerSecond[i] = (AD1108_SensorBase::Checks[i] - LastChecks[i]) / WindowTime;
			LastChecks[i] = AD1108_SensorBase::Checks[i];
		}
		Fps = FramesInWindow / WindowTime;
		WindowTime = 0.f;
		FramesInWindow = 0;
	}

	if (Panel)
	{
		Panel->SetBody(FString::Printf(
			TEXT("센서 레인마다 %d개 · 현재 %.0f fps\n\n")
			TEXT("Tick       %6.0f 회/초   (%d개 × fps)\n")
			TEXT("Timer %.1fs %6.0f 회/초   (%d개 × %.0f)\n")
			TEXT("Overlap    %6.0f 회/초   (들어오고 나갈 때만)\n\n")
			TEXT("누적: Tick %lld · Timer %lld · Overlap %lld\n")
			TEXT("bCanEverTick=false 장식 %d개 → Tick 0회\n\n")
			TEXT("Timer 는 최대 %.1f초 늦게 반응할 수 있다\n(빠른 반응이 필요 없을 때 적합)"),
			SensorsPerLane, Fps,
			PerSecond[0], SensorsPerLane,
			TimerInterval, PerSecond[1], SensorsPerLane, 1.f / TimerInterval,
			PerSecond[2],
			AD1108_SensorBase::Checks[0], AD1108_SensorBase::Checks[1], AD1108_SensorBase::Checks[2],
			IdleActors, TimerInterval));
	}
	SetStatus(FString::Printf(TEXT("초당 검사 — Tick %.0f · Timer %.0f · Overlap %.0f"), PerSecond[0], PerSecond[1], PerSecond[2]));
}
