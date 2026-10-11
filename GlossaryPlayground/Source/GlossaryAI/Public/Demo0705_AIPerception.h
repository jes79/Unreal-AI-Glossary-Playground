#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GlossaryDemoBase.h"
#include "Perception/AIPerceptionTypes.h"
#include "Demo0705_AIPerception.generated.h"

class ACameraActor;
class AD07Enemy;

UENUM()
enum class ED0705_State : uint8
{
	Idle,         // 제자리 경계
	Investigate,  // 소리 난 곳 확인
	Chase,        // 보이는 플레이어 추격
	Search,       // 놓친 위치 확인
};

/**
 * AIPerception 을 가진 컨트롤러: Sight(반경 1500, 시야각 90°) + Hearing(범위 2000).
 * OnTargetPerceptionUpdated 로 "보임/놓침/들림" 자극을 받아 상태를 바꾼다.
 */
UCLASS()
class GLOSSARYAI_API AD0705_PerceptionController : public AAIController
{
	GENERATED_BODY()

public:
	AD0705_PerceptionController();

	ED0705_State State = ED0705_State::Idle;
	bool bSeesPlayer = false;
	FVector LastSeenLocation = FVector::ZeroVector;
	FVector LastHeardLocation = FVector::ZeroVector;
	float LastHeardTime = -100.f;
	int32 HeardCount = 0;
	FVector HomeLocation = FVector::ZeroVector;
	float HomeYaw = 90.f;

	float SightRadius = 1500.f;
	float SightHalfAngle = 45.f;
	float HearingRange = 2000.f;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	float StateTimer = 0.f;
};

/**
 * 0705 AI Perception (Sight·Hearing)
 * 적은 +Y 쪽을 경계한다. 플레이어가 걸으면 0.45초마다 MakeNoise(발소리), N 키는 큰 소리.
 * 소리가 들리면 그쪽으로 조사하러 가고, 시야 부채꼴(1500, 90°)에 들어오면 추격한다. 벽 뒤는 보이지 않는다.
 */
UCLASS()
class GLOSSARYAI_API ADemo0705_AIPerception : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0705_AIPerception();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 시야 반경 (AISenseConfig_Sight::SightRadius) */
	UPROPERTY(EditAnywhere, Category = "AI Perception")
	float SightRadius = 1500.f;

	/** 시야각 전체 90° → PeripheralVisionAngleDegrees 는 반각 45° */
	UPROPERTY(EditAnywhere, Category = "AI Perception")
	float SightAngle = 90.f;

	UPROPERTY(EditAnywhere, Category = "AI Perception")
	float HearingRange = 2000.f;

	/** 발소리 간격과 크기 (Loudness × HearingRange 안에서만 들린다) */
	UPROPERTY(EditAnywhere, Category = "AI Perception")
	float FootstepInterval = 0.45f;

	UPROPERTY(EditAnywhere, Category = "AI Perception")
	float FootstepLoudness = 0.5f;

	UPROPERTY(Transient) TObjectPtr<AD07Enemy> Enemy;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> OverviewCamera;

	float FootstepTimer = 0.f;
	int32 NoiseCount = 0;

	struct FNoiseRing { FVector Location; float Radius; float Age; };
	TArray<FNoiseRing> Rings;
	void EmitNoise(float Loudness);
};
