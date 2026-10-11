#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "RenderingDemoKit.h"
#include "Demo1009_ReflectionCapture.generated.h"

class ACameraActor;
class UPostProcessComponent;
class UReflectionCaptureComponent;

/**
 * 1009 Reflection Capture
 * Lumen 을 끈(모바일 같은) 설정에서 주황 방·파란 방에 Sphere Reflection Capture 를 하나씩 둔다(BuildLevel).
 * 캡처가 있으면 크롬 구와 광택 바닥이 각 방의 색을 비추고, 끄면 하늘빛(Sky Light)만 비쳐 두 방이 똑같이 보인다.
 */
UCLASS()
class GLOSSARYRENDERING_API ADemo1009_ReflectionCapture : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1009_ReflectionCapture();

#if WITH_EDITOR
	virtual void BuildLevel(UWorld* World) override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Reflection Capture") bool bCapturesEnabled = true;
	/** 비교용: Lumen GI·반사를 켠다 (캡처 없이도 반사가 나오는 데스크톱 방식) */
	UPROPERTY(EditAnywhere, Category = "Reflection Capture") bool bLumen = false;

	UPROPERTY(Transient) TArray<TObjectPtr<UReflectionCaptureComponent>> Captures;
	UPROPERTY(Transient) TObjectPtr<UPostProcessComponent> PostProcess;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;

private:
	void Apply();
	void BuildRoom(float Y, const FLinearColor& WallColor, const FLinearColor& LightColor);
	int32 RefreshFrames = 0;
	RenderKit::FFrameStats Stats;
};
