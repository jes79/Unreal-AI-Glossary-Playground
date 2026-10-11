#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "RenderingDemoKit.h"
#include "Demo1003_Nanite.generated.h"

class ACameraActor;
class UStaticMeshComponent;
class UStaticMesh;

/**
 * 1003 Nanite
 * 빌더가 만든 고폴리 바위(약 33만 삼각형, Nanite 켬) 20개 = 650만 삼각형을 깔고
 * r.Nanite.Visualize 로 Triangles / Clusters 를 본다. 같은 바위의 Nanite 끈 버전과 프레임 시간을 비교한다.
 */
UCLASS()
class GLOSSARYRENDERING_API ADemo1003_Nanite : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1003_Nanite();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** 바위 개수 (가로 × 세로) */
	UPROPERTY(EditAnywhere, Category = "Nanite")
	FIntPoint RockGrid = FIntPoint(5, 4);

	/** 현재 Nanite Visualization 모드 ("" = 끔, "Triangles", "Clusters") */
	UPROPERTY(VisibleAnywhere, Category = "Nanite")
	FString VisualizeMode;

	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Rocks;
	UPROPERTY(Transient) TObjectPtr<UStaticMesh> NaniteMesh;
	UPROPERTY(Transient) TObjectPtr<UStaticMesh> NoNaniteMesh;

private:
	void SetVisualize(const FString& Mode);
	void SetUseNaniteMesh(bool bNanite);
	bool bUsingNaniteMesh = true;
	RenderKit::FCVarScope CVars;
	RenderKit::FFrameStats Stats;
};
