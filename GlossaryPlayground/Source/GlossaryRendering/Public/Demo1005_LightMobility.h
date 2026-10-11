#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "RenderingDemoKit.h"
#include "Demo1005_LightMobility.generated.h"

class ALight;
class AStaticMeshActor;

/**
 * 1005 Light Mobility (Static · Stationary · Movable)
 * 빌더(BuildLevel)가 실내 천장등 2개를 Static, 책상 스탠드를 Stationary, 흔들리는 전등을 Movable 로 레벨에 놓는다.
 * 실행 중 색 바꾸기 / 옮기기를 시도해 보면 Static 은 아무것도 안 바뀌고, Stationary 는 색·세기만, Movable 은 전부 된다.
 */
UCLASS()
class GLOSSARYRENDERING_API ADemo1005_LightMobility : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1005_LightMobility();

#if WITH_EDITOR
	virtual void BuildLevel(UWorld* World) override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 흔들리는 전등 진폭 (도) */
	UPROPERTY(EditAnywhere, Category = "Light Mobility")
	float SwingAngle = 30.f;

	UPROPERTY(EditAnywhere, Category = "Light Mobility")
	bool bSwing = true;

	UPROPERTY(Transient) TArray<TObjectPtr<ALight>> Lights;
	UPROPERTY(Transient) TObjectPtr<ALight> SwingLight;
	UPROPERTY(Transient) TObjectPtr<AStaticMeshActor> SwingBulb;
	UPROPERTY(Transient) TObjectPtr<AStaticMeshActor> SwingCord;

private:
	float SwingTime = 0.f;
	FString LastAction;
	RenderKit::FFrameStats Stats;
};
