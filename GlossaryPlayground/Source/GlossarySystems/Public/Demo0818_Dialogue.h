#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "Demo0818_Dialogue.generated.h"

class UButton;
class UTextBlock;
class AGlossaryLabel;
class ADemo0818_Dialogue;

/** 대사 한 줄 + 선택지 최대 3개. NextN 은 다음 행 이름, "End" 면 대화 종료 */
USTRUCT(BlueprintType)
struct FD0818_DialogueRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue") FString Speaker;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue") FString Line;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue") FString Choice1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue") FName Next1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue") FString Choice2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue") FName Next2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue") FString Choice3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue") FName Next3;
};

/** 화면 아래 대화창: 화자 · 대사 · 선택지 버튼 3개 */
UCLASS()
class GLOSSARYSYSTEMS_API UD0818_DialogueWidget : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<ADemo0818_Dialogue> Owner;
	void Show(const FD0818_DialogueRow& Row);

protected:
	virtual void BuildTree() override;
	UFUNCTION() void OnChoice0();
	UFUNCTION() void OnChoice1();
	UFUNCTION() void OnChoice2();

	UPROPERTY(Transient) TObjectPtr<UTextBlock> SpeakerText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LineText;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> Buttons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> ButtonTexts;
};

/**
 * 0818 대화 시스템
 * DT_0818_Dialogue(대사·선택지·다음 대사 ID)를 따라가며, 고른 선택지에 따라 다음 행으로 분기한다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API ADemo0818_Dialogue : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0818_Dialogue();

	static const TCHAR* DialogueCSV;

	/** 선택지 Index(0~2)를 골랐다 */
	void Choose(int32 Index);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Dialogue") TSoftObjectPtr<UDataTable> DialogueTable;
	UPROPERTY(EditAnywhere, Category = "Dialogue") FName StartRow = TEXT("Start");
	UPROPERTY(EditAnywhere, Category = "Dialogue") float TalkDistance = 400.f;

private:
	void StartDialogue();
	void GoTo(FName RowName);
	void EndDialogue();

	UPROPERTY(Transient) TObjectPtr<UDataTable> Table;
	UPROPERTY(Transient) TObjectPtr<UD0818_DialogueWidget> Window;
	UPROPERTY(Transient) TObjectPtr<AActor> Npc;
	UPROPERTY(Transient) TObjectPtr<AGlossaryLabel> NpcPrompt;
	FName CurrentRow;
	TArray<FString> Path;
	bool bTalking = false;
};
