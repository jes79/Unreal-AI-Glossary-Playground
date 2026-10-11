#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "MovementKit.h"
#include "Demo0108_Dash.generated.h"

class AGlossaryLabel;

/**
 * 0108 대시 / 회피
 * Shift → LaunchCharacter 대시 0.2초 + 무적 + 쿨타임 1초 (AD0108_DashCharacter).
 * 양쪽 포탑이 공을 쏘고, 대시 중(무적)이면 공이 몸을 통과한다.
 */
UCLASS()
class GLOSSARYMOVEMENT_API ADemo0108_Dash : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0108_Dash();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "Dash") float FireInterval = 0.7f;
	UPROPERTY(EditAnywhere, Category = "Dash") float ProjectileSpeed = 900.f;

private:
	MovementKit::FSimpleProjectiles Projectiles;
	TArray<FVector> Turrets;
	TWeakObjectPtr<AGlossaryLabel> HeadLabel;
	float FireTimer = 0.f;
	int32 TurretIndex = 0;
	int32 Hits = 0;
	int32 Dodges = 0;
};
