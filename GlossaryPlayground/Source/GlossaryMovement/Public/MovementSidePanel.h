#pragma once

#include "CoreMinimal.h"
#include "GlossaryWidgets.h"
#include "MovementSidePanel.generated.h"

class APlayerController;
class UTextBlock;

/**
 * 01 분야 데모 공용: 화면 오른쪽 위에 붙는 보조 패널(이벤트 로그·활성 매핑 컨텍스트 목록 등).
 * 왼쪽 위 정보 패널 자리(620×440)를 피해서 오른쪽에 앵커한다.
 */
UCLASS()
class GLOSSARYMOVEMENT_API UMovementSidePanel : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	/** 패널을 만들어 뷰포트에 올린다 */
	static UMovementSidePanel* Create(APlayerController* PC, const FString& Title, float Width = 440.f);

	void SetTitle(const FString& InTitle);
	void SetBody(const FString& InBody);

protected:
	virtual void BuildTree() override;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BodyText;

	FString PendingTitle;
	FString PendingBody;
	float PanelWidth = 440.f;
};
