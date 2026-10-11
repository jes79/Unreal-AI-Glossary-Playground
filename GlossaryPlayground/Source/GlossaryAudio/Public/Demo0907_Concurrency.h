#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0907_Concurrency.generated.h"

class ACameraActor;
class UAudioComponent;
class UGlossaryAudioPanel;
class UMaterialInstanceDynamic;
class USoundConcurrency;
class USoundWave;
class UStaticMeshComponent;

/**
 * 0907 Sound Concurrency
 * 꼬리가 1.2초인 총소리를 0.1초마다 쏘면 소리가 10개 넘게 겹친다.
 * SCon_Gunshot(Max Count 4, Resolution Rule = Stop Oldest)을 넣으면 동시에 4개까지만 나고 5번째가 오면 가장 오래된 것이 꺼진다.
 * 화면: 보이스 슬롯 16칸(재생 중 = 주황 불), 한도 4 표시선, Stop Oldest 로 끊긴 수.
 */
UCLASS()
class GLOSSARYAUDIO_API ADemo0907_Concurrency : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0907_Concurrency();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 연사 간격(초) */
	UPROPERTY(EditAnywhere, Category = "Concurrency", meta = (ClampMin = 0.03))
	float FireInterval = 0.1f;

	/** true 면 총소리에 SCon_Gunshot 을 적용 */
	UPROPERTY(EditAnywhere, Category = "Concurrency")
	bool bUseConcurrency = true;

private:
	void Fire();

	struct FShot { TWeakObjectPtr<UAudioComponent> Audio; double StartTime = 0.0; };

	UPROPERTY(Transient) TObjectPtr<USoundWave> Gunshot;
	UPROPERTY(Transient) TObjectPtr<USoundConcurrency> Concurrency;
	UPROPERTY(Transient) TObjectPtr<UGlossaryAudioPanel> Panel;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> SlotMats;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> MuzzleFlash;

	TArray<FShot> Shots;
	float FireTimer = 0.f;
	float FlashTime = 0.f;
	bool bAutoFire = true;
	int32 ShotCount = 0;
	int32 CutCount = 0;
	int32 PeakVoices = 0;
};
