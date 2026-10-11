#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GlossaryDemoBase.h"
#include "Demo0814_Checkpoint.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class ADemo0814_Checkpoint;
class UD08_SidePanel;

/** 체크포인트 깃발: Box Trigger 에 플레이어가 닿으면 데모에 알린다 */
UCLASS()
class GLOSSARYSYSTEMS_API AD0814_CheckpointFlag : public AActor
{
	GENERATED_BODY()

public:
	AD0814_CheckpointFlag();

	int32 Index = 0;
	TWeakObjectPtr<ADemo0814_Checkpoint> Demo;
	void SetActive(bool bActive);

protected:
	virtual void BeginPlay() override;
	UFUNCTION() void OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UPROPERTY(VisibleAnywhere, Category = "Checkpoint") TObjectPtr<UBoxComponent> Trigger;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Flag;
};

/** 용암: 닿으면 죽는다 */
UCLASS()
class GLOSSARYSYSTEMS_API AD0814_KillZone : public AActor
{
	GENERATED_BODY()

public:
	AD0814_KillZone();
	TWeakObjectPtr<ADemo0814_Checkpoint> Demo;
	void SetSize(const FVector& Size);

protected:
	virtual void BeginPlay() override;
	UFUNCTION() void OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UPROPERTY(VisibleAnywhere, Category = "Checkpoint") TObjectPtr<UBoxComponent> Trigger;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Lava;
};

/**
 * 0814 체크포인트 / 리스폰
 * 깃발 Trigger 를 지나면 위치를 저장하고, 용암에 빠지면(또는 K) 폰을 없앤 뒤 GameMode::RestartPlayerAtTransform 으로 거기서 부활시킨다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API ADemo0814_Checkpoint : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0814_Checkpoint();

	void ReachCheckpoint(AD0814_CheckpointFlag* Flag);
	void KillPlayer(const FString& Reason);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 죽은 뒤 부활까지 */
	UPROPERTY(EditAnywhere, Category = "Checkpoint") float RespawnDelay = 1.5f;

private:
	void Respawn();

	UPROPERTY(Transient) TArray<TObjectPtr<AD0814_CheckpointFlag>> Flags;
	UPROPERTY(Transient) TObjectPtr<UD08_SidePanel> Panel;
	/** 마지막 체크포인트 위치 — 처음에는 PlayerStart */
	FTransform SavedTransform;
	int32 SavedIndex = -1;
	int32 Deaths = 0;
	float RespawnTimer = 0.f;
	FString LastEvent;
};
