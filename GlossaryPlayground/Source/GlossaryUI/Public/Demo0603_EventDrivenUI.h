#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "Demo0603_EventDrivenUI.generated.h"

class UProgressBar;
class UTextBlock;
class ADemo0603_EventDrivenUI;

/** 체력이 바뀔 때만 방송하는 이벤트 (Delegate 방식의 핵심) */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDemo0603HealthChanged, float /*Health*/, float /*MaxHealth*/);

/**
 * 왼쪽: Property Binding (UMG 'Bind' 버튼 = TextDelegate / PercentDelegate) → Slate 가 그릴 때마다 함수를 부른다.
 * 오른쪽: Delegate 로 체력이 바뀔 때만 SetPercent / SetText.
 */
UCLASS()
class GLOSSARYUI_API UDemo0603_CompareHud : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	void Init(ADemo0603_EventDrivenUI* InSource);

	int64 BindingCalls = 0;
	int64 DelegateUpdates = 0;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> LeftCount;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> RightCount;

protected:
	virtual void BuildTree() override;
	virtual void NativeDestruct() override;

	/** WBP 에서 Text 옆 'Bind' → Create Binding 으로 만드는 함수와 같은 것. 매 프레임 불린다 */
	UFUNCTION() FText GetBoundHealthText();
	UFUNCTION() float GetBoundHealthPercent();

	/** Delegate 방식: 체력이 바뀌었을 때 한 번만 */
	void HandleHealthChanged(float Health, float MaxHealth);

	UPROPERTY(Transient) TObjectPtr<UProgressBar> BoundBar;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BoundText;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> EventBar;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> EventText;

	TWeakObjectPtr<ADemo0603_EventDrivenUI> Source;
	FDelegateHandle HealthChangedHandle;
};

/**
 * 0603 이벤트 갱신 vs Property Binding
 * 같은 체력 값을 '매 프레임 Bind 함수' 와 '바뀔 때만 Delegate' 로 보여 주고 호출 횟수를 센다.
 */
UCLASS()
class GLOSSARYUI_API ADemo0603_EventDrivenUI : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0603_EventDrivenUI();

	float GetHealth() const { return Health; }
	float GetMaxHealth() const { return MaxHealth; }

	/** UI 는 이 이벤트만 구독하면 된다 (UI 가 매 프레임 체력을 물어볼 필요가 없다) */
	FDemo0603HealthChanged OnHealthChanged;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SetHealth(float NewHealth);

	UPROPERTY(EditAnywhere, Category = "Event Driven UI")
	float MaxHealth = 100.f;

	/** 이 간격마다 자동으로 피해를 입는다 (체력 변화가 드물다는 것을 보여 주기 위해) */
	UPROPERTY(EditAnywhere, Category = "Event Driven UI")
	float AutoDamageInterval = 2.f;

	UPROPERTY(Transient)
	TObjectPtr<UDemo0603_CompareHud> Hud;

private:
	float Health = 100.f;
	float DamageTimer = 0.f;
	float Elapsed = 0.f;
};
