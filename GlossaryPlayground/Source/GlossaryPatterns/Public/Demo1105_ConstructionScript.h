#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GlossaryDemoBase.h"
#include "D11_SidePanel.h"
#include "Demo1105_ConstructionScript.generated.h"

class UStaticMeshComponent;

/**
 * 울타리: PostCount(기둥 개수)를 바꾸면 OnConstruction 에서 기둥 메시를 그만큼 다시 배치한다.
 * 블루프린트의 Construction Script 와 같은 자리 — 에디터에서 값을 바꾸거나 액터를 옮길 때마다 불린다.
 *
 * 포인트: 여기서 만드는 컴포넌트는 CreationMethod = UserConstructionScript 로 표시한다.
 *   → 다시 실행될 때 엔진이 이전 것을 자동으로 지운다(중복 생성 방지). 에디터는 레벨을 열 때 다시 조립하고,
 *     게임 실행은 저장된 조립 결과를 그대로 로드한다(실행 중에는 Construction Script 를 다시 돌리지 않는 것이 기본).
 */
UCLASS()
class GLOSSARYPATTERNS_API AD1105_Fence : public AActor
{
	GENERATED_BODY()

public:
	AD1105_Fence();

	/** 기둥 개수 — 이 값을 바꾸면 다시 조립된다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Fence, meta = (ClampMin = 2, ClampMax = 30))
	int32 PostCount = 8;

	/** 기둥 간격 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Fence, meta = (ClampMin = 50, ClampMax = 500))
	float Spacing = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Fence, meta = (ClampMin = 50))
	float PostHeight = 160.f;

	/** 가로대(레일)도 만들지 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Fence)
	bool bRails = true;

	/** OnConstruction 이 몇 번 불렸나 (저장 안 함) */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = Fence)
	int32 ConstructionCount = 0;

	/** 지금 붙어 있는 기둥 컴포넌트 수 (중복 생성이 없는지 확인용) */
	int32 CountPostComponents() const;

	/** 에디터 밖(패키지 게임)에서 같은 조립을 할 때 직접 부르는 함수 */
	void RebuildFence();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = Components)
	TObjectPtr<USceneComponent> Root;

private:
	UStaticMeshComponent* AddPiece(const FVector& Location, const FVector& Size, const FLinearColor& Color);
	void DestroyPieces();
};

/**
 * 1105 Construction Script (OnConstruction)
 * 레벨에 저장된 울타리(빌더가 BuildLevel 에서 놓음)를 실행 중에 [ ] 로 기둥 개수를 바꾸고
 * RerunConstructionScripts() 로 에디터가 하는 재조립을 그대로 재현한다.
 */
UCLASS()
class GLOSSARYPATTERNS_API ADemo1105_ConstructionScript : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1105_ConstructionScript();

#if WITH_EDITOR
	virtual void BuildLevel(UWorld* World) override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void Rerun();

	UPROPERTY(Transient) TObjectPtr<AD1105_Fence> Fence;
	UPROPERTY(Transient) TObjectPtr<UD11_SidePanel> Panel;
	FD11_LogLines Log{6};
};
