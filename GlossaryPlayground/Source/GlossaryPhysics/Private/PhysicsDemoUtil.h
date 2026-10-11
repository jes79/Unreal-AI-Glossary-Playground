#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

class ACameraActor;
class AActor;
class AGlossaryDemoBase;
class APlayerController;
class UPhysicalMaterial;

/** 02 물리 분야 데모끼리 쓰는 작은 헬퍼 (고정 카메라, 화면 중앙 트레이스 등) */
namespace PhysicsDemoUtil
{
	/** 고정 카메라를 만든다. 실제 뷰 전환은 KeepViewTarget 이 매 Tick 확인한다(빙의 순서와 무관하게). */
	ACameraActor* SpawnFixedCamera(UWorld* World, const FVector& Location, const FRotator& Rotation, float FOV = 80.f);

	/** PC 의 뷰 타깃이 Camera 가 아니면 바꾼다 (폰 빙의가 BeginPlay 뒤에 일어나도 고정 카메라 유지) */
	void KeepViewTarget(const AGlossaryDemoBase* Demo, ACameraActor* Camera);

	/** 화면 중앙에서 앞으로 트레이스 (커서가 보여도 항상 중앙). 셧(shot) 키 입력으로 클릭 동작을 재현할 때 쓴다 */
	bool TraceScreenCenter(const AGlossaryDemoBase* Demo, FHitResult& OutHit, float Distance = 100000.f,
		ECollisionChannel Channel = ECC_Visibility, const AActor* Ignore = nullptr);

	/** 화면 중앙 시선의 시작점과 방향 */
	bool GetViewRay(const AGlossaryDemoBase* Demo, FVector& OutStart, FVector& OutDir);

	/** 2D 화면 좌표 → 월드 트레이스 (마우스 클릭) */
	bool TraceCursor(const AGlossaryDemoBase* Demo, FHitResult& OutHit, ECollisionChannel Channel = ECC_Visibility);

	/** 생성 에셋 Physical Material 을 불러오고, 없으면(빌더 미실행) 같은 값으로 런타임에 만든다 */
	UPhysicalMaterial* LoadOrMakePhysMat(UObject* Outer, const FString& Path, float Friction, float Restitution,
		bool bFrictionMin, bool bRestitutionMax);
}
