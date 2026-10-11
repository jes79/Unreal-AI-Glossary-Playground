#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GlossaryDemoBase.h"
#include "D11_SidePanel.h"
#include "Demo1101_UPropertyUFunction.generated.h"

class UStaticMeshComponent;

/**
 * 1101 예제의 주인공: 요청 예문 그대로 Speed 는 EditAnywhere·BlueprintReadWrite(Category=Movement, ClampMin 0),
 * Fire 는 BlueprintCallable 로 노출한 대포. 지정자(specifier)마다 무엇이 달라지는지 주석으로 적어 두었다.
 *
 *  ─ UPROPERTY 지정자 ─────────────────────────────────────────────
 *   EditAnywhere        디테일 패널에서 클래스 기본값(BP)과 레벨에 놓인 인스턴스 둘 다 수정
 *   EditDefaultsOnly    클래스 기본값(블루프린트 에디터)에서만 수정, 레벨 인스턴스는 못 바꿈
 *   EditInstanceOnly    레벨에 놓인 인스턴스에서만 수정
 *   VisibleAnywhere     보이기만(회색), 수정 불가 — 컴포넌트 포인터·디버그 값에 사용
 *   BlueprintReadWrite  블루프린트 그래프에 Get/Set 노드가 생김
 *   BlueprintReadOnly   Get 노드만 생김
 *   Category = "A|B"    디테일 패널·BP 메뉴의 그룹(| 로 하위 그룹)
 *   meta=(ClampMin/ClampMax)  디테일 패널·BP 핀에 넣는 값의 한계 (C++ 대입은 막지 않는다!)
 *   meta=(UIMin/UIMax)        슬라이더 드래그 범위만(직접 타이핑은 넘을 수 있음)
 *   meta=(EditCondition="bFlag")  bFlag 가 true 일 때만 편집 가능
 *   Transient           저장하지 않는 값(실행 중 계산값)
 *
 *  ─ UFUNCTION 지정자 ─────────────────────────────────────────────
 *   BlueprintCallable   실행 핀이 있는 BP 노드로 호출 가능
 *   BlueprintPure       실행 핀 없는 '값' 노드 (const 함수에 사용, 부작용 없어야 함)
 *   BlueprintNativeEvent  C++ 기본 구현(이름_Implementation) + BP 에서 덮어쓰기 가능
 *   BlueprintImplementableEvent  구현은 BP 만. C++ 은 호출만 한다
 *   CallInEditor        디테일 패널에 버튼으로 나타난다(에디터에서 실행)
 */
UCLASS()
class GLOSSARYPATTERNS_API AD1101_Cannon : public AActor
{
	GENERATED_BODY()

public:
	AD1101_Cannon();

	/** 좌우 이동 속도 (cm/s). 요청 예문: EditAnywhere · BlueprintReadWrite · Category=Movement · ClampMin 0 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (ClampMin = 0, ClampMax = 2000, UIMin = 0, UIMax = 1000, Units = "cm/s"))
	float Speed = 300.f;

	/** 이동 폭 (cm). 레벨 인스턴스마다 다르게 두는 값이라 EditInstanceOnly */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Movement, meta = (ClampMin = 100))
	float TravelWidth = 900.f;

	/** 한 발의 피해량. 밸런스 값이라 클래스 기본값에서만 수정(EditDefaultsOnly) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Combat, meta = (ClampMin = 1))
	int32 Damage = 10;

	/** 자동 발사 켜기. 아래 FireInterval 의 EditCondition 으로 쓰인다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat)
	bool bAutoFire = true;

	/** 자동 발사 간격 (초). bAutoFire 가 꺼져 있으면 디테일 패널에서 회색 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (EditCondition = "bAutoFire", ClampMin = 0.05))
	float FireInterval = 0.5f;

	/** 지금까지 쏜 수. 보기만 가능(VisibleAnywhere) + BP 에서 읽기만(BlueprintReadOnly) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat)
	int32 ShotsFired = 0;

	/** 실행 중 계산값 — 저장하지 않는다(Transient) */
	UPROPERTY(Transient, VisibleInstanceOnly, Category = Combat)
	float Heat = 0.f;

	/** 한 발 쏜다. BP 에서 실행 핀이 있는 노드로 호출 가능 */
	UFUNCTION(BlueprintCallable, Category = Combat)
	void Fire();

	/** 분당 발사 수. 값만 돌려주는 순수 함수 → BP 에서 실행 핀 없는 초록 노드 */
	UFUNCTION(BlueprintPure, Category = Combat)
	float GetShotsPerMinute() const;

	/** 발사 직후 반응. C++ 기본 구현은 OnFired_Implementation, BP 자식이 덮어쓸 수 있다 */
	UFUNCTION(BlueprintNativeEvent, Category = Combat)
	void OnFired(int32 ShotIndex);

	/** 과열 알림. 구현은 BP 에서만 — C++ 은 부르기만 한다(BP 가 없으면 아무 일도 없음) */
	UFUNCTION(BlueprintImplementableEvent, Category = Combat)
	void OnOverheated();

	/** 디테일 패널에 'Reset Counter' 버튼이 생긴다 */
	UFUNCTION(CallInEditor, Category = Combat)
	void ResetCounter();

	/** OnFired_Implementation 이 몇 번 불렸나 (화면 표시용, 리플렉션 대상 아님) */
	int32 NativeEventCalls = 0;

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = Components)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = Components)
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = Components)
	TObjectPtr<UStaticMeshComponent> Barrel;

private:
	FVector StartLocation;
	float Phase = 0.f;
	float FireTimer = 0.f;
	float FlashTimer = 0.f;
};

/**
 * 1101 UPROPERTY / UFUNCTION
 * 대포의 Speed·FireInterval 을 실행 중에 바꾸고, 오른쪽 패널에는 "리플렉션 시스템이 실제로 기록한 플래그"를
 * TFieldIterator 로 읽어서 보여 준다 — 지정자가 결국 엔진이 읽는 데이터(플래그·메타데이터)가 된다는 것을 눈으로 확인.
 */
UCLASS()
class GLOSSARYPATTERNS_API ADemo1101_UPropertyUFunction : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1101_UPropertyUFunction();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 1/2 키로 Speed 를 바꾸는 단위 */
	UPROPERTY(EditAnywhere, Category = "UPROPERTY Demo", meta = (ClampMin = 10))
	float SpeedStep = 150.f;

private:
	/** 블루프린트의 'Set Speed' 노드와 같은 경로: FProperty 를 찾아 값을 쓴다. ClampMin 메타데이터를 직접 적용 */
	void SetSpeedLikeDetailsPanel(float NewValue);
	/** 블루프린트의 'Fire' 노드와 같은 경로: UFunction 을 찾아 ProcessEvent */
	void CallFireLikeBlueprint();
	FString DescribeReflection() const;

	UPROPERTY(Transient) TObjectPtr<AD1101_Cannon> Cannon;
	UPROPERTY(Transient) TObjectPtr<UD11_SidePanel> Panel;
	FD11_LogLines Log{5};
};
