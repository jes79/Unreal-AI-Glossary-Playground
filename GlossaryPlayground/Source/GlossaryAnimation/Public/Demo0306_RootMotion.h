#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0306_RootMotion.generated.h"

class AD03_Mannequin;
class UAnimMontage;

/**
 * 0306 Root Motion
 * 같은 대시 몽타주(AM_Dash, MM_Dash 의 Root Motion 켜짐)를 두 캐릭터가 재생한다.
 * 왼쪽은 Root Motion 을 캡슐 이동으로 쓰고(RootMotionFromMontagesOnly), 오른쪽은 추출하지 않는다(NoRootMotionExtraction).
 */
UCLASS()
class GLOSSARYANIMATION_API ADemo0306_RootMotion : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0306_RootMotion();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 대시 반복 간격(초) */
	UPROPERTY(EditAnywhere, Category = "Root Motion")
	float DashInterval = 3.f;

private:
	void StartDash();

	UPROPERTY(Transient) TObjectPtr<UAnimMontage> DashMontage;
	UPROPERTY(Transient) TObjectPtr<AD03_Mannequin> WithRM;
	UPROPERTY(Transient) TObjectPtr<AD03_Mannequin> WithoutRM;
	float Timer = 0.f;
	FVector StartA, StartB;
	float MaxMeshOffsetB = 0.f;
};
