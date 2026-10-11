#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryGameMode.generated.h"

/** 프로젝트 기본 게임모드. 레벨의 데모 액터가 정한 PlayerPawnClass 를 스폰한다. */
UCLASS()
class GLOSSARYCORE_API AGlossaryGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGlossaryGameMode();

	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
};

/** F1 정보 패널 접기 · F2 허브로 · F3 마우스 커서 전환 */
UCLASS()
class GLOSSARYCORE_API AGlossaryPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void PlayerTick(float DeltaTime) override;
	virtual void BeginPlay() override;

	/** 마우스 커서 보이기 + 게임/UI 입력 모드 */
	UFUNCTION(BlueprintCallable, Category = Glossary)
	void SetCursorMode(bool bShowCursor);

private:
	/**
	 * 자동 스크린샷 (검증·README 용):
	 *   -GlossaryShot=C:/shots/0101.png [-GlossaryShotDelay=6] [-GlossaryShotKeys=One,Two]
	 * Delay 초 뒤 화면(UI 포함)을 저장하고 게임을 끝낸다. Keys 는 그 전에 1초 간격으로 눌렀다 뗀다.
	 */
	void TickAutoShot(float DeltaTime);

	FString ShotPath;
	TArray<FKey> ShotKeys;
	float ShotDelay = 6.f;
	float ShotTimer = 0.f;
	int32 ShotKeyIndex = 0;
	int32 ShotStage = 0; // 0 대기, 1 요청함, 2 종료 대기
};
