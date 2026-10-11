#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "MVVMViewModelBase.h"
#include "Demo0617_MVVM.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * 체력 뷰모델: UI 가 알아야 할 값만 FieldNotify 로 노출한다.
 * 값이 실제로 바뀔 때만 알림을 보내고(UE_MVVM_SET_PROPERTY_VALUE), 계산 값(GetHealthPercent)도 함께 알린다.
 */
UCLASS(BlueprintType)
class GLOSSARYUI_API UDemo0617_HealthViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	float GetHealth() const { return Health; }
	float GetMaxHealth() const { return MaxHealth; }
	void SetHealth(float NewHealth);
	void SetMaxHealth(float NewMax);

	/** 파생 값도 FieldNotify 함수로 만들면 바인딩할 수 있다 (Health·MaxHealth 가 바뀔 때 같이 알림) */
	UFUNCTION(BlueprintPure, FieldNotify, Category = "Health")
	float GetHealthPercent() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }

	/** 디버그용: 지금까지 보낸 알림 수 */
	int32 BroadcastCount = 0;

protected:
	virtual void BroadcastFieldValueChanged(UE::FieldNotification::FFieldId InFieldId) override;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, Category = "Health", meta = (AllowPrivateAccess))
	float Health = 100.f;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, Category = "Health", meta = (AllowPrivateAccess))
	float MaxHealth = 100.f;
};

/** View 1: 큰 체력바. 뷰모델의 Health · MaxHealth · GetHealthPercent 알림을 구독 */
UCLASS()
class GLOSSARYUI_API UDemo0617_HealthBarView : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	void SetViewModel(UDemo0617_HealthViewModel* InViewModel);
	int32 UpdateCount = 0;

protected:
	virtual void BuildTree() override;
	virtual void NativeDestruct() override;
	/** 필드 값이 바뀌었다는 알림 (어느 필드인지 FieldId 로 온다) */
	void OnFieldChanged(UObject* ViewModelObject, UE::FieldNotification::FFieldId FieldId);
	void Refresh();

	UPROPERTY(Transient) TObjectPtr<UDemo0617_HealthViewModel> ViewModel;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> Bar;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Label;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Counter;
};

/** View 2: 같은 뷰모델을 보는 작은 상태 배지 (뷰모델 하나 → 뷰 여러 개) */
UCLASS()
class GLOSSARYUI_API UDemo0617_StatusBadgeView : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	void SetViewModel(UDemo0617_HealthViewModel* InViewModel);
	int32 UpdateCount = 0;

protected:
	virtual void BuildTree() override;
	virtual void NativeDestruct() override;
	void OnPercentChanged(UObject* ViewModelObject, UE::FieldNotification::FFieldId FieldId);

	UPROPERTY(Transient) TObjectPtr<UDemo0617_HealthViewModel> ViewModel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Badge;
};

/**
 * 0617 MVVM (Viewmodel)
 * Health · MaxHealth 뷰모델을 만들고 체력바에 바인딩한다. 게임 코드는 뷰모델 값만 바꾸고, UI 는 알림으로 알아서 갱신된다.
 */
UCLASS()
class GLOSSARYUI_API ADemo0617_MVVM : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0617_MVVM();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "MVVM") float AutoDamageInterval = 2.f;

	UPROPERTY(Transient) TObjectPtr<UDemo0617_HealthViewModel> HealthViewModel;
	UPROPERTY(Transient) TObjectPtr<UDemo0617_HealthBarView> BarView;
	UPROPERTY(Transient) TObjectPtr<UDemo0617_StatusBadgeView> BadgeView;

private:
	float Timer = 0.f;
	FString LastAction = TEXT("-");
};
