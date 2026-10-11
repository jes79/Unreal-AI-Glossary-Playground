#pragma once

#include "CoreMinimal.h"
#include "GlossaryMaterialDemoBase.h"
#include "Demo0402_MaterialInstance.generated.h"

class UStaticMeshComponent;

/**
 * 0402 Material Instance
 * 부모 M_Base(Color·Roughness 파라미터) 하나와 색만 바꾼 Material Instance Constant 5개(MI_Base_*)를 나란히 놓는다.
 * C 로 다섯 물체를 모두 부모로 되돌려 "같은 셰이더, 값만 다름" 을 확인한다.
 */
UCLASS()
class GLOSSARYMATERIAL_API ADemo0402_MaterialInstance : public AGlossaryMaterialDemoBase
{
	GENERATED_BODY()

public:
	ADemo0402_MaterialInstance();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 빌더가 만든 인스턴스 이름 (부모 M_Base) */
	UPROPERTY(EditAnywhere, Category = "Material Instance")
	TArray<FString> InstanceNames = { TEXT("MI_Base_Red"), TEXT("MI_Base_Orange"), TEXT("MI_Base_Green"), TEXT("MI_Base_Blue"), TEXT("MI_Base_Purple") };

private:
	void Apply();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Meshes;

	bool bUseInstances = true;
	int32 Selected = 0;
};
