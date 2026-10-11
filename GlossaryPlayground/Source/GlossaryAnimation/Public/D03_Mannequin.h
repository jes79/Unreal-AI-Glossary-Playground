#pragma once

#include "CoreMinimal.h"
#include "GlossaryCharacter.h"
#include "D03_Mannequin.generated.h"

class UAnimSequence;
class UAnimMontage;
class UD03_AnimInstance;
class AGlossaryLabel;
class ACameraActor;
class APlayerController;

/**
 * 03 분야 데모용 NPC Manny. 플레이어 캐릭터와 같은 메시·ABP_Unarmed 를 쓰고, 컨트롤러 없이
 * 코드가 넣는 이동 입력(자동 조종)으로 걷고 뛴다. 스크린샷에서도 애니메이션 상태가 보이도록 데모가 직접 움직인다.
 */
UCLASS()
class GLOSSARYANIMATION_API AD03_Mannequin : public AGlossaryCharacter
{
	GENERATED_BODY()

public:
	AD03_Mannequin(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 애니메이션을 C++ AnimInstance(UD03_AnimInstance)로 바꾸고 돌려준다 */
	UD03_AnimInstance* UseCppAnimInstance();
	/** 템플릿 ABP_Unarmed 로 되돌린다 */
	void UseTemplateAnimBP();

	/** 자동 조종: 매 프레임 이 방향(월드)으로 입력 Scale 만큼 이동 입력 */
	UPROPERTY(EditAnywhere, Category = "Autopilot")
	FVector AutoMoveDirection = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Autopilot")
	float AutoMoveScale = 1.f;

	/** 두 점 사이 왕복 (PatrolA == PatrolB 면 끔) */
	void SetPatrol(const FVector& A, const FVector& B);

	/** 몸을 이 Yaw 로 고정 (이동 방향으로 돌지 않음 → 옆걸음·뒷걸음) */
	void SetFixedFacing(bool bFixed, float Yaw = 0.f);

	/** 머리 위 라벨 */
	void SetHeadLabel(const FString& Text, const FLinearColor& Color = FLinearColor::White, float Size = 16.f);

	UPROPERTY(Transient)
	TObjectPtr<AGlossaryLabel> HeadLabel;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	FVector PatrolA = FVector::ZeroVector;
	FVector PatrolB = FVector::ZeroVector;
	bool bPatrolToB = true;
	bool bFixedFacing = false;
	float FixedYaw = 0.f;
};

/** 03 분야 공통 헬퍼 */
namespace D03
{
	/** "/Game/Characters/Mannequins/Anims/" + RelPath (예: "Unarmed/Attack/MM_Attack_01") */
	GLOSSARYANIMATION_API UAnimSequence* LoadAnim(const FString& RelPath);
	GLOSSARYANIMATION_API UAnimMontage* LoadMontage(const FString& AssetName);   // /Game/Glossary/03_Animation/{AssetName}
	GLOSSARYANIMATION_API UClass* GetUnarmedAnimClass();

	GLOSSARYANIMATION_API AD03_Mannequin* SpawnMannequin(UWorld* World, const FVector& FeetLocation, float Yaw);

	/** 고정 카메라를 놓고 플레이어 시점으로 */
	GLOSSARYANIMATION_API ACameraActor* SpawnViewCamera(UWorld* World, APlayerController* PC, const FVector& Location, const FVector& LookAt, float FOV = 70.f);

	inline const FString Folder = TEXT("/Game/Glossary/03_Animation/");
}
