#include "Demo1005_LightMobility.h"

#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

namespace
{
	const FVector SwingPivot(150.f, 250.f, 440.f);
	constexpr float CordLength = 180.f;
}

ADemo1005_LightMobility::ADemo1005_LightMobility()
{
	DemoId = TEXT("1005");
	Controls = TEXT("1 : 모든 라이트 색 바꾸기 시도   2 : 모든 라이트 옆으로 옮기기 시도   3 : 흔들림 ON/OFF\n")
		TEXT("관찰: Static 천장등은 실행 중 아무것도 안 바뀐다(굽는 빛). Stationary 스탠드는 색·세기만 바뀌고 못 움직인다.\n")
		TEXT("Movable 전등만 흔들리며 그림자도 실시간으로 따라 움직인다.");
	Showcase = TEXT("Static 천장등 2·Stationary 스탠드·Movable 흔들리는 전등, 실행 중 색 변경/이동 시도로 Mobility 차이 확인");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Indoor;
	Implementation = EGlossaryImplementation::Alternative;
	AlternativeNote = TEXT("라이트 배치·Mobility 는 실제(BuildLevel 이 레벨에 저장). 단 Build Lighting(Lightmass 라이트맵 굽기)은 ")
		TEXT("레벨 생성 커맨드렛에서 돌리지 않아 Static 라이트가 '미리보기(Unbuilt)' 상태로 보인다. ")
		TEXT("에디터에서 Build > Build Lighting Only 를 누르면 라이트맵이 구워진다(Lumen GI 를 쓰면 GI 는 Lumen 이 대신한다).");
}

#if WITH_EDITOR
void ADemo1005_LightMobility::BuildLevel(UWorld* World)
{
	// 레벨에 저장돼야 Build Lighting 대상이 되므로 라이트는 BuildLevel 에서 놓는다
	auto Place = [World](UClass* Class, const FVector& Loc, const FRotator& Rot, EComponentMobility::Type Mobility, const TCHAR* Label, float Candela, const FLinearColor& Color) -> ALight*
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ALight* Light = World->SpawnActor<ALight>(Class, Loc, Rot, Params);
		ULightComponent* Comp = Light->GetLightComponent();
		Light->GetRootComponent()->SetMobility(Mobility);   // Static / Stationary / Movable
		if (ULocalLightComponent* Local = Cast<ULocalLightComponent>(Comp))
		{
			Local->IntensityUnits = ELightUnits::Candelas;
			Local->AttenuationRadius = 1200.f;
		}
		Comp->Intensity = Candela;
		Comp->LightColor = Color.ToFColor(true);
		Light->SetActorLabel(Label);
		Light->Tags.Add(TEXT("MobilityDemo"));
		return Light;
	};
	Place(APointLight::StaticClass(), FVector(-250.f, -250.f, 420.f), FRotator::ZeroRotator, EComponentMobility::Static, TEXT("CeilingLight_Static_A"), 30.f, FLinearColor(1.f, 0.9f, 0.75f));
	Place(APointLight::StaticClass(), FVector(-250.f, 250.f, 420.f), FRotator::ZeroRotator, EComponentMobility::Static, TEXT("CeilingLight_Static_B"), 30.f, FLinearColor(1.f, 0.9f, 0.75f));
	Place(ASpotLight::StaticClass(), FVector(250.f, -300.f, 180.f), FRotator(-70.f, 0.f, 0.f), EComponentMobility::Stationary, TEXT("DeskLamp_Stationary"), 120.f, FLinearColor(0.7f, 0.85f, 1.f));
	Place(APointLight::StaticClass(), SwingPivot - FVector(0.f, 0.f, CordLength), FRotator::ZeroRotator, EComponentMobility::Movable, TEXT("SwingLamp_Movable"), 25.f, FLinearColor(1.f, 0.6f, 0.3f));
}
#endif

