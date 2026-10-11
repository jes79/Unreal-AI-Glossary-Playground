#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "D11_SidePanel.h"
#include "Demo1104_Delegate.generated.h"

class AGlossaryLabel;
class UStaticMeshComponent;
class USoundBase;
class UTextBlock;

/**
 * 방송 채널의 '모양' 선언: 인자 하나(AActor* DeadActor)를 받는 다중(Multicast) + 동적(Dynamic) 델리게이트.
 *  - Multicast: 여러 구독자가 동시에 들을 수 있다
 *  - Dynamic  : UFUNCTION 을 이름으로 묶는다(AddDynamic) → 블루프린트에서도 바인딩 가능, 직렬화 가능
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FD1104_OnDied, AActor*, DeadActor);

/** 방송하는 쪽. UI·사운드·점수가 있는지조차 모른다. */
UCLASS()
class GLOSSARYPATTERNS_API AD1104_Enemy : public AActor
{
	GENERATED_BODY()

public:
	AD1104_Enemy();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Health, meta = (ClampMin = 1))
	float MaxHealth = 100.f;

	/** BlueprintAssignable: BP 의 'Bind Event to OnDied' 노드로도 구독할 수 있다 (= Event Dispatcher) */
	UPROPERTY(BlueprintAssignable, Category = Health)
	FD1104_OnDied OnDied;

	UFUNCTION(BlueprintCallable, Category = Health)
	void TakeHit(float Damage);

	float GetHealth() const { return Health; }
	bool IsAlive() const { return Health > 0.f; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = Components)
	TObjectPtr<UStaticMeshComponent> Body;

	float Health = 0.f;
	float RespawnTimer = 0.f;
};

/** 듣는 쪽 1: UI — 화면 아래 "적 처치!" 배너 */
UCLASS()
class GLOSSARYPATTERNS_API UD1104_KillBanner : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleDied(AActor* DeadActor);

	int32 Reactions = 0;

	/** 배너를 서서히 사라지게 한다(데모 Tick 에서 호출) */
	void Advance(float DeltaSeconds);

protected:
	virtual void BuildTree() override;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> Banner;
	float ShowTimer = 0.f;
};

/** 듣는 쪽 2: 사운드 — 생성 사운드 S_Died 재생 + 스피커가 울리는 연출 */
UCLASS()
class GLOSSARYPATTERNS_API AD1104_SoundPlayer : public AActor
{
	GENERATED_BODY()

public:
	AD1104_SoundPlayer();

	UFUNCTION()
	void HandleDied(AActor* DeadActor);

	UPROPERTY(EditAnywhere, Category = Sound)
	TObjectPtr<USoundBase> DiedSound;

	int32 Reactions = 0;

protected:
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = Components)
	TObjectPtr<UStaticMeshComponent> Speaker;

	float Ring = 0.f;
};

/** 듣는 쪽 3: 점수판 — 처치당 +100 */
UCLASS()
class GLOSSARYPATTERNS_API AD1104_ScoreBoard : public AActor
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleDied(AActor* DeadActor);

	int32 Score = 0;
	int32 Reactions = 0;

	UPROPERTY(Transient) TObjectPtr<AGlossaryLabel> Label;
};

/**
 * 1104 Event Dispatcher / Delegate
 * 적의 체력이 0 이 되면 OnDied.Broadcast(this) 한 줄만 실행한다. UI·사운드·점수는 각자 AddDynamic 으로 구독해서 반응.
 * 2·3·4 로 구독을 끊었다 붙여도 적 코드는 한 글자도 바뀌지 않는다(느슨한 결합).
 */
UCLASS()
class GLOSSARYPATTERNS_API ADemo1104_Delegate : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1104_Delegate();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Delegate Demo", meta = (ClampMin = 1))
	float HitDamage = 35.f;

private:
	/** 데모 자신도 구독자 하나 — 로그에 방송 사실을 남긴다 */
	UFUNCTION()
	void HandleDiedForLog(AActor* DeadActor);

	void ToggleListener(int32 Index);

	UPROPERTY(Transient) TObjectPtr<AD1104_Enemy> Enemy;
	UPROPERTY(Transient) TObjectPtr<UD1104_KillBanner> BannerUI;
	UPROPERTY(Transient) TObjectPtr<AD1104_SoundPlayer> SoundPlayer;
	UPROPERTY(Transient) TObjectPtr<AD1104_ScoreBoard> ScoreBoard;
	UPROPERTY(Transient) TObjectPtr<AGlossaryLabel> EnemyLabel;
	UPROPERTY(Transient) TObjectPtr<UD11_SidePanel> Panel;

	bool bSubscribed[3] = { true, true, true };
	int32 BroadcastCount = 0;
	FD11_LogLines Log{6};
};
