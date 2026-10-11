#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GlossaryDemoBase.h"
#include "GlossaryEnvironment.generated.h"

class UDirectionalLightComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class UVolumetricCloudComponent;
class UStaticMeshComponent;
class UPostProcessComponent;

/**
 * 모든 예제 레벨의 공통 환경: 해(Directional Light) + Sky Atmosphere + Sky Light + Height Fog + 격자 바닥 + 노출 고정.
 * 빌더가 데모의 Environment 설정을 복사해 레벨에 하나 놓는다. 데모는 GetWorld 에서 찾아 조명을 바꿀 수 있다.
 */
UCLASS()
class GLOSSARYCORE_API AGlossaryEnvironment : public AActor
{
	GENERATED_BODY()

public:
	AGlossaryEnvironment();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Environment)
	FGlossaryEnvironmentSettings Settings;

	/** 설정을 컴포넌트에 반영 (에디터에서 값 바꾸면 바로 보임) */
	UFUNCTION(BlueprintCallable, Category = Environment)
	void ApplySettings();

	static AGlossaryEnvironment* Find(const UObject* WorldContext);

	UDirectionalLightComponent* GetSun() const { return Sun; }
	USkyLightComponent* GetSkyLight() const { return SkyLight; }
	UExponentialHeightFogComponent* GetFog() const { return Fog; }
	UStaticMeshComponent* GetFloor() const { return Floor; }
	UPostProcessComponent* GetPostProcess() const { return PostProcess; }

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = Environment) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category = Environment) TObjectPtr<UDirectionalLightComponent> Sun;
	UPROPERTY(VisibleAnywhere, Category = Environment) TObjectPtr<USkyAtmosphereComponent> Atmosphere;
	UPROPERTY(VisibleAnywhere, Category = Environment) TObjectPtr<USkyLightComponent> SkyLight;
	UPROPERTY(VisibleAnywhere, Category = Environment) TObjectPtr<UExponentialHeightFogComponent> Fog;
	UPROPERTY(VisibleAnywhere, Category = Environment) TObjectPtr<UVolumetricCloudComponent> Clouds;
	UPROPERTY(VisibleAnywhere, Category = Environment) TObjectPtr<UStaticMeshComponent> Floor;
	UPROPERTY(VisibleAnywhere, Category = Environment) TObjectPtr<UPostProcessComponent> PostProcess;
};
