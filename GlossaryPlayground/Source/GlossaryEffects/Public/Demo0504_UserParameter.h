#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0504_UserParameter.generated.h"

class UNiagaraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APostProcessVolume;

/**
 * 0504 User Parameter — NS_UserParam 의 User.Color / User.SpawnRate 를 C++ 에서 SetVariableLinearColor / SetVariableFloat 로 바꾼다.
 */
UCLASS()
class GLOSSARYEFFECTS_API ADemo0504_UserParameter : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0504_UserParameter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "User Parameter") FLinearColor Color = FLinearColor(0.1f, 0.6f, 1.f);
	UPROPERTY(EditAnywhere, Category = "User Parameter") float SpawnRate = 80.f;

private:
	void Apply();
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> Fountain;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Swatch;
	FString ColorName = TEXT("파랑");
};
