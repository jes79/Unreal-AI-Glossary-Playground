#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GlossaryDemoBase.h"
#include "Demo0804_SaveGame.generated.h"

class AGlossaryLabel;
class UD08_SidePanel;

/** 저장 파일에 들어갈 데이터. UPROPERTY 로 표시한 변수만 직렬화된다. */
UCLASS()
class GLOSSARYSYSTEMS_API UD0804_SaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() FVector PlayerLocation = FVector::ZeroVector;
	UPROPERTY() FRotator PlayerRotation = FRotator::ZeroRotator;
	UPROPERTY() float HP = 100.f;
	UPROPERTY() int32 Coins = 0;
	UPROPERTY() FString SavedAt;
};

/**
 * 0804 SaveGame
 * 위치·체력·코인을 USaveGame 에 담아 SaveGameToSlot 으로 파일에 쓰고, LoadGameFromSlot 으로 되돌린다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API ADemo0804_SaveGame : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0804_SaveGame();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 저장 슬롯 이름 → Saved/SaveGames/{SlotName}.sav */
	UPROPERTY(EditAnywhere, Category = "SaveGame") FString SlotName = TEXT("Glossary0804");
	UPROPERTY(EditAnywhere, Category = "SaveGame") int32 UserIndex = 0;

	UPROPERTY(VisibleAnywhere, Category = "SaveGame") float HP = 100.f;
	UPROPERTY(VisibleAnywhere, Category = "SaveGame") int32 Coins = 0;

private:
	void Save();
	void Load();
	void DeleteSave();
	void UpdateMarker(const FVector& Location, bool bVisible);

	UPROPERTY(Transient) TObjectPtr<AActor> SavedMarker;
	UPROPERTY(Transient) TObjectPtr<AGlossaryLabel> SavedLabel;
	UPROPERTY(Transient) TObjectPtr<UD08_SidePanel> Panel;
	FString LastAction;
};
