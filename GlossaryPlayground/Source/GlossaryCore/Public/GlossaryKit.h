#pragma once

#include "CoreMinimal.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

class AActor;
class AStaticMeshActor;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class USceneComponent;
class AGlossaryLabel;
class UWorld;

/** 데모 코드를 짧게 만드는 런타임 헬퍼. 메시는 엔진 기본 도형, 색은 생성 머티리얼 M_Color 를 쓴다. */
namespace GlossaryKit
{
	enum class EShape : uint8 { Cube, Sphere, Cylinder, Cone, Plane };

	/** 자주 쓰는 색. 플레이어=파랑, 적=빨강, 아이템=노랑 처럼 모든 데모에서 같은 의미로 쓴다. */
	namespace Colors
	{
		inline const FLinearColor Player(0.10f, 0.35f, 1.00f);
		inline const FLinearColor Enemy(0.90f, 0.12f, 0.10f);
		inline const FLinearColor Item(1.00f, 0.80f, 0.10f);
		inline const FLinearColor Ally(0.15f, 0.80f, 0.30f);
		inline const FLinearColor Wall(0.55f, 0.57f, 0.62f);
		inline const FLinearColor Ground(0.30f, 0.32f, 0.35f);
		inline const FLinearColor Platform(0.40f, 0.45f, 0.55f);
		inline const FLinearColor Accent(0.95f, 0.45f, 0.05f);
		inline const FLinearColor Neutral(0.80f, 0.80f, 0.80f);
		inline const FLinearColor Dark(0.05f, 0.05f, 0.06f);
		inline const FLinearColor Cyan(0.05f, 0.80f, 0.95f);
		inline const FLinearColor Purple(0.55f, 0.20f, 0.90f);
	}

	GLOSSARYCORE_API UStaticMesh* GetShapeMesh(EShape Shape);

	/** /Game/Glossary/Common/M_Color (Color·Roughness·Metallic·Emissive 파라미터). 없으면 엔진 BasicShapeMaterial */
	GLOSSARYCORE_API UMaterialInterface* GetColorMaterial();

	/** 컴포넌트의 0번 머티리얼을 색 MID 로 바꾼다. Emissive > 0 이면 그 배수로 스스로 빛난다. */
	GLOSSARYCORE_API UMaterialInstanceDynamic* SetColor(UPrimitiveComponent* Component, const FLinearColor& Color,
		float Roughness = 0.6f, float Metallic = 0.f, float Emissive = 0.f);

	/**
	 * 월드에 도형 액터를 만든다. Size 는 cm 단위 실제 크기(엔진 도형은 100cm 기준이라 자동으로 나눠 스케일).
	 * bMovable=false 면 Static(라이트맵·NavMesh 대상), bPhysics=true 면 Simulate Physics.
	 */
	GLOSSARYCORE_API AStaticMeshActor* SpawnShape(UWorld* World, EShape Shape, const FVector& Location, const FVector& Size,
		const FLinearColor& Color, const FRotator& Rotation = FRotator::ZeroRotator, bool bPhysics = false, bool bMovable = true);

	/** 바닥에 놓인 상자: Location 은 상자 바닥 중심 */
	GLOSSARYCORE_API AStaticMeshActor* SpawnBlock(UWorld* World, const FVector& BottomCenter, const FVector& Size,
		const FLinearColor& Color = Colors::Platform, const FRotator& Rotation = FRotator::ZeroRotator);

	/**
	 * 액터에 도형 컴포넌트를 붙인다. 생성자에서는 CreateDefaultSubobject 를 쓰고, BeginPlay 이후에는 이 함수를 쓴다.
	 * Parent 가 null 이면 루트에 붙는다.
	 */
	GLOSSARYCORE_API UStaticMeshComponent* AddShape(AActor* Owner, EShape Shape, const FVector& RelativeLocation, const FVector& Size,
		const FLinearColor& Color, USceneComponent* Parent = nullptr, const FRotator& RelativeRotation = FRotator::ZeroRotator);

	/** 생성자용: 도형 메시를 지정한다(ConstructorHelpers 대신 경로 로드). */
	GLOSSARYCORE_API void InitShapeComponent(UStaticMeshComponent* Component, EShape Shape, const FVector& Size);

	/** 머리 위에 떠 있는 한글 라벨(화면을 향하는 위젯). */
	GLOSSARYCORE_API AGlossaryLabel* SpawnLabel(UWorld* World, const FString& Text, const FVector& Location,
		const FLinearColor& Color = FLinearColor::White, float FontSize = 18.f);

	/** 에셋 경로로 로드 ("/Game/Glossary/04_Material/M_Dissolve"). 패키지 경로만 줘도 된다. */
	GLOSSARYCORE_API UObject* LoadAssetObject(UClass* Class, const FString& Path);

	template <typename T>
	T* LoadAsset(const FString& Path)
	{
		return Cast<T>(LoadAssetObject(T::StaticClass(), Path));
	}

	/** "/Game/Glossary/01_Movement/" + Name */
	GLOSSARYCORE_API FString AssetPath(const FString& DemoId, const FString& AssetName);
}
