#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0815_LevelStreaming.generated.h"

class UBoxComponent;
class UD08_SidePanel;

/**
 * 0815 Level Streaming
 * 빌더가 만든 서브레벨 L_0815_SubA / SubB 를 퍼시스턴트 레벨의 Streaming Levels 에 등록해 두고(BuildLevel),
 * 트리거 박스에 들어가면 LoadStreamLevel, 나가면 UnloadStreamLevel 한다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API ADemo0815_LevelStreaming : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0815_LevelStreaming();

	/** 서브레벨 패키지 경로 (빌더와 공유) */
	static const TCHAR* SubLevelA;
	static const TCHAR* SubLevelB;

#if WITH_EDITOR
	virtual void BuildLevel(UWorld* World) override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION() void OnEnterA(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);
	UFUNCTION() void OnExitA(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex);
	UFUNCTION() void OnEnterB(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);
	UFUNCTION() void OnExitB(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex);
	/** 로드/언로드 완료 콜백 (Latent Action) */
	UFUNCTION() void OnStreamingDone();

	/** 트리거 크기 */
	UPROPERTY(EditAnywhere, Category = "Level Streaming") FVector TriggerSize = FVector(700.f, 700.f, 300.f);

private:
	void Stream(const TCHAR* LevelPath, bool bLoad);
	UBoxComponent* MakeTrigger(const FVector& Location, const FLinearColor& Color, const FString& Label);
	FString DescribeLevel(const TCHAR* LevelPath) const;

	UPROPERTY(Transient) TObjectPtr<UBoxComponent> TriggerA;
	UPROPERTY(Transient) TObjectPtr<UBoxComponent> TriggerB;
	UPROPERTY(Transient) TObjectPtr<UD08_SidePanel> Panel;
	int32 NextUUID = 1;
	int32 CallbackCount = 0;
	FString LastEvent;
};
