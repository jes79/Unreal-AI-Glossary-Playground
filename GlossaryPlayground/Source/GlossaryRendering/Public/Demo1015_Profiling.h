#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "RenderingDemoKit.h"
#include "Demo1015_Profiling.generated.h"

class AStaticMeshActor;

/**
 * 1015 프로파일링 (stat unit · Unreal Insights)
 * 일부러 병목을 만드는 스위치 3개(Game 스레드 계산 / Draw 스레드 액터 수천 개 / GPU 해상도 200%)를 켜 보고,
 * stat unit 의 Game·Draw·GPU 중 어느 숫자가 커지는지로 병목을 찾는다. T 로 Unreal Insights 트레이스 파일을 뜬다.
 */
UCLASS()
class GLOSSARYRENDERING_API ADemo1015_Profiling : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1015_Profiling();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Game 병목: 매 프레임 이 시간(ms)만큼 계산하는 함수 (Insights 에서 이름으로 보인다) */
	UPROPERTY(EditAnywhere, Category = "Profiling") float GameWorkMs = 12.f;
	/** Draw 병목: 머티리얼이 모두 다른 상자 액터 수 */
	UPROPERTY(EditAnywhere, Category = "Profiling") int32 DrawHeavyActorCount = 4000;
	/** GPU 병목: r.ScreenPercentage */
	UPROPERTY(EditAnywhere, Category = "Profiling") int32 GPUHeavyScreenPercentage = 200;

	UPROPERTY(Transient) TArray<TObjectPtr<AStaticMeshActor>> DrawHeavyActors;

private:
	/** 일부러 느린 게임 로직 — Unreal Insights 의 Timing 창에서 이 이름이 가장 길게 보인다 */
	void SimulateExpensiveGameLogic();
	void SetDrawHeavy(bool bOn);
	void Exec(const TCHAR* Command);

	bool bGameHeavy = false;
	bool bDrawHeavy = false;
	bool bGPUHeavy = false;
	bool bTracing = false;
	volatile double Sink = 0.0;
	RenderKit::FCVarScope CVars;
	RenderKit::FFrameStats Stats;
};
