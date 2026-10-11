#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "NavAreas/NavArea.h"
#include "Demo0707_NavModifier.generated.h"

class ACameraActor;
class AD07Enemy;
class ANavModifierVolume;

/** 늪: 지나갈 수는 있지만 1cm 당 비용이 10배 (길찾기가 웬만하면 돌아간다) */
UCLASS(meta = (DisplayName = "NavArea_Swamp (Cost 10)"))
class GLOSSARYAI_API UD0707_NavArea_Swamp : public UNavArea
{
	GENERATED_BODY()

public:
	UD0707_NavArea_Swamp();
};

/**
 * 0707 Nav Modifier / Nav Area (이동 비용)
 * BuildLevel 에서 Nav Modifier Volume 두 개를 레벨에 놓는다: 늪(AreaClass = 비용 10 인 UD0707_NavArea_Swamp),
 * 용암(AreaClass = NavArea_Null → NavMesh 에서 잘려 나감). AI 는 양 끝을 왕복하며 늪을 돌아가고, 용암 쪽으로는 아예 못 간다.
 * C 키로 늪의 AreaClass 를 NavArea_Default 로 바꾸면 늪을 그대로 가로지른다.
 */
UCLASS()
class GLOSSARYAI_API ADemo0707_NavModifier : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0707_NavModifier();

#if WITH_EDITOR
	virtual void BuildLevel(UWorld* World) override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 늪 영역 (중심, 크기 cm) */
	UPROPERTY(EditAnywhere, Category = "Nav Modifier")
	FVector SwampCenter = FVector(0.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Nav Modifier")
	FVector SwampSize = FVector(500.f, 2200.f, 300.f);

	UPROPERTY(EditAnywhere, Category = "Nav Modifier")
	FVector LavaCenter = FVector(0.f, 1700.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Nav Modifier")
	FVector LavaSize = FVector(1400.f, 1200.f, 300.f);

	UPROPERTY(Transient) TObjectPtr<ANavModifierVolume> SwampVolume;
	UPROPERTY(Transient) TObjectPtr<AD07Enemy> Agent;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> OverviewCamera;

	bool bSwampCostly = true;
	bool bGoingRight = true;
	float WaitTimer = 0.5f;
	FVector PointA = FVector(-1400.f, 200.f, 0.f);
	FVector PointB = FVector(1400.f, 200.f, 0.f);
};
