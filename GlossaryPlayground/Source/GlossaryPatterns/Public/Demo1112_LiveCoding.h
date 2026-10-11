#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "D11_SidePanel.h"
#include "Demo1112_LiveCoding.generated.h"

class AGlossaryLabel;
class UStaticMeshComponent;

/** GetLiveTuning() 이 돌려주는 값 묶음 (구조체 '모양'을 바꾸면 Live Coding 으로는 안 되고 재시작이 필요) */
struct FD1112_Tuning
{
	float RotationSpeed = 0.f; // 도/초
	FLinearColor Color = FLinearColor::White;
	int32 Spokes = 0;          // 바퀴살 개수
	const TCHAR* Message = TEXT("");
};

/**
 * 1112 Live Coding
 * 톱니바퀴의 회전 속도·색·바퀴살 수·문구가 모두 GetLiveTuning() '함수 본문'에서 나온다.
 * 에디터 PIE 중에 Demo1112_LiveCoding.cpp 의 숫자를 고치고 Ctrl+Alt+F11 을 누르면 재시작 없이 바로 바뀐다.
 * 헤더(클래스 멤버·UPROPERTY·UFUNCTION·구조체 모양)를 바꾸면 메모리 배치·리플렉션이 바뀌어 에디터를 다시 켜야 한다.
 */
UCLASS()
class GLOSSARYPATTERNS_API ADemo1112_LiveCoding : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1112_LiveCoding();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** ▼ Live Coding 실습 대상: 이 함수 본문(.cpp)만 고친다 */
	FD1112_Tuning GetLiveTuning() const;

	void RebuildSpokes(int32 Count);
	FString DescribeLiveCoding() const;

	UPROPERTY(Transient) TObjectPtr<AActor> Gear;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> SpokeMeshes;
	UPROPERTY(Transient) TObjectPtr<AGlossaryLabel> MessageLabel;
	UPROPERTY(Transient) TObjectPtr<UD11_SidePanel> Panel;

	int32 PatchCount = 0;
	FDelegateHandle PatchHandle;
	FString LastMessage;
	FLinearColor LastColor = FLinearColor::Black;
};
