#include "Demo1107_SoftReference.h"

#include "Animation/AnimSequence.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo1107_SoftReference::ADemo1107_SoftReference()
{
	DemoId = TEXT("1107");
	Controls = TEXT("1 : RequestAsyncLoad 로 보스 메시+애니 비동기 로드 → 완료 콜백에서 스폰\n")
		TEXT("2 : 보스 제거 + 핸들 해제 + GC → 메모리에서 내려감 (다시 1 로 로드)\n")
		TEXT("관찰: 시작 직후 '메모리에 있음: 아니오' — 경로만 들고 있다가 필요할 때 불러온다");
	Showcase = TEXT("TSoftObjectPtr 보스 메시를 RequestAsyncLoad 로 불러와 스폰, 로드 상태·시간·언로드 표시");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-500.f, 0.f, 100.f));

	// 생성자에서는 경로만 넣는다. 이 줄은 아무것도 로드하지 않는다.
	BossMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple")));
	BossIntroAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack.MM_ChargedAttack")));
}

void ADemo1107_SoftReference::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	// 보스 등장 자리(받침대)
	SpawnShape(World, EShape::Cylinder, FVector(400.f, 0.f, 10.f), FVector(400.f, 400.f, 20.f), Colors::Purple);
	SpawnLabel(World, TEXT("보스 등장 자리"), FVector(400.f, 0.f, 80.f), Colors::Purple, 16.f);
	Panel = UD11_SidePanel::Create(GetPC(), TEXT("TSoftObjectPtr 상태"), FVector2D(1.f, 0.f), 14.f);
	Log.Add(BossMesh.Get() ? TEXT("시작: Quinn 메시가 이미 메모리에 있음(다른 곳에서 로드됨)") : TEXT("시작: 경로만 있음, 메시는 메모리에 없음"));
}

void ADemo1107_SoftReference::StartAsyncLoad()
{
	if (State != ELoadState::Unloaded)
	{
		return;
	}
	State = ELoadState::Loading;
	LoadStartTime = FPlatformTime::Seconds();
	++LoadCount;
	Log.Add(FString::Printf(TEXT("RequestAsyncLoad 요청 (메시 %s, 애니 %s)"),
		BossMesh.Get() ? TEXT("이미 있음") : TEXT("없음"), BossIntroAnim.Get() ? TEXT("이미 있음") : TEXT("없음")));

	// ▼ 핵심: 경로 목록을 넘기고, 끝나면 부를 함수를 등록한다. 게임 스레드는 멈추지 않는다.
	//    UAssetManager 의 StreamableManager 를 쓰면 프로젝트 전체가 같은 로드 큐·캐시를 공유한다.
	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	TArray<FSoftObjectPath> Paths = { BossMesh.ToSoftObjectPath(), BossIntroAnim.ToSoftObjectPath() };
	LoadHandle = Streamable.RequestAsyncLoad(Paths, FStreamableDelegate::CreateUObject(this, &ADemo1107_SoftReference::OnBossLoaded),
		FStreamableManager::AsyncLoadHighPriority);
}

void ADemo1107_SoftReference::OnBossLoaded()
{
	LastLoadMs = (FPlatformTime::Seconds() - LoadStartTime) * 1000.0;
	State = ELoadState::Loaded;

	// 로드가 끝났으니 Get() 이 실제 객체를 돌려준다
	USkeletalMesh* Mesh = BossMesh.Get();
	if (!Mesh)
	{
		Log.Add(TEXT("로드 실패: 경로를 확인하세요"));
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Boss = GetWorld()->SpawnActor<ASkeletalMeshActor>(FVector(400.f, 0.f, 20.f), FRotator(0.f, 180.f, 0.f), Params);
	USkeletalMeshComponent* SkelComp = Boss->GetSkeletalMeshComponent();
	SkelComp->SetSkeletalMesh(Mesh);
	Boss->SetActorScale3D(FVector(BossScale));
	if (UAnimSequence* Anim = BossIntroAnim.Get())
	{
		SkelComp->PlayAnimation(Anim, true);
	}
	Log.Add(FString::Printf(TEXT("완료 콜백 OnBossLoaded: %.1f ms → 스폰"), LastLoadMs));
}

void ADemo1107_SoftReference::UnloadBoss()
{
	if (State == ELoadState::Loading)
	{
		LoadHandle->CancelHandle(); // 로딩 중이면 취소도 가능
	}
	if (Boss)
	{
		Boss->Destroy();
		Boss = nullptr;
	}
	// 핸들을 놓아야 StreamableManager 가 붙잡고 있던 참조가 풀린다
	if (LoadHandle.IsValid())
	{
		LoadHandle->ReleaseHandle();
		LoadHandle.Reset();
	}
	State = ELoadState::Unloaded;
	// 참조가 하나도 없으면 다음 GC 때 메모리에서 내려간다 (보통은 엔진이 알아서 주기적으로 GC 한다)
	GEngine->ForceGarbageCollection(true);
	Log.Add(TEXT("보스 제거 + ReleaseHandle + GC 요청"));
}

FString ADemo1107_SoftReference::StateName() const
{
	switch (State)
	{
	case ELoadState::Loading: return FString::Printf(TEXT("Loading %.0f%%"), LoadHandle.IsValid() ? LoadHandle->GetProgress() * 100.f : 0.f);
	case ELoadState::Loaded:  return TEXT("Loaded (스폰됨)");
	default:                  return TEXT("Unloaded");
	}
}

void ADemo1107_SoftReference::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One)) { StartAsyncLoad(); }
	if (WasKeyPressed(EKeys::Two)) { UnloadBoss(); }

	// TSoftObjectPtr::Get() 은 로드를 일으키지 않는다. 메모리에 있으면 객체, 없으면 null.
	const bool bMeshInMemory = BossMesh.Get() != nullptr;
	const bool bAnimInMemory = BossIntroAnim.Get() != nullptr;

	if (Panel)
	{
		Panel->SetBody(FString::Printf(
			TEXT("BossMesh   = %s\n  IsNull() %s · IsValid() %s · 메모리에 있음: %s\n")
			TEXT("BossIntroAnim = %s\n  메모리에 있음: %s\n\n")
			TEXT("상태: %s\n로드 횟수 %d · 마지막 로드 %.1f ms\n\n")
			TEXT("코드:\n  UAssetManager::GetStreamableManager()\n    .RequestAsyncLoad(Paths,\n      FStreamableDelegate::CreateUObject(this, &OnBossLoaded));\n\n%s"),
			*BossMesh.ToSoftObjectPath().GetAssetName(), BossMesh.IsNull() ? TEXT("true") : TEXT("false"), BossMesh.IsValid() ? TEXT("true") : TEXT("false"),
			bMeshInMemory ? TEXT("예") : TEXT("아니오"),
			*BossIntroAnim.ToSoftObjectPath().GetAssetName(), bAnimInMemory ? TEXT("예") : TEXT("아니오"),
			*StateName(), LoadCount, LastLoadMs, *Log.Join()));
	}
	SetStatus(FString::Printf(TEXT("상태 %s · 메시 메모리에 %s\n(소프트 레퍼런스는 경로 문자열 + 약한 포인터 — 레벨을 열 때 같이 로드되지 않는다)"),
		*StateName(), bMeshInMemory ? TEXT("있음") : TEXT("없음")));
}

void ADemo1107_SoftReference::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (LoadHandle.IsValid())
	{
		LoadHandle->CancelHandle();
		LoadHandle.Reset();
	}
	Super::EndPlay(EndPlayReason);
}
