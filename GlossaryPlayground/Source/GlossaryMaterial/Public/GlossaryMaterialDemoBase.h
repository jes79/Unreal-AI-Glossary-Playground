#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "GlossaryKit.h"
#include "GlossaryMaterialDemoBase.generated.h"

class ACameraActor;
class AStaticMeshActor;
class ASkeletalMeshActor;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPrimitiveComponent;

/**
 * 04 머티리얼 분야 데모 공통 베이스.
 * 머티리얼은 "보는" 데모가 대부분이라 기본은 고정 카메라(플레이어 폰 없음)이고,
 * 빌더가 만든 /Game/Glossary/04_Material 에셋을 불러 도형에 입히는 헬퍼를 제공한다.
 */
UCLASS(Abstract)
class GLOSSARYMATERIAL_API AGlossaryMaterialDemoBase : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	AGlossaryMaterialDemoBase();

protected:
	virtual void Tick(float DeltaSeconds) override;

	/** 고정 카메라를 놓고 시점으로 쓴다 (PlayerPawnClass 가 null 일 때) */
	void SetupFixedCamera(const FVector& Location, const FVector& LookAt, float FOV = 60.f);

	/** "/Game/Glossary/04_Material/{Name}" 머티리얼. 없으면 경고 후 M_Color */
	static UMaterialInterface* LoadMaterial(const FString& Name);

	/** 도형을 스폰하고 0번 슬롯에 머티리얼을 입힌다 */
	AStaticMeshActor* SpawnWithMaterial(GlossaryKit::EShape Shape, const FVector& Location, const FVector& Size,
		UMaterialInterface* Material, const FRotator& Rotation = FRotator::ZeroRotator);

	/** 마네킹(SKM_Manny_Simple) + Idle 애니메이션. Material 이 있으면 모든 슬롯에 입힌다 */
	ASkeletalMeshActor* SpawnMannequin(const FVector& Location, const FRotator& Rotation, UMaterialInterface* Material = nullptr);

	/** 컴포넌트의 모든 슬롯에 같은 머티리얼 */
	static void SetAllMaterials(UPrimitiveComponent* Component, UMaterialInterface* Material);

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> DemoCamera;

private:
	bool bViewTargetApplied = false;
};
