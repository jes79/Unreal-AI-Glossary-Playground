#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "D11_SidePanel.h"
#include "Demo1107_SoftReference.generated.h"

class ASkeletalMeshActor;
class UAnimSequence;
class USkeletalMesh;
struct FStreamableHandle;

/**
 * 1107 Soft Reference / 비동기 로드
 * 보스 메시를 TSoftObjectPtr(경로만 들고 있는 참조)로 들고 있다가, 1 키에서 FStreamableManager::RequestAsyncLoad 로
 * 백그라운드 로드 → 완료 콜백에서 스폰한다. 2 키로 핸들을 놓고 GC 하면 메모리에서 다시 내려간다.
 *
 * 하드 레퍼런스(TObjectPtr<USkeletalMesh>)였다면 이 레벨을 여는 순간 메시·머티리얼·텍스처가 모두 같이 로드됐을 것이다.
 */
UCLASS()
class GLOSSARYPATTERNS_API ADemo1107_SoftReference : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1107_SoftReference();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 보스 메시 — 경로만 저장된다. 플레이어(Manny)가 쓰지 않는 Quinn 메시라 처음엔 메모리에 없다 */
	UPROPERTY(EditAnywhere, Category = "Soft Reference")
	TSoftObjectPtr<USkeletalMesh> BossMesh;

	/** 보스 등장 애니메이션 — 메시와 한 번의 요청으로 같이 로드 */
	UPROPERTY(EditAnywhere, Category = "Soft Reference")
	TSoftObjectPtr<UAnimSequence> BossIntroAnim;

	UPROPERTY(EditAnywhere, Category = "Soft Reference", meta = (ClampMin = 0.5))
	float BossScale = 1.8f;

private:
	enum class ELoadState : uint8 { Unloaded, Loading, Loaded };

	void StartAsyncLoad();
	/** RequestAsyncLoad 완료 콜백 (게임 스레드) */
	void OnBossLoaded();
	void UnloadBoss();
	FString StateName() const;

	TSharedPtr<FStreamableHandle> LoadHandle;
	UPROPERTY(Transient) TObjectPtr<ASkeletalMeshActor> Boss;
	UPROPERTY(Transient) TObjectPtr<UD11_SidePanel> Panel;

	ELoadState State = ELoadState::Unloaded;
	double LoadStartTime = 0.0;
	double LastLoadMs = 0.0;
	int32 LoadCount = 0;
	FD11_LogLines Log{6};
};
