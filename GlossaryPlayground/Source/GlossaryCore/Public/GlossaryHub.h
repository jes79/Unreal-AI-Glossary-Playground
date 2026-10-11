#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "GlossaryHub.generated.h"

class UScrollBox;
class UTextBlock;
class UVerticalBox;

/** 클릭하면 Payload 문자열을 넘겨주는 버튼 (버튼마다 다른 값을 가진 목록용) */
UCLASS()
class GLOSSARYCORE_API UGlossaryPayloadButton : public UButton
{
	GENERATED_BODY()

public:
	FString Payload;
	TFunction<void(const FString&)> OnPicked;

	void Bind();

private:
	UFUNCTION() void HandleClicked();
};

/** 허브 화면: 왼쪽 분야 11개, 오른쪽 그 분야의 용어 목록. 용어를 누르면 해당 예제 레벨로 이동 */
UCLASS()
class GLOSSARYCORE_API UGlossaryHubWidget : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	void ShowCategory(int32 CategoryIndex);

protected:
	virtual void BuildTree() override;
	virtual void NativeConstruct() override;

	void OpenDemo(const FString& Id);

	UPROPERTY(Transient) TObjectPtr<UScrollBox> TermList;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CategoryTitle;
	UPROPERTY(Transient) TArray<TObjectPtr<UGlossaryPayloadButton>> CategoryButtons;

	int32 CurrentCategory = 0;
};

/** L_0000_Hub 에 놓이는 허브 액터 */
UCLASS()
class GLOSSARYCORE_API AGlossaryHubDemo : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	AGlossaryHubDemo();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(Transient) TObjectPtr<UGlossaryHubWidget> Hub;
};
