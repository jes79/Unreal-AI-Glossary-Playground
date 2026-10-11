#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0819_DayNight.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UD08_SidePanel;

/**
 * 0819 낮밤 주기
 * 환경의 해(Directional Light)를 1분에 한 바퀴(하루) 돌린다. Sky Atmosphere 가 해 각도를 읽어
 * 하늘색·노을·밤하늘을 자동으로 그리고, 해가 지면 가로등이 켜진다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API ADemo0819_DayNight : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0819_DayNight();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 실제 몇 초가 게임 속 하루(24시간)인지 */
	UPROPERTY(EditAnywhere, Category = "Day Night", meta = (ClampMin = 1)) float DayLengthSeconds = 60.f;
	/** 시작 시각 (0~24) */
	UPROPERTY(EditAnywhere, Category = "Day Night", meta = (ClampMin = 0, ClampMax = 24)) float StartHour = 15.f;
	/** 해의 방위각 (지평선 위를 지나가는 방향) */
	UPROPERTY(EditAnywhere, Category = "Day Night") float SunYaw = -60.f;
	/** 한낮 해 밝기 (lux) */
	UPROPERTY(EditAnywhere, Category = "Day Night") float NoonIntensity = 8.f;

private:
	void ApplyTime();
	FString PhaseName() const;

	UPROPERTY(Transient) TArray<TObjectPtr<UPointLightComponent>> Lamps;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> BulbMIDs;
	UPROPERTY(Transient) TObjectPtr<UD08_SidePanel> Panel;
	float Hour = 0.f;
	float SpeedScale = 1.f;
	bool bPaused = false;
};
