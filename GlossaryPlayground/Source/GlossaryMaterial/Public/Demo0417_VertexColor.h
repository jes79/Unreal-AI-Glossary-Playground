#pragma once

#include "CoreMinimal.h"
#include "GlossaryMaterialDemoBase.h"
#include "Demo0417_VertexColor.generated.h"

class UProceduralMeshComponent;
class UMaterialInstanceDynamic;

/**
 * 0417 Vertex Color 블렌딩
 * 정점이 촘촘한 바닥 메시(Procedural Mesh)의 정점 색 R 채널에 이끼 양을 칠하고,
 * M_VertexColorBlend 가 Lerp(돌, 이끼, VertexColor.R) 로 섞는다. 마우스 브러시가 에디터의 Mesh Paint 모드 역할을 한다.
 */
UCLASS()
class GLOSSARYMATERIAL_API ADemo0417_VertexColor : public AGlossaryMaterialDemoBase
{
	GENERATED_BODY()

public:
	ADemo0417_VertexColor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 바닥 크기(cm)와 한 변 정점 수 */
	UPROPERTY(EditAnywhere, Category = "Vertex Color")
	float GroundSize = 1600.f;

	UPROPERTY(EditAnywhere, Category = "Vertex Color", meta = (ClampMin = 2))
	int32 Resolution = 80;

	/** 브러시 반경(cm)과 초당 칠하는 양 */
	UPROPERTY(EditAnywhere, Category = "Vertex Color", meta = (ClampMin = 10))
	float BrushRadius = 160.f;

	UPROPERTY(EditAnywhere, Category = "Vertex Color", meta = (ClampMin = 0))
	float BrushStrength = 3.f;

private:
	void Paint(const FVector& WorldCenter, float Amount);
	void UploadColors();

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> Ground;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BlendMaterial;

	TArray<FVector> Vertices;
	TArray<FLinearColor> VertexColors;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;

	bool bShowVertexColor = false;
	float AutoBrushTime = 0.f;
	bool bAutoBrush = true;
};
