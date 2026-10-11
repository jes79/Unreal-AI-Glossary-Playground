#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"

class AActor;
class APawn;
class APlayerController;
class UInputAction;
class UInputMappingContext;
class UEnhancedInputLocalPlayerSubsystem;
class UWorld;

/** 01 분야 데모끼리 공유하는 작은 헬퍼 (실행 중 Enhanced Input 에셋 조립, 매핑 컨텍스트 목록, 단순 투사체) */
namespace MovementKit
{
	/** 실행 중에 Input Action 을 만든다 (에디터의 IA_ 에셋과 같은 객체). Outer 가 살아 있는 동안 유지 */
	GLOSSARYMOVEMENT_API UInputAction* MakeAction(UObject* Outer, const FName& Name, EInputActionValueType ValueType);

	/** 빈 Input Mapping Context (에디터의 IMC_ 에셋과 같은 객체) */
	GLOSSARYMOVEMENT_API UInputMappingContext* MakeContext(UObject* Outer, const FName& Name);

	/**
	 * WASD 를 Axis2D 액션에 매핑: D=+X, A=Negate(-X), W=Swizzle(+Y), S=Swizzle+Negate(-Y).
	 * 템플릿 IMC_Default 의 IA_Move 와 같은 구성.
	 */
	GLOSSARYMOVEMENT_API void MapWASD(UInputMappingContext* Context, UInputAction* Action);

	/** 게임패드 스틱(2D)을 Dead Zone 모디파이어와 함께 매핑 */
	GLOSSARYMOVEMENT_API void MapStick(UInputMappingContext* Context, UInputAction* Action, const FKey& StickKey);

	GLOSSARYMOVEMENT_API UEnhancedInputLocalPlayerSubsystem* GetInputSubsystem(const APlayerController* PC);

	/** 이름·IMC 목록 중 지금 서브시스템에 올라가 있는 것을 "이름 (우선순위 N)" 줄로 */
	struct FContextEntry
	{
		FString Name;
		const UInputMappingContext* Context = nullptr;
	};
	GLOSSARYMOVEMENT_API FString DescribeContexts(const APlayerController* PC, const TArray<FContextEntry>& Contexts);

	/** 템플릿 /Game/Input/IMC_Default · IMC_MouseLook */
	GLOSSARYMOVEMENT_API UInputMappingContext* LoadTemplateDefaultContext();
	GLOSSARYMOVEMENT_API UInputMappingContext* LoadTemplateMouseLookContext();

	/** 일직선으로 날아가는 공 (0108 대시 무적, 0112 피격 흔들림에서 사용). 물리·콜리전 없이 거리로 명중 판정 */
	struct GLOSSARYMOVEMENT_API FSimpleProjectiles
	{
		struct FShot
		{
			TWeakObjectPtr<AActor> Actor;
			FVector Velocity = FVector::ZeroVector;
			float Life = 0.f;
			bool bPassed = false;   // 무적으로 통과한 공은 다시 판정하지 않는다
		};
		TArray<FShot> Shots;
		float Radius = 25.f;

		void Fire(UWorld* World, const FVector& From, const FVector& Velocity, const FLinearColor& Color, float InRadius = 25.f, float Life = 4.f);

		/**
		 * 이동 + 명중 판정. 캡슐(반경 R, 반높이 H) 안에 들어오면 OnHit(bool bBlocked) 호출.
		 * bInvincible 이면 공은 그대로 통과하고 OnHit(true) — '회피' 로 센다.
		 */
		void Tick(float DeltaSeconds, const APawn* Target, bool bInvincible, TFunctionRef<void(bool bDodged)> OnHit);
		void Clear();
	};
}
