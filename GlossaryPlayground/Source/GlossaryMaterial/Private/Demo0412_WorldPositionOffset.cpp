#include "Demo0412_WorldPositionOffset.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

using namespace GlossaryKit;

ADemo0412_WorldPositionOffset::ADemo0412_WorldPositionOffset()
{
	DemoId = TEXT("0412");
	Controls = TEXT("C : WPO 바람 ON/OFF    [ / ] : WindStrength −/+ 10    , / . : 빈도 −/+\n")
		TEXT("관찰: 풀 끝과 깃발 끝만 Sine(Time) 으로 흔들린다(정점 셰이더). 콜리전·CPU 위치는 그대로");
	Showcase = TEXT("WPO 에 Sine(Time) 을 연결해 풀밭과 깃발이 바람에 흔들림, 세기·빈도 조절");
	Environment.bClouds = true;
}

void ADemo0412_WorldPositionOffset::BeginPlay()
{
	Super::BeginPlay();
	SetupFixedCamera(FVector(-1300.f, -80.f, 520.f), FVector(200.f, -80.f, 220.f), 60.f);

	// 풀밭: 원뿔 인스턴스 (Instanced Static Mesh)
	GrassMaterial = UMaterialInstanceDynamic::Create(LoadMaterial(TEXT("M_WindGrass")), this);
	UInstancedStaticMeshComponent* Grass = NewObject<UInstancedStaticMeshComponent>(this, TEXT("Grass"));
	Grass->SetStaticMesh(GetShapeMesh(EShape::Cone));
	Grass->SetMaterial(0, GrassMaterial);
	Grass->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Grass->SetupAttachment(GetRootComponent());
	Grass->RegisterComponent();
	AddInstanceComponent(Grass);
	FRandomStream Random(412);
	TArray<FTransform> Blades;
	for (float X = -400.f; X <= 900.f; X += 26.f)
	{
		for (float Y = -1000.f; Y <= 700.f; Y += 26.f)
		{
			const float Height = Random.FRandRange(60.f, 110.f);
			Blades.Add(FTransform(FRotator(0.f, Random.FRandRange(0.f, 360.f), 0.f),
				FVector(X + Random.FRandRange(-9.f, 9.f), Y + Random.FRandRange(-9.f, 9.f), Height * 0.5f), FVector(0.07f, 0.07f, Height / 100.f)));
		}
	}
	Grass->AddInstances(Blades, false, true);

	// 깃대 + 깃발 (정점이 많은 격자를 실행 중에 만든다)
	const FVector PolePos(200.f, 400.f, 0.f);
	SpawnShape(GetWorld(), EShape::Cylinder, PolePos + FVector(0.f, 0.f, 250.f), FVector(12.f, 12.f, 500.f), Colors::Wall);
	UProceduralMeshComponent* Flag = NewObject<UProceduralMeshComponent>(this, TEXT("Flag"));
	Flag->SetupAttachment(GetRootComponent());
	Flag->SetRelativeLocationAndRotation(PolePos + FVector(0.f, 0.f, 500.f), FRotator(0.f, -35.f, 0.f));   // 비스듬히 보이게 (역광이라 약간 어둡다)
	Flag->RegisterComponent();
	AddInstanceComponent(Flag);
	BuildFlag(Flag, 260.f, 160.f);
	FlagMaterial = UMaterialInstanceDynamic::Create(LoadMaterial(TEXT("M_Flag")), this);
	Flag->SetMaterial(0, FlagMaterial);

	SpawnLabel(GetWorld(), TEXT("M_Flag : WPO = Sine(Time − U) × U"), PolePos + FVector(0.f, -100.f, 540.f), Colors::Item, 15.f);
	SpawnLabel(GetWorld(), TEXT("M_WindGrass : WPO = Sine(Time) × 높이²"), FVector(-300.f, -550.f, 200.f), Colors::Item, 15.f);
	Apply();
}

void ADemo0412_WorldPositionOffset::BuildFlag(UProceduralMeshComponent* Mesh, float Width, float Height)
{
	// XZ 평면(카메라를 향함) 격자. U = 깃대(0)→끝(1), V = 위(0)→아래(1)
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	const int32 NX = FlagResolution.X, NY = FlagResolution.Y;
	for (int32 J = 0; J <= NY; ++J)
	{
		for (int32 I = 0; I <= NX; ++I)
		{
			const float U = I / float(NX), V = J / float(NY);
			Vertices.Add(FVector(0.f, -U * Width, -V * Height));
			Normals.Add(FVector(-1.f, 0.f, 0.f));
			UV0.Add(FVector2D(U, V));
		}
	}
	for (int32 J = 0; J < NY; ++J)
	{
		for (int32 I = 0; I < NX; ++I)
		{
			const int32 A = J * (NX + 1) + I, B = A + 1, C = A + NX + 1, D = C + 1;
			Triangles.Append({ A, C, B, B, C, D });
		}
	}
	Mesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, {}, {}, false);
}

void ADemo0412_WorldPositionOffset::Apply()
{
	for (UMaterialInstanceDynamic* MID : { GrassMaterial.Get(), FlagMaterial.Get() })
	{
		if (MID)
		{
			MID->SetScalarParameterValue(TEXT("WindStrength"), bWind ? WindStrength : 0.f);
			MID->SetScalarParameterValue(TEXT("WindFrequency"), MID == FlagMaterial ? WindFrequency * 2.f : WindFrequency);
		}
	}
}

void ADemo0412_WorldPositionOffset::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	bool bChanged = false;
	if (WasKeyPressed(EKeys::C)) { bWind = !bWind; bChanged = true; }
	if (WasKeyPressed(EKeys::LeftBracket)) { WindStrength = FMath::Max(0.f, WindStrength - 10.f); bChanged = true; }
	if (WasKeyPressed(EKeys::RightBracket)) { WindStrength += 10.f; bChanged = true; }
	if (WasKeyPressed(EKeys::Comma)) { WindFrequency = FMath::Max(0.1f, WindFrequency - 0.2f); bChanged = true; }
	if (WasKeyPressed(EKeys::Period)) { WindFrequency += 0.2f; bChanged = true; }
	if (bChanged)
	{
		Apply();
	}
	SetStatus(FString::Printf(TEXT("WPO 바람 %s · WindStrength %.0fcm · WindFrequency %.1fHz (깃발 ×2)"),
		bWind ? TEXT("ON") : TEXT("OFF"), WindStrength, WindFrequency));
}
