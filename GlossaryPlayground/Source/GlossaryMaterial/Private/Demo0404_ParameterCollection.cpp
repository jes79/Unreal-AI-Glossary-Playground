#include "Demo0404_ParameterCollection.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Pawn.h"
#include "GlossaryCharacter.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"

using namespace GlossaryKit;

ADemo0404_ParameterCollection::ADemo0404_ParameterCollection()
{
	DemoId = TEXT("0404");
	Controls = TEXT("WASD 이동 · 마우스 시점 — 풀밭을 걸어 다녀 보세요\n")
		TEXT("C : MPC PlayerPosition 갱신 ON/OFF    [ / ] : PushRadius −/+ 50\n")
		TEXT("관찰: 값 하나(MPC_Player)를 풀 머티리얼과 바닥 링 머티리얼이 함께 읽는다");
	Showcase = TEXT("MPC PlayerPosition 으로 플레이어 근처 풀이 눌리고 바닥 링이 따라옴 (머티리얼 2개가 값 공유)");
	PlayerPawnClass = AGlossaryCharacter::StaticClass();   // 이 데모는 직접 걸어 다닌다
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-500.f, 0.f, 100.f));
}

void ADemo0404_ParameterCollection::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	Collection = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Glossary/04_Material/MPC_Player.MPC_Player"));

	// 흙바닥 (M_Soil_MPC: PushRadius 링을 그린다)
	AStaticMeshActor* Soil = SpawnWithMaterial(EShape::Plane, FVector(-300.f, 0.f, 1.f), FVector(FieldSize.X + 200.f, FieldSize.Y + 200.f, 1.f), LoadMaterial(TEXT("M_Soil_MPC")));
	Soil->SetActorEnableCollision(false);

	// 풀잎: 원뿔 수천 개를 Instanced Static Mesh 로 (M_Grass_MPC 의 WPO 가 플레이어 근처를 밀어낸다)
	UInstancedStaticMeshComponent* Grass = NewObject<UInstancedStaticMeshComponent>(this, TEXT("Grass"));
	Grass->SetStaticMesh(GetShapeMesh(EShape::Cone));
	Grass->SetMaterial(0, LoadMaterial(TEXT("M_Grass_MPC")));
	Grass->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Grass->SetupAttachment(GetRootComponent());
	Grass->RegisterComponent();
	AddInstanceComponent(Grass);

	FRandomStream Random(404);
	TArray<FTransform> Blades;
	for (float X = -FieldSize.X * 0.5f; X <= FieldSize.X * 0.5f; X += BladeSpacing)
	{
		for (float Y = -FieldSize.Y * 0.5f; Y <= FieldSize.Y * 0.5f; Y += BladeSpacing)
		{
			const float Height = Random.FRandRange(55.f, 95.f);
			const FVector Location(X - 300.f + Random.FRandRange(-10.f, 10.f), Y + Random.FRandRange(-10.f, 10.f), Height * 0.5f);
			Blades.Add(FTransform(FRotator(0.f, Random.FRandRange(0.f, 360.f), 0.f), Location, FVector(0.07f, 0.07f, Height / 100.f)));
		}
	}
	Grass->AddInstances(Blades, false, true);

	SpawnLabel(World, TEXT("MPC_Player.PlayerPosition 을 읽는 풀 (M_Grass_MPC)"), FVector(400.f, 0.f, 220.f), Colors::Item, 16.f);
}

void ADemo0404_ParameterCollection::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::C)) { bUpdateCollection = !bUpdateCollection; }
	if (WasKeyPressed(EKeys::LeftBracket)) { PushRadius = FMath::Max(50.f, PushRadius - 50.f); }
	if (WasKeyPressed(EKeys::RightBracket)) { PushRadius += 50.f; }

	FVector PlayerLocation = FVector(0.f, 0.f, -100000.f);
	if (APawn* Pawn = GetPlayerPawn())
	{
		if (bUpdateCollection)
		{
			PlayerLocation = Pawn->GetActorLocation();
		}
	}
	if (Collection)
	{
		// 핵심: 전역 값 하나를 바꾸면 이 MPC 를 쓰는 모든 머티리얼이 같은 값을 본다
		UKismetMaterialLibrary::SetVectorParameterValue(this, Collection, TEXT("PlayerPosition"), FLinearColor(PlayerLocation));
		UKismetMaterialLibrary::SetScalarParameterValue(this, Collection, TEXT("PushRadius"), PushRadius);
	}

	SetStatus(FString::Printf(TEXT("MPC_Player.PlayerPosition = (%.0f, %.0f, %.0f)%s\nPushRadius %.0f · 읽는 머티리얼: M_Grass_MPC, M_Soil_MPC"),
		PlayerLocation.X, PlayerLocation.Y, PlayerLocation.Z, bUpdateCollection ? TEXT("") : TEXT("  (갱신 OFF → 멀리 보냄)"), PushRadius));
}
