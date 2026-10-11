#include "GlossaryKit.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GlossaryCatalog.h"
#include "GlossaryLabel.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"

namespace GlossaryKit
{
	static const TCHAR* ShapePath(EShape Shape)
	{
		switch (Shape)
		{
		case EShape::Sphere:   return TEXT("/Engine/BasicShapes/Sphere.Sphere");
		case EShape::Cylinder: return TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
		case EShape::Cone:     return TEXT("/Engine/BasicShapes/Cone.Cone");
		case EShape::Plane:    return TEXT("/Engine/BasicShapes/Plane.Plane");
		default:               return TEXT("/Engine/BasicShapes/Cube.Cube");
		}
	}

	UStaticMesh* GetShapeMesh(EShape Shape)
	{
		return LoadObject<UStaticMesh>(nullptr, ShapePath(Shape));
	}

	UMaterialInterface* GetColorMaterial()
	{
		static TWeakObjectPtr<UMaterialInterface> Cached;
		if (!Cached.IsValid())
		{
			UMaterialInterface* Found = LoadAsset<UMaterialInterface>(TEXT("/Game/Glossary/Common/M_Color"));
			if (!Found)
			{
				Found = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
			}
			Cached = Found;
		}
		return Cached.Get();
	}

	UMaterialInstanceDynamic* SetColor(UPrimitiveComponent* Component, const FLinearColor& Color, float Roughness, float Metallic, float Emissive)
	{
		if (!Component)
		{
			return nullptr;
		}
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(GetColorMaterial(), Component);
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetScalarParameterValue(TEXT("Roughness"), Roughness);
		MID->SetScalarParameterValue(TEXT("Metallic"), Metallic);
		MID->SetScalarParameterValue(TEXT("Emissive"), Emissive);
		Component->SetMaterial(0, MID);
		return MID;
	}

	static FVector ShapeScale(EShape Shape, const FVector& Size)
	{
		// 엔진 기본 도형은 모두 100cm 크기. Plane 은 두께가 없으므로 Z 무시.
		return Size / 100.f;
	}

	void InitShapeComponent(UStaticMeshComponent* Component, EShape Shape, const FVector& Size)
	{
		Component->SetStaticMesh(GetShapeMesh(Shape));
		Component->SetRelativeScale3D(ShapeScale(Shape, Size));
	}

	AStaticMeshActor* SpawnShape(UWorld* World, EShape Shape, const FVector& Location, const FVector& Size,
		const FLinearColor& Color, const FRotator& Rotation, bool bPhysics, bool bMovable)
	{
		if (!World)
		{
			return nullptr;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Location, Rotation, Params);
		UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetStaticMesh(GetShapeMesh(Shape));
		Actor->SetActorScale3D(ShapeScale(Shape, Size));
		SetColor(Mesh, Color);
		if (bPhysics)
		{
			Mesh->SetSimulatePhysics(true);
		}
		else if (!bMovable)
		{
			Mesh->SetMobility(EComponentMobility::Static);
		}
		return Actor;
	}

	AStaticMeshActor* SpawnBlock(UWorld* World, const FVector& BottomCenter, const FVector& Size, const FLinearColor& Color, const FRotator& Rotation)
	{
		return SpawnShape(World, EShape::Cube, BottomCenter + FVector(0.f, 0.f, Size.Z * 0.5f), Size, Color, Rotation);
	}

	UStaticMeshComponent* AddShape(AActor* Owner, EShape Shape, const FVector& RelativeLocation, const FVector& Size,
		const FLinearColor& Color, USceneComponent* Parent, const FRotator& RelativeRotation)
	{
		if (!Owner)
		{
			return nullptr;
		}
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Owner);
		Mesh->SetMobility(EComponentMobility::Movable);
		InitShapeComponent(Mesh, Shape, Size);
		USceneComponent* AttachTo = Parent ? Parent : Owner->GetRootComponent();
		if (AttachTo)
		{
			Mesh->SetupAttachment(AttachTo);
		}
		Mesh->SetRelativeLocationAndRotation(RelativeLocation, RelativeRotation);
		Mesh->RegisterComponent();
		Owner->AddInstanceComponent(Mesh);
		if (!AttachTo)
		{
			Owner->SetRootComponent(Mesh);
		}
		SetColor(Mesh, Color);
		return Mesh;
	}

	AGlossaryLabel* SpawnLabel(UWorld* World, const FString& Text, const FVector& Location, const FLinearColor& Color, float FontSize)
	{
		if (!World)
		{
			return nullptr;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AGlossaryLabel* Label = World->SpawnActor<AGlossaryLabel>(Location, FRotator::ZeroRotator, Params);
		Label->SetText(Text);
		Label->SetColor(Color);
		Label->SetFontSize(FontSize);
		return Label;
	}

	UObject* LoadAssetObject(UClass* Class, const FString& Path)
	{
		FString ObjectPath = Path;
		if (!ObjectPath.Contains(TEXT(".")))
		{
			ObjectPath += TEXT(".") + FPackageName::GetShortName(Path);
		}
		return StaticLoadObject(Class, nullptr, *ObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	FString AssetPath(const FString& DemoId, const FString& AssetName)
	{
		return UGlossaryCatalog::GetAssetFolder(DemoId) / AssetName;
	}
}
