#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "D0114_ViewCharacter.h"
#include "Demo0114_CameraViews.generated.h"

/**
 * 0114 시점 전환 (TPS · FPS · 탑다운 · 횡스크롤)
 * 1~4 키로 AD0114_ViewCharacter 의 시점을 바꾼다. 탑다운은 마우스 커서 + 클릭 이동(SimpleMoveToLocation, NavMesh).
 */
UCLASS()
class GLOSSARYMOVEMENT_API ADemo0114_CameraViews : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0114_CameraViews();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void SwitchView(ED0114View View);
	TWeakObjectPtr<AActor> ClickMarker;
};
