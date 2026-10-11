#include "Demo0417_VertexColor.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

using namespace GlossaryKit;

ADemo0417_VertexColor::ADemo0417_VertexColor()
{
	DemoId = TEXT("0417");
	Controls = TEXT("왼쪽 버튼 누른 채 드래그 : 이끼 칠하기 (R=1)   오른쪽 버튼 : 지우기 (R=0)\n")
		TEXT("C : 정점 색 그대로 보기 ON/OFF    A : 자동 브러시 ON/OFF\n")
		TEXT("관찰: 텍스처는 돌·이끼 두 장뿐 — 어디에 이끼가 나는지는 정점 색 R 이 결정한다 (Mesh Paint 와 같은 데이터)");
	Showcase = TEXT("정점 색 R 채널을 마우스로 칠해 돌 바닥에 이끼 텍스처를 섞음, 정점 색 보기 토글");
	bShowMouseCursor = true;
}

void ADemo0417_VertexColor::BeginPlay()
{
	Super::BeginPlay();
	SetupFixedCamera(FVector(-1100.f, -200.f, 1000.f), FVector(150.f, 150.f, 0.f), 60.f);

	// 정점이 촘촘한 바닥 격자 (정점 색은 정점 사이에서 보간되므로 정점이 많을수록 붓 자국이 매끈하다)
	const int32 N = Resolution;
	const float Half = GroundSize * 0.5f;
	for (int32 J = 0; J <= N; ++J)
	{
		for (int32 I = 0; I <= N; ++I)
		{
			const float U = I / float(N), V = J / float(N);
			Vertices.Add(FVector(-Half + U * GroundSize, -Half + V * GroundSize, 0.f));
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D(U, V));
			VertexColors.Add(FLinearColor::Black);
		}
	}
	for (int32 J = 0; J < N; ++J)
	{
		for (int32 I = 0; I < N; ++I)
		{
			const int32 A = J * (N + 1) + I, B = A + 1, C = A + N + 1, D = C + 1;
			Triangles.Append({ A, C, B, B, C, D });
		}
	}
	Ground = NewObject<UProceduralMeshComponent>(this, TEXT("PaintableGround"));
	Ground->SetupAttachment(GetRootComponent());
	Ground->SetRelativeLocation(FVector(150.f, 150.f, 2.f));
	Ground->SetCollisionProfileName(TEXT("BlockAll"));   // 마우스 브러시 트레이스용
	Ground->RegisterComponent();
	AddInstanceComponent(Ground);
	Ground->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, {}, true);
	BlendMaterial = UMaterialInstanceDynamic::Create(LoadMaterial(TEXT("M_VertexColorBlend")), this);
	Ground->SetMaterial(0, BlendMaterial);

	// 처음부터 몇 군데 칠해 둔다 (그늘진 구석에 이끼가 낀 느낌)
	const FVector Origin = Ground->GetComponentLocation();
	Paint(Origin + FVector(-400.f, -350.f, 0.f), 1.f);
	Paint(Origin + FVector(-250.f, -200.f, 0.f), 1.f);
	Paint(Origin + FVector(300.f, 400.f, 0.f), 1.f);
	Paint(Origin + FVector(450.f, 250.f, 0.f), 0.6f);
	UploadColors();

	SpawnLabel(GetWorld(), TEXT("M_VertexColorBlend = Lerp(돌, 이끼, VertexColor.R)"), Origin + FVector(700.f, 0.f, 150.f), Colors::Item, 16.f);
}

void ADemo0417_VertexColor::Paint(const FVector& WorldCenter, float Amount)
{
	// 브러시 반경 안 정점의 R 을 부드럽게 올리거나(+) 내린다(−)
	const FVector Local = Ground->GetComponentTransform().InverseTransformPosition(WorldCenter);
	for (int32 I = 0; I < Vertices.Num(); ++I)
	{
		const float Dist = FVector::Dist2D(Vertices[I], Local);
		if (Dist < BrushRadius)
		{
			const float Falloff = FMath::SmoothStep(0.f, 1.f, 1.f - Dist / BrushRadius);
			VertexColors[I].R = FMath::Clamp(VertexColors[I].R + Amount * Falloff, 0.f, 1.f);
		}
	}
}

void ADemo0417_VertexColor::UploadColors()
{
	// 위치는 그대로 두고 정점 색만 GPU 로 다시 보낸다
	Ground->UpdateMeshSection_LinearColor(0, Vertices, Normals, UVs, VertexColors, {});
}

void ADemo0417_VertexColor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::C))
	{
		bShowVertexColor = !bShowVertexColor;
		BlendMaterial->SetScalarParameterValue(TEXT("ShowVertexColor"), bShowVertexColor ? 1.f : 0.f);
	}
	if (WasKeyPressed(EKeys::A)) { bAutoBrush = !bAutoBrush; }

	bool bPainted = false;
	const bool bAdd = IsKeyDown(EKeys::LeftMouseButton), bErase = IsKeyDown(EKeys::RightMouseButton);
	if (bAdd || bErase)
	{
		FHitResult Hit;
		if (GetMouseWorldHit(Hit))
		{
			Paint(Hit.ImpactPoint, (bAdd ? 1.f : -1.f) * BrushStrength * DeltaSeconds);
			bPainted = true;
		}
	}
	else if (bAutoBrush)
	{
		// 자동 브러시: 8자 궤적으로 천천히 칠한다 (손대지 않아도 Mesh Paint 과정이 보이게)
		AutoBrushTime += DeltaSeconds;
		const FVector Center = Ground->GetComponentLocation() + FVector(FMath::Sin(AutoBrushTime * 0.6f) * 500.f, FMath::Sin(AutoBrushTime * 1.2f) * 300.f, 0.f);
		Paint(Center, BrushStrength * 0.5f * DeltaSeconds);
		bPainted = true;
	}
	if (bPainted)
	{
		UploadColors();
	}

	int32 Mossy = 0;
	for (const FLinearColor& C : VertexColors) { Mossy += C.R > 0.5f ? 1 : 0; }
	SetStatus(FString::Printf(TEXT("이끼(R>0.5) 정점 %d / %d (%.0f%%)%s%s"), Mossy, VertexColors.Num(), 100.f * Mossy / FMath::Max(1, VertexColors.Num()),
		bShowVertexColor ? TEXT(" · 정점 색 보기") : TEXT(""), bAutoBrush ? TEXT(" · 자동 브러시") : TEXT("")));
}
