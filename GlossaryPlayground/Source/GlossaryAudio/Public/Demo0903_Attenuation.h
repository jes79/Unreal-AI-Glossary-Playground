#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0903_Attenuation.generated.h"

class UAudioComponent;
class UGlossaryAudioPanel;
class UGlossaryAudioProbe;
class USoundAttenuation;
class UStaticMeshComponent;

/**
 * 0903 Attenuation (3D 거리 감쇠)
 * 모닥불 루프 소리에 ATT_Campfire(Inner Radius 200, Falloff 1500, Spatialization) 를 넣는다.
 * 바닥 링 2개(200 / 1700cm)로 감쇠 구간을 그리고, 걸어 다니면 거리·계산 볼륨·측정 Envelope·좌우 방향이 바뀐다.
 */
UCLASS()
class GLOSSARYAUDIO_API ADemo0903_Attenuation : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0903_Attenuation();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Attenuation")
	FVector FireLocation = FVector(700.f, 0.f, 0.f);

	/** C 로 끄면 Attenuation 없이 2D 로 같은 크기로 들린다 */
	UPROPERTY(EditAnywhere, Category = "Attenuation")
	bool bUseAttenuation = true;

private:
	void ApplyAttenuation();

	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Fire;
	UPROPERTY(Transient) TObjectPtr<USoundAttenuation> Attenuation;
	UPROPERTY(Transient) TObjectPtr<UGlossaryAudioProbe> Probe;
	UPROPERTY(Transient) TObjectPtr<UGlossaryAudioPanel> Panel;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Flame;
	UPROPERTY(Transient) TObjectPtr<AActor> ListenerMarker;

	float FlameTime = 0.f;
};
