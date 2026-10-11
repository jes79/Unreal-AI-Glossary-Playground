#include "MovementKit.h"

#include "Components/CapsuleComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

namespace MovementKit
{
	UInputAction* MakeAction(UObject* Outer, const FName& Name, EInputActionValueType ValueType)
	{
		UInputAction* Action = NewObject<UInputAction>(Outer, Name);
		Action->ValueType = ValueType;
		return Action;
	}

	UInputMappingContext* MakeContext(UObject* Outer, const FName& Name)
	{
		return NewObject<UInputMappingContext>(Outer, Name);
	}

	void MapWASD(UInputMappingContext* Context, UInputAction* Action)
	{
		// D : 그대로 (+X)
		Context->MapKey(Action, EKeys::D);
		// A : Negate → -X
		FEnhancedActionKeyMapping& A = Context->MapKey(Action, EKeys::A);
		A.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
		// W : Swizzle(YXZ) → 1D 입력을 Y 축으로 (+Y = 앞)
		FEnhancedActionKeyMapping& W = Context->MapKey(Action, EKeys::W);
		W.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Context));
		// S : Swizzle + Negate → -Y
		FEnhancedActionKeyMapping& S = Context->MapKey(Action, EKeys::S);
		S.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Context));
		S.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
	}

	void MapStick(UInputMappingContext* Context, UInputAction* Action, const FKey& StickKey)
	{
		FEnhancedActionKeyMapping& Stick = Context->MapKey(Action, StickKey);
		// 스틱이 살짝 기울어진 잡음은 무시 (Radial Dead Zone)
		Stick.Modifiers.Add(NewObject<UInputModifierDeadZone>(Context));
	}

	UEnhancedInputLocalPlayerSubsystem* GetInputSubsystem(const APlayerController* PC)
	{
		return PC ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()) : nullptr;
	}

	FString DescribeContexts(const APlayerController* PC, const TArray<FContextEntry>& Contexts)
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem = GetInputSubsystem(PC);
		if (!Subsystem)
		{
			return TEXT("(서브시스템 없음)");
		}
		FString Out;
		for (const FContextEntry& Pair : Contexts)
		{
			int32 Priority = 0;
			const bool bActive = Pair.Context && Subsystem->HasMappingContext(Pair.Context, Priority);
			Out += bActive
				? FString::Printf(TEXT("[켜짐] %s · 우선순위 %d\n"), *Pair.Name, Priority)
				: FString::Printf(TEXT("[꺼짐] %s\n"), *Pair.Name);
		}
		return Out.TrimEnd();
	}

	UInputMappingContext* LoadTemplateDefaultContext()
	{
		return GlossaryKit::LoadAsset<UInputMappingContext>(TEXT("/Game/Input/IMC_Default"));
	}

	UInputMappingContext* LoadTemplateMouseLookContext()
	{
		return GlossaryKit::LoadAsset<UInputMappingContext>(TEXT("/Game/Input/IMC_MouseLook"));
	}

	void FSimpleProjectiles::Fire(UWorld* World, const FVector& From, const FVector& Velocity, const FLinearColor& Color, float InRadius, float Life)
	{
		Radius = InRadius;
		AStaticMeshActor* Ball = GlossaryKit::SpawnShape(World, GlossaryKit::EShape::Sphere, From, FVector(InRadius * 2.f), Color);
		if (!Ball)
		{
			return;
		}
		// 판정은 거리로 직접 한다 — 공 자체는 아무것도 막지 않는다
		Ball->SetActorEnableCollision(false);
		GlossaryKit::SetColor(Ball->GetStaticMeshComponent(), Color, 0.4f, 0.f, 2.f);
		FShot Shot;
		Shot.Actor = Ball;
		Shot.Velocity = Velocity;
		Shot.Life = Life;
		Shots.Add(Shot);
	}

	void FSimpleProjectiles::Tick(float DeltaSeconds, const APawn* Target, bool bInvincible, TFunctionRef<void(bool bDodged)> OnHit)
	{
		const ACharacter* Character = Cast<ACharacter>(Target);
		float CapsuleRadius = 40.f, CapsuleHalf = 90.f;
		if (Character)
		{
			Character->GetCapsuleComponent()->GetScaledCapsuleSize(CapsuleRadius, CapsuleHalf);
		}
		for (int32 i = Shots.Num() - 1; i >= 0; --i)
		{
			FShot& Shot = Shots[i];
			AActor* Ball = Shot.Actor.Get();
			Shot.Life -= DeltaSeconds;
			if (!Ball || Shot.Life <= 0.f)
			{
				if (Ball) Ball->Destroy();
				Shots.RemoveAtSwap(i);
				continue;
			}
			const FVector Pos = Ball->GetActorLocation() + Shot.Velocity * DeltaSeconds;
			Ball->SetActorLocation(Pos);
			if (!Target || Shot.bPassed)
			{
				continue;
			}
			// 캡슐 축(세로 선분)까지의 거리로 명중 판정
			const FVector Center = Target->GetActorLocation();
			const FVector Closest = FMath::ClosestPointOnSegment(Pos, Center - FVector(0.f, 0.f, CapsuleHalf - CapsuleRadius), Center + FVector(0.f, 0.f, CapsuleHalf - CapsuleRadius));
			if (FVector::Dist(Pos, Closest) <= CapsuleRadius + Radius)
			{
				OnHit(bInvincible);
				if (bInvincible)
				{
					Shot.bPassed = true;   // 무적: 몸을 통과해 계속 날아간다
				}
				else
				{
					Ball->Destroy();
					Shots.RemoveAtSwap(i);
				}
			}
		}
	}

	void FSimpleProjectiles::Clear()
	{
		for (FShot& Shot : Shots)
		{
			if (AActor* Ball = Shot.Actor.Get()) Ball->Destroy();
		}
		Shots.Reset();
	}
}
