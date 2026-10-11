#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GlossaryCatalog.h"
#include "GlossaryDemoBase.generated.h"

class ACharacter;
class APlayerController;
class UGlossaryInfoPanel;

/** 구현 방식 (위키 표의 '구현 방식' 열) */
UENUM(BlueprintType)
enum class EGlossaryImplementation : uint8
{
	Runtime     UMETA(DisplayName = "런타임 데모"),
	Alternative UMETA(DisplayName = "설명 + 대체 구현"),
	EditorTool  UMETA(DisplayName = "에디터 도구 데모"),
};

UENUM(BlueprintType)
enum class EGlossarySky : uint8
{
	Day,     // 맑은 낮 (기본)
	Sunset,  // 낮은 해, 주황빛
	Night,   // 달빛 정도의 어두운 하늘
	Indoor,  // 해·하늘 없음, 약한 앰비언트 (라이트 데모용)
	None,    // 조명 없음 (데모가 직접 만든다)
};

/** 빌더가 레벨에 놓는 AGlossaryEnvironment 설정. 데모 생성자에서 Environment.* 로 바꾼다. */
USTRUCT(BlueprintType)
struct GLOSSARYCORE_API FGlossaryEnvironmentSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Environment) EGlossarySky Sky = EGlossarySky::Day;
	/** 격자 바닥 (가로세로 FloorSize cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Environment) bool bFloor = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Environment) float FloorSize = 6000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Environment) bool bFog = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Environment) bool bClouds = false;
	/** 자동 노출을 끄고 EV100 을 고정한다(데모끼리 밝기 비교가 쉬움). 0527 처럼 노출 데모는 false. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Environment) bool bFixedExposure = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Environment) float ExposureEV100 = 10.f;
};

/**
 * 예제 레벨 하나 = 이 클래스를 상속한 데모 액터 하나 (클래스 이름 Demo{Id}_{Name}).
 * 빌더(GlossaryBuild 커맨드렛)가 레벨에 이 액터 + 환경 + PlayerStart (+ NavMesh) 를 놓고,
 * 데모는 생성자(에디터에서도 보이는 컴포넌트)와 BeginPlay(실행 중 스폰)에서 장면을 만든다.
 */
UCLASS(Abstract)
class GLOSSARYCORE_API AGlossaryDemoBase : public AActor
{
	GENERATED_BODY()

public:
	AGlossaryDemoBase();

	// ---------- 데모 설정 (서브클래스 생성자에서 지정) ----------

	/** "0105" — 카탈로그 키. 생성자에서 반드시 지정 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Glossary)
	FString DemoId;

	/** 조작법과 "무엇을 관찰할지" 2~4줄 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Glossary, meta = (MultiLine = true))
	FString Controls;

	/** 위키 '씬에서 보여 주는 것' 한 줄 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Glossary, meta = (MultiLine = true))
	FString Showcase;

	/** 대체 구현이면 무엇을 왜 대체했는지 (정보 패널에 표시) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Glossary, meta = (MultiLine = true))
	FString AlternativeNote;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Glossary)
	EGlossaryImplementation Implementation = EGlossaryImplementation::Runtime;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Glossary)
	FGlossaryEnvironmentSettings Environment;

	/** 플레이어 폰. 기본은 AGlossaryCharacter(3인칭 Manny). null 이면 관전자(카메라만) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Glossary)
	TSubclassOf<APawn> PlayerPawnClass;

	/** 빌더가 PlayerStart 를 놓는 위치 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Glossary)
	FTransform PlayerStartTransform;

	/** true 면 빌더가 NavMeshBoundsVolume(NavBoundsExtent) 을 놓는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Glossary)
	bool bNeedsNavMesh = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Glossary)
	FVector NavBoundsExtent = FVector(3000.f, 3000.f, 600.f);

	/** 시작할 때 마우스 커서를 보이고 클릭을 받는다 (UI 데모). F3 으로 언제든 전환 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Glossary)
	bool bShowMouseCursor = false;

	// ---------- 데모에서 쓰는 헬퍼 ----------

	/** 정보 패널 아래 '상태' 줄 갱신 */
	UFUNCTION(BlueprintCallable, Category = Glossary)
	void SetStatus(const FString& Status);

	APlayerController* GetPC() const;
	APawn* GetPlayerPawn() const;
	ACharacter* GetPlayerCharacter() const;

	/** 키 입력을 바로 읽는다. 데모 토글(1~5, C 등)은 이걸로, 캐릭터 조작은 Enhanced Input 으로. */
	bool WasKeyPressed(const FKey& Key) const;
	bool WasKeyReleased(const FKey& Key) const;
	bool IsKeyDown(const FKey& Key) const;

	/** 마우스 커서 아래 월드 위치 (Visibility 채널). 커서가 숨겨져 있으면 화면 중앙 */
	bool GetMouseWorldHit(FHitResult& OutHit, ECollisionChannel Channel = ECC_Visibility) const;

	const FGlossaryEntry* GetEntry() const;
	UGlossaryInfoPanel* GetInfoPanel() const { return InfoPanel; }

#if WITH_EDITOR
	/**
	 * 빌더가 레벨을 만들 때(에디터, 저장 직전) 한 번 호출된다. 레벨에 저장돼야 하는 것을 여기서 놓는다:
	 * 볼륨(Nav Modifier·Audio·Post Process·Cull Distance), 정적 라이트, 레벨 스트리밍 설정 등.
	 * 실행 중에만 필요한 것은 BeginPlay 에서 스폰한다.
	 */
	virtual void BuildLevel(UWorld* World) {}
#endif

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 루트 (에디터에서 이동 기즈모용) */
	UPROPERTY(VisibleAnywhere, Category = Glossary)
	TObjectPtr<USceneComponent> Root;

private:
	void CreateInfoPanel();

	UPROPERTY(Transient)
	TObjectPtr<UGlossaryInfoPanel> InfoPanel;
};
