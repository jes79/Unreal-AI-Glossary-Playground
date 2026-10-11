#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "Demo0606_WidgetComponent.generated.h"

class UProgressBar;
class UTextBlock;
class UWidgetComponent;
class UStaticMeshComponent;

/** 적 머리 위에 붙는 작은 체력바 위젯 */
UCLASS()
class GLOSSARYUI_API UDemo0606_EnemyBar : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	void SetValues(float Percent, float DistanceMeters);

protected:
	virtual void BuildTree() override;

	UPROPERTY(Transient) TObjectPtr<UProgressBar> Bar;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Info;
};

/** 0606 용 적: 몸통 + WidgetComponent(Screen) 체력바. 앞뒤로 걸어 다니며 거리가 바뀐다 */
UCLASS()
class GLOSSARYUI_API ADemo0606_Enemy : public AActor
{
	GENERATED_BODY()

public:
	ADemo0606_Enemy();
	virtual void Tick(float DeltaSeconds) override;

	/** 이 거리(cm)보다 멀면 체력바를 숨긴다 */
	float HideDistance = 1500.f;
	bool bHideByDistance = true;
	float Phase = 0.f;
	float Health = 1.f;
	bool bBarVisible = true;
	float LastDistance = 0.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
	/** 핵심 컴포넌트: Space = Screen 이면 항상 화면을 향하고 거리와 무관하게 같은 픽셀 크기 */
	UPROPERTY(VisibleAnywhere) TObjectPtr<UWidgetComponent> HealthBarWidget;

private:
	FVector Origin;
	float Time = 0.f;
};

/**
 * 0606 Widget Component (월드 UI)
 * 적 머리 위에 WidgetComponent(Space: Screen) 로 체력바를 붙이고, 멀어지면 숨긴다.
 */
UCLASS()
class GLOSSARYUI_API ADemo0606_WidgetComponent : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0606_WidgetComponent();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 이 거리(cm)보다 먼 적의 체력바는 숨긴다 */
	UPROPERTY(EditAnywhere, Category = "Widget Component", meta = (ClampMin = 0)) float HideDistance = 1500.f;

	UPROPERTY(Transient) TArray<TObjectPtr<ADemo0606_Enemy>> Enemies;

private:
	bool bHideByDistance = true;
};
