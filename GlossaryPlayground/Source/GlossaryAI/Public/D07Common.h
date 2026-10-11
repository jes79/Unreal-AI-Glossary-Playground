#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GlossaryWidgets.h"
#include "D07Common.generated.h"

class AAIController;
class ACameraActor;
class AGlossaryDemoBase;
class AGlossaryLabel;
class UAnimSequence;
class UBTCompositeNode;
class UBTNode;
class UStaticMeshComponent;
class UMaterialInterface;
class UPrimitiveComponent;
class UTextBlock;

/**
 * 07 분야 데모가 같이 쓰는 AI 캐릭터.
 * Manny + ABP_Unarmed 를 그대로 쓰고(걷기·뛰기·점프 애니메이션은 ABP 가 속도로 고른다),
 * 몸 전체를 단색 머티리얼로 칠해 플레이어(기본 Manny)와 구분한다. 머리 위에는 상태 표시용 구슬과 라벨.
 * AIControllerClass 는 데모마다 바꿔 끼운다(SpawnEnemy 의 ControllerClass).
 */
UCLASS()
class GLOSSARYAI_API AD07Enemy : public ACharacter
{
	GENERATED_BODY()

public:
	AD07Enemy(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 모든 머티리얼 슬롯을 단색으로 (적=빨강, NPC=초록 …) */
	void SetBodyColor(const FLinearColor& Color);
	/** 머리 위 구슬 색 (상태 표시) */
	void SetMarkerColor(const FLinearColor& Color, float Emissive = 2.f);
	/** 머리 위 라벨 문구 (처음 부를 때 라벨 액터를 만든다) */
	void SetLabel(const FString& Text, const FLinearColor& Color = FLinearColor::White);

	/** 공격 애니메이션(MM_Attack_01)을 ABP 의 DefaultSlot 에 몽타주로 재생 */
	void PlayAttack();

	float GetHealthRatio() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float MaxHealth = 100.f;

	/** 공격 횟수 (상태 표시용) */
	int32 AttackCount = 0;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UStaticMeshComponent> Marker;

	UPROPERTY(EditAnywhere, Category = "AI")
	TObjectPtr<UAnimSequence> AttackAnim;

	UPROPERTY(Transient)
	TObjectPtr<AGlossaryLabel> Label;
};

/** 화면 오른쪽 위에 붙는 보조 패널 (BT 트리·StateTree 상태 등 여러 줄 텍스트). 정보 패널(왼쪽 위)과 겹치지 않는다. */
UCLASS()
class GLOSSARYAI_API UD07SidePanel : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	void SetPanelText(const FString& InTitle, const FString& InBody);

protected:
	virtual void BuildTree() override;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BodyText;
	FString PendingTitle, PendingBody;
};

/** 07 분야 공통 헬퍼 */
namespace D07
{
	/** 07 전용 단색 머티리얼 M_AgentColor (스켈레탈·인스턴스 메시 Usage 켜짐). 없으면 공통 M_Color */
	GLOSSARYAI_API UMaterialInterface* GetAgentMaterial();
	/** M_AgentColor 로 컴포넌트의 MaterialIndex 슬롯을 칠한다 */
	GLOSSARYAI_API void SetAgentColor(UPrimitiveComponent* Component, int32 MaterialIndex, const FLinearColor& Color, float Emissive = 0.f);

	/** 바닥 높이의 평면 시야 부채꼴 (반경, 좌우 반각) */
	GLOSSARYAI_API void DrawSightFan(UWorld* World, const FVector& Origin, const FVector& Forward, float Radius, float HalfAngleDeg, const FColor& Color, float Thickness = 4.f);

	/** 바닥(z=0) 위에 AI 캐릭터를 스폰한다. ControllerClass 를 AIControllerClass 로 끼운 뒤 스폰을 마치므로 바로 빙의된다. */
	GLOSSARYAI_API AD07Enemy* SpawnEnemy(UWorld* World, const FVector& GroundLocation, float Yaw,
		TSubclassOf<AAIController> ControllerClass, const FLinearColor& BodyColor, TSubclassOf<AD07Enemy> PawnClass = nullptr);

	/**
	 * 고정 오버뷰 카메라. 플레이어 캐릭터는 그대로 WASD 로 움직일 수 있게 컨트롤 회전 Yaw 를 카메라에 맞춘다.
	 * V 키로 캐릭터 카메라와 전환하려면 ToggleOverviewCamera 를 부른다.
	 */
	GLOSSARYAI_API ACameraActor* SetupOverviewCamera(AGlossaryDemoBase* Demo, const FVector& Location, const FRotator& Rotation, float FOV = 70.f);
	GLOSSARYAI_API void ToggleOverviewCamera(AGlossaryDemoBase* Demo, ACameraActor* Camera);

	/** 오른쪽 위 보조 패널 */
	GLOSSARYAI_API UD07SidePanel* CreateSidePanel(AGlossaryDemoBase* Demo);

	/** AI 의 현재 경로(PathFollowingComponent)를 선으로 그린다 */
	GLOSSARYAI_API void DrawCurrentPath(AAIController* Controller, const FColor& Color, float Thickness = 6.f);

	/**
	 * NavMesh 폴리곤을 반투명 면 + 테두리로 그린다(Nav Area 별 색). "show Navigation" 과 같은 것을 DrawDebug 로.
	 * 매 프레임 부르되 폴리곤 목록은 RefreshInterval 초마다 다시 읽는다.
	 */
	GLOSSARYAI_API void DrawNavMesh(UWorld* World, float ZOffset = 4.f, float RefreshInterval = 0.5f);

	/** BT 를 들여쓰기 텍스트로: 활성 노드는 ▶ 표시. 데코레이터는 [조건], 서비스는 (서비스) */
	GLOSSARYAI_API FString DescribeBehaviorTree(const UBTCompositeNode* Root, const UBTNode* ActiveNode);

	// ---- 리플렉션 설정 헬퍼: 엔진 BT 노드는 protected UPROPERTY 가 많아서 이름으로 값을 넣는다 (에디터 디테일 패널과 같은 경로) ----
	GLOSSARYAI_API void* FindPropertyPtr(UObject* Object, FName PropertyName);
	GLOSSARYAI_API bool SetBoolProperty(UObject* Object, FName PropertyName, bool bValue);
	/** FBlackboardKeySelector 타입 프로퍼티의 SelectedKeyName 을 지정 */
	GLOSSARYAI_API bool SetBlackboardKey(UObject* Object, FName PropertyName, FName KeyName);

	template <typename T>
	bool SetProperty(UObject* Object, FName PropertyName, const T& Value)
	{
		if (T* Ptr = static_cast<T*>(FindPropertyPtr(Object, PropertyName)))
		{
			*Ptr = Value;
			return true;
		}
		return false;
	}
}
