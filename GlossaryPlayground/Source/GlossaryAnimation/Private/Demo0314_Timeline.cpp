#include "Demo0314_Timeline.h"

#include "Components/SceneComponent.h"
#include "Curves/CurveFloat.h"
#include "D03_GraphWidget.h"
#include "D03_Mannequin.h"
#include "Engine/World.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0314_Timeline::ADemo0314_Timeline()
{
	DemoId = TEXT("0314");
	Controls = TEXT("E : 문 열기 / 다시 누르면 Reverse 로 닫기 (4초마다 자동으로도 토글, C : 자동 끄기)\n")
		TEXT("관찰: 왼쪽 문(Ease In/Out)은 천천히 출발해 가속했다가 부드럽게 멈추고, 오른쪽(Linear)은 같은 1초지만\n")
		TEXT("일정 속도로 움직여 시작·끝이 딱딱하다. 오른쪽 아래 그래프의 점이 지금 재생 위치");
	Showcase = TEXT("FTimeline + 1초 Ease In/Out 커브 에셋으로 문 열기·Reverse 닫기, Linear 커브 문과 나란히 비교, 커브와 재생 위치 그래프");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-600.f, 0.f, 100.f));
}

AActor* ADemo0314_Timeline::MakeDoor(const FVector& Hinge, const FLinearColor& Color, const FString& Label)
{
	UWorld* World = GetWorld();
	// 경첩(빈 액터) + 문짝(경첩에서 반 폭만큼 떨어진 상자) — 경첩 Yaw 를 돌리면 문이 열린다
	AActor* HingeActor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform(Hinge));
	USceneComponent* HingeRoot = NewObject<USceneComponent>(HingeActor, TEXT("Hinge"));
	HingeRoot->SetMobility(EComponentMobility::Movable);
	HingeActor->SetRootComponent(HingeRoot);
	HingeRoot->RegisterComponent();
	HingeActor->SetActorLocation(Hinge);
	AddShape(HingeActor, EShape::Cube, FVector(0.f, 80.f, 130.f), FVector(12.f, 160.f, 260.f), Color);
	AddShape(HingeActor, EShape::Cylinder, FVector(0.f, 150.f, 125.f), FVector(10.f, 10.f, 14.f), Colors::Item, nullptr, FRotator(0.f, 0.f, 90.f));
	// 문틀
	SpawnBlock(World, Hinge + FVector(0.f, -15.f, 0.f), FVector(30.f, 30.f, 280.f), Colors::Wall);
	SpawnBlock(World, Hinge + FVector(0.f, 175.f, 0.f), FVector(30.f, 30.f, 280.f), Colors::Wall);
	SpawnShape(World, EShape::Cube, Hinge + FVector(0.f, 80.f, 290.f), FVector(30.f, 220.f, 20.f), Colors::Wall);
	SpawnLabel(World, Label, Hinge + FVector(0.f, 80.f, 340.f), Color, 18.f);
	return HingeActor;
}

void ADemo0314_Timeline::BeginPlay()
{
	Super::BeginPlay();
	EaseCurve = LoadAsset<UCurveFloat>(D03::Folder + TEXT("C_DoorEaseInOut"));
	LinearCurve = LoadAsset<UCurveFloat>(D03::Folder + TEXT("C_DoorLinear"));

	EaseDoor = MakeDoor(FVector(0.f, -330.f, 0.f), Colors::Ally, TEXT("Ease In/Out 커브"));
	LinearDoor = MakeDoor(FVector(0.f, 110.f, 0.f), Colors::Accent, TEXT("Linear 커브"));

	// Timeline 노드와 같은 구성: Float 트랙(커브) + Update 이벤트, 길이 1초, 반복 안 함
	if (EaseCurve)
	{
		FOnTimelineFloat Update;
		Update.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ADemo0314_Timeline, OnEaseProgress));
		EaseTimeline.AddInterpFloat(EaseCurve, Update);
	}
	if (LinearCurve)
	{
		FOnTimelineFloat Update;
		Update.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ADemo0314_Timeline, OnLinearProgress));
		LinearTimeline.AddInterpFloat(LinearCurve, Update);
	}
	for (FTimeline* T : { &EaseTimeline, &LinearTimeline })
	{
		T->SetTimelineLength(Duration);
		T->SetLooping(false);
	}

	Graph = UD03_GraphWidget::Create(GetPC(), TEXT("Timeline 커브 (초록 Ease In/Out · 주황 Linear)"));
	if (Graph)
	{
		Graph->XMin = 0.f; Graph->XMax = Duration; Graph->YMin = 0.f; Graph->YMax = 1.f;
		Graph->GridX = 4; Graph->GridY = 4;
		Graph->XLabel = TEXT("시간 (초)");
		Graph->YLabel = TEXT("값 → 각도");
		Graph->PanelSize = FVector2D(400.f, 290.f);
	}
}

