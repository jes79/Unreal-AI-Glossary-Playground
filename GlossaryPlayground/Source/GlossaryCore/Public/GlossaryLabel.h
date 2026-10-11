#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GlossaryLabel.generated.h"

class UWidgetComponent;
class UTextBlock;

/** 월드 위치에 떠 있는 한글 라벨. WidgetComponent(Screen) 라서 항상 화면을 향하고 거리와 무관한 크기. */
UCLASS()
class GLOSSARYCORE_API AGlossaryLabel : public AActor
{
	GENERATED_BODY()

public:
	AGlossaryLabel();

	void SetText(const FString& InText);
	void SetColor(const FLinearColor& InColor);
	void SetFontSize(float InSize);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = Label)
	TObjectPtr<UWidgetComponent> Widget;

	UPROPERTY(EditAnywhere, Category = Label)
	FString Text;

	UPROPERTY(EditAnywhere, Category = Label)
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = Label)
	float FontSize = 18.f;

private:
	void Refresh();
};
