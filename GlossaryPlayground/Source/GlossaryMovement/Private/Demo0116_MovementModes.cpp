#include "Demo0116_MovementModes.h"

#include "D0116_ModeCharacter.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PhysicsVolume.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MovementSidePanel.h"

#if WITH_EDITOR
#include "BSPOps.h"
#include "Builders/CubeBuilder.h"
#include "Components/BrushComponent.h"
#include "Engine/Polys.h"
#include "Model.h"
#endif

using namespace GlossaryKit;

ADemo0116_MovementModes::ADemo0116_MovementModes()
{
	DemoId = TEXT("0116");
	Controls = TEXT("WASD 이동 · Space 점프 · F : 비행 모드 ON/OFF\n")
		TEXT("수영·비행 중: 보는 방향으로 이동, Space 상승 · Ctrl 하강\n")
		TEXT("관찰: 파란 물(Water Volume)에 들어가면 자동으로 Swimming, 탑에서 뛰어내리면 Falling, F 는 Flying");
	Showcase = TEXT("Physics Volume(Water Volume) 에 들어가면 Swimming, F 키로 Flying 토글, 탑에서 Falling — 머리 위 모드 라벨과 전환 기록");
	PlayerPawnClass = AD0116_ModeCharacter::StaticClass();
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-500.f, 0.f, 100.f));
}

#if WITH_EDITOR
void ADemo0116_MovementModes::BuildLevel(UWorld* World)
{
	// Physics Volume 은 BSP 브러시 모양이 필요해서 실행 중에는 못 만든다 → 레벨 빌드 때 놓고 저장
	// (에디터에서 Place Actors > Volumes > Physics Volume 을 놓고 Water Volume 을 체크하는 것과 같다)
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APhysicsVolume* Water = World->SpawnActor<APhysicsVolume>(WaterCenter, FRotator::ZeroRotator, Params);
	Water->PolyFlags = 0;
	Water->Brush = NewObject<UModel>(Water, NAME_None, RF_Transactional);
	Water->Brush->Initialize(nullptr, true);
	Water->Brush->Polys = NewObject<UPolys>(Water->Brush, NAME_None, RF_Transactional);
	Water->GetBrushComponent()->Brush = Water->Brush;
	UCubeBuilder* Builder = NewObject<UCubeBuilder>(Water);
	Builder->X = WaterSize.X;
	Builder->Y = WaterSize.Y;
	Builder->Z = WaterSize.Z;
	Water->BrushBuilder = Builder;
	Builder->Build(World, Water);
	FBSPOps::csgPrepMovingBrush(Water);
	Water->GetBrushComponent()->RequestUpdateBrushCollision();

	// ★ 핵심 두 값: 물로 취급 + 물의 저항
	Water->bWaterVolume = true;
	Water->FluidFriction = FluidFriction;
	Water->Priority = 1;
	Water->PostEditChange();
	Water->SetActorLabel(TEXT("WaterVolume"));
}
#endif

void ADemo0116_MovementModes::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 레벨에 저장된 Water Volume 확인
	for (TActorIterator<APhysicsVolume> It(World); It; ++It)
	{
		bHasWaterVolume |= It->bWaterVolume;
	}

	// 물의 모습: Water Volume 과 같은 크기의 반투명 상자 (충돌 없음)
	if (AStaticMeshActor* WaterBox = SpawnShape(World, EShape::Cube, WaterCenter, WaterSize, Colors::Cyan))
	{
		WaterBox->SetActorEnableCollision(false);
		if (UMaterialInterface* WaterMat = LoadAsset<UMaterialInterface>(TEXT("/Game/Glossary/01_Movement/M_Water")))
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(WaterMat, WaterBox);
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.05f, 0.35f, 0.9f));
			MID->SetScalarParameterValue(TEXT("Opacity"), 0.35f);
			WaterBox->GetStaticMeshComponent()->SetMaterial(0, MID);
		}
	}
	SpawnLabel(World, TEXT("Water Volume (Physics Volume · bWaterVolume)"), WaterCenter + FVector(0.f, 0.f, WaterSize.Z * 0.5f + 120.f), Colors::Cyan, 18.f);

	// 물속 바닥 표시 + 물가 계단
	SpawnBlock(World, WaterCenter - FVector(0.f, 0.f, WaterSize.Z * 0.5f), FVector(WaterSize.X, WaterSize.Y, 4.f), Colors::Platform);

	// 뛰어내릴 탑 (Falling 관찰용) — 계단으로 올라간다
	const FVector Tower(-300.f, -900.f, 0.f);
	for (int32 i = 0; i < 8; ++i)
	{
		SpawnBlock(World, Tower + FVector(-400.f + i * 50.f, 0.f, 0.f), FVector(50.f, 200.f, 40.f + i * 40.f), Colors::Neutral);
	}
	SpawnBlock(World, Tower + FVector(150.f, 0.f, 0.f), FVector(300.f, 300.f, 360.f), Colors::Platform);
	SpawnLabel(World, TEXT("탑에서 뛰어내리기 → Falling"), Tower + FVector(150.f, 0.f, 500.f), Colors::Item, 16.f);

	ModeLabel = SpawnLabel(World, TEXT(""), FVector::ZeroVector, FLinearColor::White, 20.f);
	Panel = UMovementSidePanel::Create(GetPC(), TEXT("MovementMode 전환 기록"), 360.f);
}

void ADemo0116_MovementModes::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Panel)
	{
		Panel->RemoveFromParent();
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0116_MovementModes::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AD0116_ModeCharacter* Character = Cast<AD0116_ModeCharacter>(GetPlayerPawn());
	if (!Character)
	{
		return;
	}
	if (WasKeyPressed(EKeys::F))
	{
		Character->ToggleFlying();
	}

	const UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	FLinearColor Color = Colors::Ally;
	switch (Move->MovementMode)
	{
	case MOVE_Falling:  Color = Colors::Item; break;
	case MOVE_Swimming: Color = Colors::Cyan; break;
	case MOVE_Flying:   Color = Colors::Purple; break;
	default: break;
	}
	if (AGlossaryLabel* L = ModeLabel.Get())
	{
		L->SetActorLocation(Character->GetActorLocation() + FVector(0.f, 0.f, 140.f));
		L->SetText(Move->GetMovementName());
		L->SetColor(Color);
	}
	if (Panel)
	{
		FString Body;
		for (const FString& Line : Character->GetModeLog())
		{
			Body += Line + TEXT("\n");
		}
		Panel->SetBody(Body.IsEmpty() ? TEXT("(아직 전환 없음)") : Body.TrimEnd());
	}

	const APhysicsVolume* Volume = Move->GetPhysicsVolume();
	SetStatus(FString::Printf(TEXT("MovementMode %s · 속도 %.0f cm/s\nPhysics Volume: %s%s"),
		*Move->GetMovementName(), Move->Velocity.Size(),
		Volume && Volume->bWaterVolume ? *FString::Printf(TEXT("Water (Fluid Friction %.1f)"), Volume->FluidFriction) : TEXT("기본(공기)"),
		bHasWaterVolume ? TEXT("") : TEXT(" · 경고: 레벨에 Water Volume 없음(레벨 다시 빌드)")));
}
