#pragma once

#include "CoreMinimal.h"

class AActor;
class ACameraActor;
class AGlossaryDemoBase;
class UPostProcessComponent;
class UWorld;
class AVolume;

/**
 * 10 분야(조명·렌더링·최적화) 데모가 같이 쓰는 작은 도구.
 * - 프레임 통계(stat unit 과 같은 숫자)를 읽어 상태 줄에 표시
 * - 콘솔 변수를 바꾸고 레벨을 나갈 때 원래 값으로 되돌림(다른 레벨에 영향이 남지 않게)
 * - 고정 카메라 / 키보드 비행 카메라
 */
namespace RenderKit
{
	/** stat unit 의 Frame/Game/Draw/GPU 와 RHI 드로우콜 수. 매 Tick Update 하고 ToString 으로 표시 */
	struct GLOSSARYRENDERING_API FFrameStats
	{
		float FPS = 0.f;
		float FrameMs = 0.f;
		float GameMs = 0.f;
		float DrawMs = 0.f;   // 렌더 스레드 (stat unit 의 Draw)
		float GPUMs = 0.f;
		int32 DrawCalls = 0;
		int32 Primitives = 0;

		void Update(float DeltaSeconds);
		/** "FPS 60 · Frame 16.6ms · Game 2.1 · Draw 3.0 · GPU 8.2 · DrawCalls 120" */
		FString ToString(bool bWithDrawCalls = true) const;
		/** 가장 오래 걸린 스레드 이름 (Game/Draw/GPU) — stat unit 으로 병목을 찾는 방법 그대로 */
		FString Bottleneck() const;
	};

	/** 콘솔 변수를 바꾸면서 처음 값을 기억해 두었다가 RestoreAll 로 되돌린다 */
	struct GLOSSARYRENDERING_API FCVarScope
	{
		void Set(const TCHAR* Name, const FString& Value);
		void Set(const TCHAR* Name, int32 Value) { Set(Name, FString::FromInt(Value)); }
		void Set(const TCHAR* Name, float Value) { Set(Name, FString::SanitizeFloat(Value)); }
		void RestoreAll();
		~FCVarScope() { RestoreAll(); }

	private:
		TMap<FString, FString> Originals;
	};

	GLOSSARYRENDERING_API FString GetCVar(const TCHAR* Name);
	GLOSSARYRENDERING_API int32 GetCVarInt(const TCHAR* Name);

	/** 고정 카메라를 스폰해 플레이어 시점으로 만든다 */
	GLOSSARYRENDERING_API ACameraActor* SpawnViewCamera(AGlossaryDemoBase* Demo, const FVector& Location, const FRotator& Rotation, float FOV = 70.f);

	/** 카메라를 Target 을 바라보게 놓는다 */
	GLOSSARYRENDERING_API ACameraActor* SpawnLookAtCamera(AGlossaryDemoBase* Demo, const FVector& Location, const FVector& Target, float FOV = 70.f);

	/**
	 * 키보드 비행 카메라: W/S 앞뒤 · A/D 좌우 · Q/E 아래위 · ←/→ 회전 · ↑/↓ 고개. Shift 를 누르면 4배 빠르게.
	 * PlayerPawnClass = nullptr 인 데모에서 Tick 마다 부른다.
	 */
	GLOSSARYRENDERING_API void TickFlyCamera(AGlossaryDemoBase* Demo, ACameraActor* Camera, float DeltaSeconds, float Speed = 1200.f);

	/** 데모 액터에 경계 없는(Unbound) Post Process 를 붙인다. 공통 환경(Priority -1)보다 우선 */
	GLOSSARYRENDERING_API UPostProcessComponent* AddPostProcess(AActor* Owner, float Priority = 10.f);

#if WITH_EDITOR
	/** BuildLevel 용: 상자 모양 브러시 볼륨(CullDistanceVolume·PostProcessVolume 등)을 놓는다 */
	GLOSSARYRENDERING_API AVolume* SpawnBoxVolume(UWorld* World, UClass* VolumeClass, const FVector& Center, const FVector& Size);
#endif
}
