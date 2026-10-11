#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0816_Timer.generated.h"

class AD08_Dummy;
class UD08_SidePanel;
class UStaticMeshComponent;

/**
 * 0816 Timer (SetTimer)
 * Tick 대신 GetWorldTimerManager().SetTimer 로 1초마다 독 데미지를 주고, 5번째에 ClearTimer 한다.
 * 같은 시간 동안 Tick 이 몇 번 불렸는지와 비교해 보여 준다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API ADemo0816_Timer : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0816_Timer();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 타이머 간격 (초) */
	UPROPERTY(EditAnywhere, Category = "Timer", meta = (ClampMin = 0.05)) float PoisonInterval = 1.f;
	/** 몇 번 주고 멈출지 */
	UPROPERTY(EditAnywhere, Category = "Timer", meta = (ClampMin = 1)) int32 PoisonTicks = 5;
	UPROPERTY(EditAnywhere, Category = "Timer") float PoisonDamage = 10.f;

private:
	void StartPoison();
	void StopPoison(const TCHAR* Reason);
	/** 타이머가 부르는 함수 */
	void PoisonTick();

	FTimerHandle PoisonTimer;
	int32 TicksDone = 0;
	int32 FrameTicksWhilePoisoned = 0;

	UPROPERTY(Transient) TObjectPtr<AD08_Dummy> Target;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> TickLamps;
	UPROPERTY(Transient) TObjectPtr<UD08_SidePanel> Panel;
	FString LastEvent;
};