void ADemo1005_LightMobility::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 방: 바닥·뒷벽·양옆 벽·천장 (카메라 쪽 -X 는 열림)
	const FLinearColor Wall(0.6f, 0.58f, 0.55f);
	SpawnShape(World, EShape::Cube, FVector(0.f, 0.f, 460.f), FVector(1000.f, 1000.f, 20.f), Wall);
	SpawnShape(World, EShape::Cube, FVector(510.f, 0.f, 230.f), FVector(20.f, 1000.f, 460.f), Wall);
	SpawnShape(World, EShape::Cube, FVector(0.f, -510.f, 230.f), FVector(1000.f, 20.f, 460.f), Wall);
	SpawnShape(World, EShape::Cube, FVector(0.f, 510.f, 230.f), FVector(1000.f, 20.f, 460.f), Wall);
	// 가구: 책상 + 의자 + 상자들 (그림자 확인용)
	SpawnBlock(World, FVector(300.f, -300.f, 0.f), FVector(200.f, 300.f, 80.f), FLinearColor(0.35f, 0.2f, 0.1f));
	SpawnBlock(World, FVector(0.f, 0.f, 0.f), FVector(120.f, 120.f, 120.f), Colors::Player);
	SpawnBlock(World, FVector(150.f, 250.f, 0.f), FVector(80.f, 80.f, 160.f), Colors::Ally);
	SpawnShape(World, EShape::Sphere, FVector(-200.f, 0.f, 50.f), FVector(100.f), Colors::Enemy);

	for (TActorIterator<ALight> It(World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("MobilityDemo")))
		{
			Lights.Add(*It);
			const EComponentMobility::Type M = It->GetRootComponent()->Mobility;
			if (M == EComponentMobility::Movable)
			{
				SwingLight = *It;
			}
			// 라이트 위치에 작은 전구 + 라벨
			const TCHAR* Name = M == EComponentMobility::Static ? TEXT("Static") : M == EComponentMobility::Stationary ? TEXT("Stationary") : TEXT("Movable");
			if (M != EComponentMobility::Movable)
			{
				SpawnLabel(World, Name, It->GetActorLocation() + FVector(0.f, 0.f, -50.f), Colors::Item, 18.f);
			}
		}
	}
	if (SwingLight)
	{
		SwingBulb = SpawnShape(World, EShape::Sphere, SwingLight->GetActorLocation(), FVector(24.f), FLinearColor(1.f, 0.7f, 0.4f));
		SetColor(SwingBulb->GetStaticMeshComponent(), FLinearColor(1.f, 0.6f, 0.3f), 0.5f, 0.f, 20.f);
		SwingBulb->GetStaticMeshComponent()->SetCastShadow(false);
		SwingCord = SpawnShape(World, EShape::Cylinder, SwingPivot - FVector(0.f, 0.f, CordLength * 0.5f), FVector(2.f, 2.f, CordLength), Colors::Dark);
		SwingCord->GetStaticMeshComponent()->SetCastShadow(false);
		SpawnLabel(World, TEXT("Movable"), SwingPivot + FVector(0.f, 0.f, -230.f), Colors::Item, 18.f);
	}

	RenderKit::SpawnLookAtCamera(this, FVector(-1300.f, 0.f, 330.f), FVector(0.f, -330.f, 170.f), 65.f);   // 왼쪽 정보 패널을 피해 방을 오른쪽으로
}

void ADemo1005_LightMobility::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (WasKeyPressed(EKeys::One))
	{
		// 실행 중 색 변경: Static 은 무시된다(AreDynamicDataChangesAllowed 가 false)
		const FLinearColor NewColor = FLinearColor::MakeFromHSV8(uint8(FMath::RandRange(0, 255)), 200, 255);
		for (ALight* L : Lights)
		{
			L->GetLightComponent()->SetLightColor(NewColor);
		}
		LastAction = TEXT("색 바꾸기 시도");
	}
	if (WasKeyPressed(EKeys::Two))
	{
		// 실행 중 이동: Static·Stationary 는 움직이지 않는다(로그에 경고)
		for (ALight* L : Lights)
		{
			if (L != SwingLight)
			{
				L->GetRootComponent()->SetWorldLocation(L->GetActorLocation() + FVector(0.f, 120.f, 0.f));
			}
		}
		LastAction = TEXT("옮기기 시도 (흔들리는 전등은 원래 움직이는 중)");
	}
	if (WasKeyPressed(EKeys::Three)) { bSwing = !bSwing; }

	if (SwingLight && bSwing)
	{
		SwingTime += DeltaSeconds;
		const float Angle = FMath::DegreesToRadians(SwingAngle * FMath::Sin(SwingTime * 2.2f));
		const FVector Offset(FMath::Sin(Angle) * CordLength, 0.f, -FMath::Cos(Angle) * CordLength);
		SwingLight->GetRootComponent()->SetWorldLocation(SwingPivot + Offset);
		SwingBulb->SetActorLocation(SwingPivot + Offset);
		SwingCord->SetActorLocationAndRotation(SwingPivot + Offset * 0.5f, FRotator(0.f, 0.f, 0.f) + FRotator(FMath::RadiansToDegrees(Angle), 0.f, 0.f));
	}

	Stats.Update(DeltaSeconds);
	FString Lines;
	for (ALight* L : Lights)
	{
		const ULightComponent* C = L->GetLightComponent();
		const EComponentMobility::Type M = C->Mobility;
		Lines += FString::Printf(TEXT("%-12s 색(%d,%d,%d) 위치Y %.0f\n"),
			M == EComponentMobility::Static ? TEXT("Static") : M == EComponentMobility::Stationary ? TEXT("Stationary") : TEXT("Movable"),
			C->LightColor.R, C->LightColor.G, C->LightColor.B, L->GetActorLocation().Y);
	}
	SetStatus(FString::Printf(TEXT("%s마지막 시도: %s · 흔들림 %s\n%s"), *Lines, LastAction.IsEmpty() ? TEXT("-") : *LastAction,
		bSwing ? TEXT("ON") : TEXT("OFF"), *Stats.ToString(false)));
}
