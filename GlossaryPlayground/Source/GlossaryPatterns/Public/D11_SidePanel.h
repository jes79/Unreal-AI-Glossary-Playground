#pragma once

#include "CoreMinimal.h"
#include "GlossaryWidgets.h"
#include "D11_SidePanel.generated.h"

class APlayerController;
class UTextBlock;

/** 11 분야 데모들이 같이 쓰는 로그 카테고리 (Output Log 에서 LogGlossaryPatterns 로 필터) */
GLOSSARYPATTERNS_API DECLARE_LOG_CATEGORY_EXTERN(LogGlossaryPatterns, Log, All);

/**
 * 화면 오른쪽 위 보조 패널: 11 분야는 코드 패턴이 주제라서 "지금 무슨 일이 일어났는지"를
 * 카운터·로그·코드 한 줄로 보여 주는 칸이 필요하다. 정보 패널(왼쪽 위 620×440)과 겹치지 않게 오른쪽에 붙는다.
 */
UCLASS()
class GLOSSARYPATTERNS_API UD11_SidePanel : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	/** 패널을 만들어 뷰포트에 올린다. Anchor 는 (1,0)=오른쪽 위, (1,1)=오른쪽 아래 */
	static UD11_SidePanel* Create(APlayerController* PC, const FString& Title, const FVector2D& Anchor = FVector2D(1.f, 0.f), float FontSize = 13.f);

	void SetTitle(const FString& InTitle);
	void SetBody(const FString& InBody);

protected:
	virtual void BuildTree() override;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BodyText;

	FString PendingTitle;
	FString PendingBody;
	FVector2D AnchorPoint = FVector2D(1.f, 0.f);
	float BodyFontSize = 13.f;
};

/** 최근 N줄만 남기는 화면용 로그. Add 할 때 UE_LOG 로도 같이 남긴다. */
struct GLOSSARYPATTERNS_API FD11_LogLines
{
	explicit FD11_LogLines(int32 InMaxLines = 8) : MaxLines(InMaxLines) {}

	void Add(const FString& Line);
	FString Join() const;
	void Reset() { Lines.Reset(); }

private:
	TArray<FString> Lines;
	int32 MaxLines;
};
