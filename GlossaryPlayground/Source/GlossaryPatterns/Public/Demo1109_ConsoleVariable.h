#pragma once

#include "CoreMinimal.h"
#include "GlossaryCharacter.h"
#include "GlossaryDemoBase.h"
#include "D11_SidePanel.h"
#include "Demo1109_ConsoleVariable.generated.h"

class AGlossaryLabel;

/**
 * Exec 함수를 가진 플레이어 캐릭터.
 * 콘솔(~)에 "GodMode" 를 치면 엔진이 PlayerInput → PlayerController → Pawn → HUD → GameMode → CheatManager … 순서로
 * 같은 이름의 UFUNCTION(Exec) 를 찾는다. 그래서 Exec 는 이 목록에 있는 클래스에만 둘 수 있다(일반 액터에 두면 무시됨).
 * 실전에서는 UCheatManager 서브클래스에 모아 두고 PlayerController 의 CheatClass 로 지정하는 것이 정석이다
 * (CheatManager 는 Shipping 빌드에서 자동으로 만들어지지 않아 치트가 빠진다). 이 데모는 공통 PlayerController 를
 * 건드리지 않기 위해 Pawn 에 두었다.
 */
UCLASS()
class GLOSSARYPATTERNS_API AD1109_CheatCharacter : public AGlossaryCharacter
{
	GENERATED_BODY()

public:
	AD1109_CheatCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 콘솔: GodMode — 무적 켜기/끄기 */
	UFUNCTION(Exec)
	void GodMode();

	/** 콘솔: SetHealth 50 — 인자도 받을 수 있다(문자열을 자동 변환) */
	UFUNCTION(Exec)
	void SetHealth(float NewHealth);

	void ReceiveDamage(float Amount);

	bool bGodMode = false;
	float Health = 100.f;
	int32 BlockedHits = 0;
};

/**
 * 1109 Console Variable / Exec 명령
 *  - TAutoConsoleVariable 로 game.PlayerSpeed 를 만들고 매 프레임 읽어 캐릭터 속도에 반영
 *  - UFUNCTION(Exec) GodMode 로 무적 치트
 *  - FAutoConsoleCommand 로 game.Heal 명령 등록(Exec 와 달리 어느 클래스에 있든 전역 명령)
 * 1/2/3/G/H 키는 실제로 PC->ConsoleCommand("…") 를 실행한다 — ~ 키로 콘솔을 열고 직접 쳐도 똑같다.
 */
UCLASS()
class GLOSSARYPATTERNS_API ADemo1109_ConsoleVariable : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1109_ConsoleVariable();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 용암 장판이 주는 초당 피해 */
	UPROPERTY(EditAnywhere, Category = "Console Demo", meta = (ClampMin = 0))
	float HazardDamage = 20.f;

private:
	void RunConsole(const FString& Command);

	UPROPERTY(Transient) TObjectPtr<UD11_SidePanel> Panel;
	UPROPERTY(Transient) TObjectPtr<AGlossaryLabel> HealthLabel;
	FD11_LogLines Log{6};
	float LastAppliedSpeed = -1.f;
	float HazardTimer = 0.f;
};
