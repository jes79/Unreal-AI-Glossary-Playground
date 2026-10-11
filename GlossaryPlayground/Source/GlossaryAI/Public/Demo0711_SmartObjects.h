#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "SmartObjectDefinition.h"
#include "SmartObjectRuntime.h"
#include "SmartObjectTypes.h"
#include "Demo0711_SmartObjects.generated.h"

class ACameraActor;
class AD07Enemy;
class AStaticMeshActor;
class USmartObjectComponent;

/** 벤치 슬롯이 NPC 에게 알려 주는 "어떻게 쓰나" 정보: 앉아 있는 시간 */
UCLASS(EditInlineNew, meta = (DisplayName = "Sit (Glossary 0711)"))
class GLOSSARYAI_API UD0711_SitBehaviorDefinition : public USmartObjectBehaviorDefinition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Sit")
	float SitDuration = 6.f;
};

namespace D0711
{
	inline const TCHAR* DefinitionPath = TEXT("/Game/Glossary/07_AI/SOD_Bench");

	/** 벤치 정의: 슬롯 3개(앞쪽, 벤치 반대 방향을 봄) + 기본 행동 정의 Sit. 빌더·런타임 대체 경로 공용 */
	GLOSSARYAI_API void FillBenchDefinition(USmartObjectDefinition* Definition);
}

/**
 * 0711 Smart Objects
 * 벤치에 USmartObjectComponent(정의 SOD_Bench: 자리 3개)를 붙이고, NPC 5명이 USmartObjectSubsystem 으로
 * 빈 자리를 찾고(FindSmartObjects) → 예약(MarkSlotAsClaimed) → 걸어가서 사용(MarkSlotAsOccupied) → 일어나 반납(MarkSlotAsFree) 한다.
 * 자리가 모두 차 있으면 찾기 결과가 비어 있어서 기다리며 돌아다닌다.
 */
UCLASS()
class GLOSSARYAI_API ADemo0711_SmartObjects : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0711_SmartObjects();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Smart Objects")
	int32 NumNPCs = 5;

	UPROPERTY(Transient) TObjectPtr<USmartObjectDefinition> BenchDefinition;
	UPROPERTY(Transient) TObjectPtr<AStaticMeshActor> Bench;
	UPROPERTY(Transient) TObjectPtr<USmartObjectComponent> BenchSmartObject;
	UPROPERTY(Transient) TArray<TObjectPtr<AD07Enemy>> NPCs;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> OverviewCamera;

	enum class ENpcState : uint8 { Wander, GoingToSlot, Sitting };
	struct FNpcBrain
	{
		ENpcState State = ENpcState::Wander;
		float Timer = 0.f;
		FSmartObjectClaimHandle Claim;
		FString Note;
	};
	TArray<FNpcBrain> Brains;
	TArray<FSmartObjectSlotHandle> CachedSlots;
	bool bDefinitionFromAsset = false;
	int32 FailedSearches = 0;
	int32 Uses = 0;

	void TickNpc(int32 Index, float DeltaSeconds);
	void Wander(AD07Enemy* Npc);
	void SetSeated(AD07Enemy* Npc, bool bSeated, const FTransform& SlotTransform);
};
