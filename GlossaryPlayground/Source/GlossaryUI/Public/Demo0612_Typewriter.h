#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "Demo0612_Typewriter.generated.h"

class UTextBlock;

/** 대사 한 줄 */
USTRUCT()
struct FDemo0612Line
{
	GENERATED_BODY()

	UPROPERTY() FString Speaker;
	UPROPERTY() FString Text;
};

/** 화면 아래 대화창: 글자를 하나씩 찍고, 다 찍히면 ▼ 표시 */
UCLASS()
class GLOSSARYUI_API UDemo0612_DialogueBox : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	/** 글자 하나당 시간 (초) */
	float CharInterval = 0.03f;

	void StartLine(const FDemo0612Line& Line);
	/** 클릭: 타이핑 중이면 전체 표시, 아니면 false (다음 줄로 넘어가라는 뜻) */
	bool SkipToEnd();
	bool IsTyping() const { return Visible < Full.Len(); }
	int32 GetVisibleCount() const { return Visible; }
	int32 GetFullCount() const { return Full.Len(); }

protected:
	virtual void BuildTree() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void ApplyVisible();

	UPROPERTY(Transient) TObjectPtr<UTextBlock> SpeakerText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BodyText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NextMark;

	FString Full;
	int32 Visible = 0;
	float Accumulator = 0.f;
	float BlinkTime = 0.f;
};

/**
 * 0612 대화창 타이핑 효과
 * 대사를 글자당 0.03초로 타이핑하고, 클릭하면 전체 표시 후 다음 대사로 넘어간다.
 */
UCLASS()
class GLOSSARYUI_API ADemo0612_Typewriter : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0612_Typewriter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 글자당 0.03초 */
	UPROPERTY(EditAnywhere, Category = "Typewriter", meta = (ClampMin = 0.001)) float CharInterval = 0.03f;

	UPROPERTY(EditAnywhere, Category = "Typewriter") TArray<FDemo0612Line> Lines;

	UPROPERTY(Transient) TObjectPtr<UDemo0612_DialogueBox> Box;

private:
	void Advance();
	int32 LineIndex = 0;
	int32 SkipCount = 0;
};
