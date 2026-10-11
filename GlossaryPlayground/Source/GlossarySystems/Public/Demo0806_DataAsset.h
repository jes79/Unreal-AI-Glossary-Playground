#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GlossaryDemoBase.h"
#include "Demo0806_DataAsset.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class AD08_Dummy;
class UD08_SidePanel;

/**
 * 무기 하나의 설정값. 코드는 이 클래스 하나, 무기는 에셋 파일(DA_0806_*) 여러 개.
 * Primary Data Asset 이라 Asset Manager 가 "Weapon:DA_0806_Rifle" 같은 ID 로 찾을 수 있다.
 */
UCLASS(BlueprintType)
class GLOSSARYSYSTEMS_API UD0806_WeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon") FString WeaponName;
	/** 한 발 데미지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon") float Damage = 10.f;
	/** 연사속도 (발/초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon") float FireRate = 2.f;
	/** 손에 드는 메시 (소프트 참조 — 무기를 고를 때만 로드) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon") TSoftObjectPtr<UStaticMesh> Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon") FVector MeshSize = FVector(60.f, 10.f, 10.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon") FLinearColor Color = FLinearColor::White;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("Weapon"), GetFName());
	}
};

/**
 * 0806 Data Asset
 * 권총·소총·샷건 Data Asset 3개를 1/2/3 으로 바꿔 끼우면, 같은 발사 코드가 에셋 값대로 쏜다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API ADemo0806_DataAsset : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0806_DataAsset();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 무기 에셋 목록 (디테일 패널에서 다른 에셋을 끼워 넣을 수 있다) */
	UPROPERTY(EditAnywhere, Category = "Data Asset") TArray<TSoftObjectPtr<UD0806_WeaponData>> Weapons;

	UPROPERTY(EditAnywhere, Category = "Data Asset") bool bAutoFire = true;

private:
	void Equip(int32 Index);
	void Fire();

	UPROPERTY(Transient) TObjectPtr<UD0806_WeaponData> Current;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> WeaponMesh;
	UPROPERTY(Transient) TObjectPtr<AD08_Dummy> Target;
	UPROPERTY(Transient) TObjectPtr<UD08_SidePanel> Panel;
	float Cooldown = 0.f;
	int32 ShotsFired = 0;
	float EquipTime = 0.f;
};