void ADemo0314_Timeline::OnEaseProgress(float Value)
{
	EaseValue = Value;
	if (EaseDoor) { EaseDoor->SetActorRotation(FRotator(0.f, -OpenAngle * Value, 0.f)); }
}

void ADemo0314_Timeline::OnLinearProgress(float Value)
{
	LinearValue = Value;
	if (LinearDoor) { LinearDoor->SetActorRotation(FRotator(0.f, -OpenAngle * Value, 0.f)); }
}

void ADemo0314_Timeline::Toggle()
{
	// Flip Flop: 열기는 Play, 닫기는 Reverse — 도중에 눌러도 지금 위치에서 방향만 바뀐다
	bOpen = !bOpen;
	for (FTimeline* T : { &EaseTimeline, &LinearTimeline })
	{
		if (bOpen) { T->Play(); }
		else { T->Reverse(); }
	}
}

void ADemo0314_Timeline::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::C)) { bAutoToggle = !bAutoToggle; }
	if (WasKeyPressed(EKeys::E)) { Toggle(); AutoTimer = 0.f; }
	AutoTimer += DeltaSeconds;
	if (bAutoToggle && AutoTimer >= 4.f) { Toggle(); AutoTimer = 0.f; }

	// Timeline 컴포넌트 대신 FTimeline 을 직접 진행 (컴포넌트도 내부적으로 같은 일을 한다)
	EaseTimeline.TickTimeline(DeltaSeconds);
	LinearTimeline.TickTimeline(DeltaSeconds);

	if (Graph)
	{
		Graph->ClearDynamic();
		UD03_GraphWidget::FLine E{ {}, FLinearColor(0.3f, 0.95f, 0.4f), 2.5f }, L{ {}, FLinearColor(1.f, 0.55f, 0.15f), 2.f };
		for (int32 i = 0; i <= 40; ++i)
		{
			const float T = Duration * i / 40.f;
			if (EaseCurve) { E.Points.Add(FVector2D(T, EaseCurve->GetFloatValue(T))); }
			if (LinearCurve) { L.Points.Add(FVector2D(T, LinearCurve->GetFloatValue(T))); }
		}
		Graph->Lines.Add(L);
		Graph->Lines.Add(E);
		Graph->Points.Add({ FVector2D(EaseTimeline.GetPlaybackPosition(), EaseValue), FLinearColor(0.3f, 1.f, 0.4f), TEXT(""), 14.f });
		Graph->Points.Add({ FVector2D(LinearTimeline.GetPlaybackPosition(), LinearValue), FLinearColor(1.f, 0.6f, 0.2f), TEXT(""), 12.f });
	}
	SetStatus(FString::Printf(TEXT("%s · 재생 위치 %.2f / %.2f 초 %s\nEase In/Out 값 %.2f → %.0f°   ·   Linear 값 %.2f → %.0f°"),
		bOpen ? TEXT("열기 (Play)") : TEXT("닫기 (Reverse)"), EaseTimeline.GetPlaybackPosition(), Duration,
		EaseTimeline.IsPlaying() ? TEXT("재생 중") : TEXT("정지"), EaseValue, EaseValue * OpenAngle, LinearValue, LinearValue * OpenAngle));
}
