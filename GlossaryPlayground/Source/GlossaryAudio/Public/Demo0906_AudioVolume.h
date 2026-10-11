#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0906_AudioVolume.generated.h"

class AAudioVolume;
class UGlossaryAudioPanel;
class USoundWave;

/**
 * 0906 Audio Volume (공간 리버브)
 * 동굴 안에 Audio Volume(브러시 볼륨, 빌더가 레벨에 저장)을 두고 Reverb Settings 에 RVB_Cave(Decay 4초)를 넣는다.
 * 플레이어가 1.2초마다 손뼉을 치고, 리스너가 볼륨 안에 있으면 리버브가 걸린다.
 * 화면: 볼륨 경계(하늘색 선), 손뼉마다 퍼지는 메아리 링(안: 여러 겹·오래 / 밖: 한 겹·짧게), 상태 줄의 현재 Audio Volume.
 */
UCLASS()
class GLOSSARYAUDIO_API ADemo0906_AudioVolume : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0906_AudioVolume();

#if WITH_EDITOR
	virtual void BuildLevel(UWorld* World) override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 동굴(= Audio Volume) 중심과 반 크기 */
	UPROPERTY(EditAnywhere, Category = "Audio Volume")
	FVector CaveCenter = FVector(1300.f, 0.f, 250.f);

	UPROPERTY(EditAnywhere, Category = "Audio Volume")
	FVector CaveExtent = FVector(700.f, 450.f, 250.f);

	/** 자동 손뼉 간격(초) */
	UPROPERTY(EditAnywhere, Category = "Audio Volume")
	float ClapInterval = 1.2f;

private:
	void Clap();
	void SpawnEcho(const FVector& Location, float Delay, float Lifetime, const FLinearColor& Color, float MaxRadius);

	struct FEcho { TWeakObjectPtr<AActor> Ring; float Delay = 0.f; float Age = 0.f; float Lifetime = 1.f; float MaxRadius = 600.f; };

	UPROPERTY(Transient) TObjectPtr<AAudioVolume> CaveVolume;
	UPROPERTY(Transient) TObjectPtr<USoundWave> ClapSound;
	UPROPERTY(Transient) TObjectPtr<UGlossaryAudioPanel> Panel;

	TArray<FEcho> Echoes;
	float ClapTimer = 0.f;
	bool bAutoClap = true;
	bool bInsideLastClap = false;
	int32 ClapCount = 0;
};
