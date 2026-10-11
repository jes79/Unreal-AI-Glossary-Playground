#include "GlossaryMaterialDemoBase.h"

#include "Animation/AnimSequence.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Animation/SkeletalMeshActor.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogGlossaryMaterial, Log, All);

AGlossaryMaterialDemoBase::AGlossaryMaterialDemoBase()
{
	// 머티리얼 데모는 보는 것이 중심이라 기본은 고정 카메라
	PlayerPawnClass = nullptr;
}

void AGlossaryMaterialDemoBase::SetupFixedCamera(const FVector& Location, const FVector& LookAt, float FOV)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	DemoCamera = GetWorld()->SpawnActor<ACameraActor>(Location, (LookAt - Location).Rotation(), Params);
	DemoCamera->GetCameraComponent()->SetFieldOfView(FOV);
	DemoCamera->GetCameraComponent()->bConstrainAspectRatio = false;
	bViewTargetApplied = false;
}

void AGlossaryMaterialDemoBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// 폰 빙의가 끝난 뒤에 시점을 바꿔야 덮어쓰이지 않는다 → 첫 Tick 에서 적용
	if (DemoCamera && !bViewTargetApplied)
	{
		if (APlayerController* PC = GetPC())
		{
			PC->SetViewTarget(DemoCamera);
			bViewTargetApplied = true;
		}
	}
}

UMaterialInterface* AGlossaryMaterialDemoBase::LoadMaterial(const FString& Name)
{
	UMaterialInterface* Material = GlossaryKit::LoadAsset<UMaterialInterface>(TEXT("/Game/Glossary/04_Material/") + Name);
	if (!Material)
	{
		UE_LOG(LogGlossaryMaterial, Warning, TEXT("[Glossary] 머티리얼 없음: %s (tools/glossary-build.sh -filter=04 로 생성)"), *Name);
		Material = GlossaryKit::GetColorMaterial();
	}
	return Material;
}

AStaticMeshActor* AGlossaryMaterialDemoBase::SpawnWithMaterial(GlossaryKit::EShape Shape, const FVector& Location, const FVector& Size,
	UMaterialInterface* Material, const FRotator& Rotation)
{
	AStaticMeshActor* Actor = GlossaryKit::SpawnShape(GetWorld(), Shape, Location, Size, GlossaryKit::Colors::Neutral, Rotation);
	if (Actor && Material)
	{
		Actor->GetStaticMeshComponent()->SetMaterial(0, Material);
	}
	return Actor;
}

ASkeletalMeshActor* AGlossaryMaterialDemoBase::SpawnMannequin(const FVector& Location, const FRotator& Rotation, UMaterialInterface* Material)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASkeletalMeshActor* Actor = GetWorld()->SpawnActor<ASkeletalMeshActor>(Location, Rotation, Params);
	USkeletalMeshComponent* Mesh = Actor->GetSkeletalMeshComponent();
	Mesh->SetSkeletalMesh(GlossaryKit::LoadAsset<USkeletalMesh>(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")));
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	if (UAnimSequence* Idle = GlossaryKit::LoadAsset<UAnimSequence>(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle")))
	{
		Mesh->PlayAnimation(Idle, true);
	}
	if (Material)
	{
		SetAllMaterials(Mesh, Material);
	}
	return Actor;
}

void AGlossaryMaterialDemoBase::SetAllMaterials(UPrimitiveComponent* Component, UMaterialInterface* Material)
{
	if (!Component)
	{
		return;
	}
	for (int32 Index = 0; Index < Component->GetNumMaterials(); ++Index)
	{
		Component->SetMaterial(Index, Material);
	}
}
