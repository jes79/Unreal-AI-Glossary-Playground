#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0902_MetaSounds.generated.h"

class ACameraActor;
class UAudioComponent;
class UGlossaryAudioPanel;
class UGlossaryAudioProbe;
class UStaticMeshComponent;

/**
 * 0902 MetaSounds
 * MetaSound Source "RPM(0~1) → Map Range 40~220Hz → Saw 오실레이터" 를 MetaSound 빌더 API 로 만든다
 * (에셋 빌더가 MS_EngineRPM 에셋으로 저장, 없으면 실행 중 같은 빌더로 임시 MetaSound 를 만든다).
 * 데모는 매 프레임 SetFloatParameter("RPM", x) 로 피치를 올리고 내린다. 타코미터 바늘·피스톤 속도로 RPM 을 보여 준다.
 */
UCLASS()
class GLOSSARYAUDIO_API ADemo0902_MetaSounds : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0902_MetaSounds();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** MetaSound 그래프 입력 이름 */
	UPROPERTY(EditAnywhere, Category = "MetaSounds")
	FName RPMParameterName = TEXT("RPM");

	/** 스로틀을 밟았을 때/뗐을 때 RPM 변화 속도(초당) */
	UPROPERTY(EditAnywhere, Category = "MetaSounds")
	float ThrottleRate = 0.6f;

	/** 그래프의 Map Range 출력 범위 (표시용, 그래프와 같은 값) */
	UPROPERTY(VisibleAnywhere, Category = "MetaSounds")
	FVector2D FrequencyRange = FVector2D(40.f, 220.f);

private:
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Engine;
	UPROPERTY(Transient) TObjectPtr<UGlossaryAudioProbe> Probe;
	UPROPERTY(Transient) TObjectPtr<UGlossaryAudioPanel> Panel;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Needle;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> NeedlePivot;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Pistons;

	FString SourceDescription;
	bool bUsingMetaSound = false;
	bool bAutoSweep = true;
	float RPM = 0.f;
	float SweepTime = 0.f;
	float CrankAngle = 0.f;
};
