#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0104_Acceleration.generated.h"

class ACharacter;
class AGlossaryLabel;

/**
 * 0104 가속/감속 (Max Acceleration · Braking Deceleration · Ground Friction)
 * 같은 거리를 자동으로 왕복하는 두 러너(기본 값 vs 미끄러운 값)를 나란히 두어
 * 출발할 때 서서히 빨라지고, 멈출 때 미끄러지는 거리를 비교한다. 플레이어도 C 로 같은 값을 써 볼 수 있다.
 */
UCLASS()
class GLOSSARYMOVEMENT_API ADemo0104_Acceleration : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0104_Acceleration();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// ---- 요청 예문의 미끄러운 값 ----
	UPROPERTY(EditAnywhere, Category = "Acceleration") float MaxAcceleration = 1000.f;
	UPROPERTY(EditAnywhere, Category = "Acceleration") float BrakingDecelerationWalking = 500.f;
	UPROPERTY(EditAnywhere, Category = "Acceleration") float GroundFriction = 2.f;

	// ---- 비교용 기본 값(Third Person 템플릿과 비슷한 즉각 반응) ----
	UPROPERTY(EditAnywhere, Category = "Acceleration|Default") float DefaultMaxAcceleration = 2048.f;
	UPROPERTY(EditAnywhere, Category = "Acceleration|Default") float DefaultBrakingDecelerationWalking = 2000.f;
	UPROPERTY(EditAnywhere, Category = "Acceleration|Default") float DefaultGroundFriction = 8.f;

	/** 러너가 달리는 시간 / 멈춰 있는 시간 */
	UPROPERTY(EditAnywhere, Category = "Acceleration") float RunTime = 1.6f;
	UPROPERTY(EditAnywhere, Category = "Acceleration") float RestTime = 1.6f;

private:
	void ApplySettings(ACharacter* Character, bool bSlippery) const;
	ACharacter* SpawnRunner(const FVector& Location, bool bSlippery);

	struct FRunner
	{
		TWeakObjectPtr<ACharacter> Character;
		TWeakObjectPtr<AGlossaryLabel> Label;
		TWeakObjectPtr<AActor> StopMarker;
		bool bSlippery = false;
		float ReleaseX = 0.f;      // 입력을 뗀 순간의 X
		float LastSlide = 0.f;     // 입력을 뗀 뒤 미끄러진 거리
		float TimeToTop = 0.f;     // 출발 후 최고 속도 90% 까지 걸린 시간
		bool bReachedTop = false;
	};
	TArray<FRunner> Runners;
	float Phase = 0.f;
	float Direction = 1.f;
	bool bRunning = true;
	bool bPlayerSlippery = true;
};
