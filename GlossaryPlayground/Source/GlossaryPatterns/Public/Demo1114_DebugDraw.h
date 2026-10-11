#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GlossaryDemoBase.h"
#include "D11_SidePanel.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"
#include "Demo1114_DebugDraw.generated.h"

class UStaticMeshComponent;

/**
 * 순찰 경비병. 시야(원뿔)·순찰 경로·플레이어까지의 시선을 매 프레임 DrawDebug 로 그리고,
 * 같은 내용을 UE_VLOG_* 로 Visual Logger 에 기록한다(녹화 중일 때만 비용이 든다).
 * IVisualLoggerDebugSnapshotInterface: Visual Logger 가 프레임마다 이 액터의 상태 표를 찍어 갈 때 부르는 함수.
 */
UCLASS()
class GLOSSARYPATTERNS_API AD1114_Guard : public AActor, public IVisualLoggerDebugSnapshotInterface
{
	GENERATED_BODY()

public:
	AD1114_Guard();

	/** 시야 거리 (cm) */
	UPROPERTY(EditAnywhere, Category = Sight, meta = (ClampMin = 100))
	float SightRange = 1400.f;

	/** 시야 반각 (도) — 원뿔 전체 폭은 이 값의 두 배 */
	UPROPERTY(EditAnywhere, Category = Sight, meta = (ClampMin = 1, ClampMax = 89))
	float SightHalfAngle = 30.f;

	UPROPERTY(EditAnywhere, Category = Patrol, meta = (ClampMin = 10))
	float PatrolSpeed = 260.f;

	UPROPERTY(EditAnywhere, Category = Patrol)
	TArray<FVector> Waypoints;

	bool bDrawDebug = true;
	bool bDrawPath = true;
	bool bSeesPlayer = false;
	int32 VLogEntries = 0;

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = Components)
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = Components)
	TObjectPtr<UStaticMeshComponent> Eye;

	int32 TargetIndex = 0;
	float LookAround = 0.f;
};

/**
 * 1114 디버그 그리기 (DrawDebug · Visual Logger)
 * 1 = DrawDebug 켜기/끄기(끄면 아무것도 안 보인다 → 그리기의 가치), 2 = 경로만 켜기/끄기, V = Visual Logger 녹화 시작/정지.
 * 녹화 파일(.bvlog)은 Saved/Logs 에 저장되고, 에디터 Tools > Debug > Visual Logger 에서 열어 타임라인을 되감아 본다.
 */
UCLASS()
class GLOSSARYPATTERNS_API ADemo1114_DebugDraw : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1114_DebugDraw();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void ToggleRecording();

	UPROPERTY(Transient) TObjectPtr<AD1114_Guard> Guard;
	UPROPERTY(Transient) TObjectPtr<UD11_SidePanel> Panel;
	bool bRecording = false;
	double RecordStart = 0.0;
	int32 EntriesAtStart = 0;
	FD11_LogLines Log{4};
};
