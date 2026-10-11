#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "Demo0618_Localization.generated.h"

class UTextBlock;

/** String Table 키만 쓰는 메뉴 화면. 문자열을 코드에 하드코딩하지 않는다 */
UCLASS()
class GLOSSARYUI_API UDemo0618_Menu : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	/** 언어가 바뀐 뒤 화면 갱신 (FText 는 그대로 두고 다시 넣기만 한다) */
	void RefreshTexts();

protected:
	virtual void BuildTree() override;

	/** (텍스트 블록, String Table 에서 가져온 FText) 쌍. FText 는 한 번만 만든다 */
	TArray<TPair<TObjectPtr<UTextBlock>, FText>> Bound;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HealthLine;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> FormatLine;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CultureLine;

	UTextBlock* AddKeyText(class UVerticalBox* Column, const TCHAR* Key, float Size, const FLinearColor& Color, bool bBold = false);
};

/**
 * 0618 텍스트 현지화 (FText · String Table)
 * UI 문자열을 FText 와 String Table 로 바꾸고 한국어·영어를 전환한다.
 */
UCLASS()
class GLOSSARYUI_API ADemo0618_Localization : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0618_Localization();

	/** 코드로 만든 String Table 의 ID 와 네임스페이스 */
	static const FName StringTableId;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(Transient) TObjectPtr<UDemo0618_Menu> Menu;

private:
	static void RegisterStringTable();
	void SetLanguage(const FString& Culture);
	FString OriginalCulture;
	FString CurrentCulture;
	int32 SwitchCount = 0;
};
