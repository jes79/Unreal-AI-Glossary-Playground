#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "Demo0107_VariableJump.generated.h"

class ACharacter;
class AGlossaryLabel;

/**
 * 0107 가변 점프 (Jump Max Hold Time)
 * JumpMaxHoldTime 0.3초 동안은 버튼을 누르고 있는 만큼 상승 속도가 유지돼 더 높이 뛴다.
 * 자동으로 짧게/길게 누르는 두 캐릭터와 높이 눈금, 최고점 표시로 비교한다.
 */
UCLASS()
class GLOSSARYMOVEMENT_API ADemo0107_VariableJump : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0107_VariableJump();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 요청 예문 값: 이 시간까지 누르고 있으면 상승 속도(JumpZVelocity)가 유지된다 */
	UPROPERTY(EditAnywhere, Category = "Variable Jump", meta = (ClampMin = 0))
	float JumpMaxHoldTime = 0.3f;

	/** 짧게 눌렀을 때의 점프 속도 (낮게 해 두어야 차이가 잘 보인다) */
	UPROPERTY(EditAnywhere, Category = "Variable Jump")
	float JumpZVelocity = 420.f;

private:
	struct FJumper
	{
		TWeakObjectPtr<ACharacter> Character;
		TWeakObjectPtr<AGlossaryLabel> Label;
		FString Name;
		FLinearColor Color;
		float AutoHold = -1.f;      // 자동 점프: 누르고 있을 시간 (-1 = 플레이어)
		float Timer = 0.f;
		bool bHolding = false;
		float PressTime = 0.f;      // 이번 점프에서 누른 시간
		float StartZ = 0.f;
		float ApexZ = 0.f;
		bool bInAir = false;
		TArray<TWeakObjectPtr<AActor>> Markers;
	};
	TArray<FJumper> Jumpers;
	bool bVariableJump = true;

	void Apply(ACharacter* Character) const;
	void TrackJump(FJumper& J, float DeltaSeconds, bool bKeyDown);
	void AddJumper(ACharacter* Character, const FString& Name, const FLinearColor& Color, float AutoHold);
};
