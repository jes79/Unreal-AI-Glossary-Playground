#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0710_StateTree.generated.h"

class ACameraActor;
class AD07Enemy;
class UD07SidePanel;
class UStateTree;

/**
 * 0710 StateTree
 * 에셋 빌더가 UStateTreeEditorData(상태·태스크·전환·조건)를 코드로 채우고 컴파일해 ST_Enemy 를 만든다.
 *  Root ─ Idle(2초) ─완료→ Patrol(지점 이동) ─완료→ Idle …
 *         Idle/Patrol ─[플레이어 보임]→ Chase ─[200 이내]→ Attack ─[250 밖]→ Chase ─[2초간 못 봄]→ Idle
 * AI 컨트롤러의 UStateTreeAIComponent 가 실행하고, 오른쪽 패널에 활성 상태를 표시한다.
 */
UCLASS()
class GLOSSARYAI_API ADemo0710_StateTree : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0710_StateTree();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "StateTree")
	float AttackRange = 200.f;

	UPROPERTY(Transient) TObjectPtr<UStateTree> EnemyStateTree;
	UPROPERTY(Transient) TObjectPtr<AD07Enemy> Enemy;
	UPROPERTY(Transient) TObjectPtr<UD07SidePanel> SidePanel;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> OverviewCamera;

	FString LastState;
	float StateTime = 0.f;
	int32 TransitionCount = 0;
};
