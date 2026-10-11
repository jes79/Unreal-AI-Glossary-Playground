#include "D0102_InputCharacter.h"

#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "MovementKit.h"

AD0102_InputCharacter::AD0102_InputCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AD0102_InputCharacter::BuildInputAssets()
{
	if (IMC_Default)
	{
		return;
	}
	using namespace MovementKit;

	// ---- Input Action: "무엇을 할지" (값의 종류만 정한다. 어떤 키인지는 모른다) ----
	IA_Move = MakeAction(this, TEXT("IA_Move"), EInputActionValueType::Axis2D);
	IA_Jump = MakeAction(this, TEXT("IA_Jump"), EInputActionValueType::Boolean);
	IA_Attack = MakeAction(this, TEXT("IA_Attack"), EInputActionValueType::Boolean);
	IA_HeavyAttack = MakeAction(this, TEXT("IA_HeavyAttack"), EInputActionValueType::Boolean);

	// ---- Input Mapping Context: "어떤 키가 어떤 액션인지" ----
	IMC_Default = MakeContext(this, TEXT("IMC_Default"));

	// 이동: 키보드 WASD (Negate/Swizzle 모디파이어) + 게임패드 왼쪽 스틱 (Dead Zone)
	MapWASD(IMC_Default, IA_Move);
	MapStick(IMC_Default, IA_Move, EKeys::Gamepad_Left2D);

	// 점프: Space + 패드 A(아래 버튼)
	IMC_Default->MapKey(IA_Jump, EKeys::SpaceBar);
	IMC_Default->MapKey(IA_Jump, EKeys::Gamepad_FaceButton_Bottom);

	// 공격: 좌클릭 + 패드 X(왼쪽 버튼), Pressed 트리거 = 누른 순간 한 번만 Triggered
	for (const FKey& Key : { EKeys::LeftMouseButton, EKeys::Gamepad_FaceButton_Left })
	{
		FEnhancedActionKeyMapping& Mapping = IMC_Default->MapKey(IA_Attack, Key);
		Mapping.Triggers.Add(NewObject<UInputTriggerPressed>(IMC_Default));
	}

	// 강공격: 우클릭 + 패드 RT, Hold 트리거 = 0.5초 누르고 있어야 Triggered (그 전에 떼면 Canceled)
	for (const FKey& Key : { EKeys::RightMouseButton, EKeys::Gamepad_RightTrigger })
	{
		FEnhancedActionKeyMapping& Mapping = IMC_Default->MapKey(IA_HeavyAttack, Key);
		UInputTriggerHold* Hold = NewObject<UInputTriggerHold>(IMC_Default);
		Hold->HoldTimeThreshold = HeavyAttackHoldTime;
		Hold->bIsOneShot = true;
		Mapping.Triggers.Add(Hold);
	}

	// 시점: 마우스는 템플릿 IMC_MouseLook 을 그대로 쓰고, 패드 오른쪽 스틱만 템플릿 IA_Look 에 연결
	if (LookAction)
	{
		MapStick(IMC_Default, LookAction, EKeys::Gamepad_Right2D);
	}
}

void AD0102_InputCharacter::NotifyControllerChanged()
{
	// 부모(AGlossaryCharacter)는 템플릿 IMC_Default 를 등록한다. 이 데모는 직접 만든 IMC 를 쓰므로 한 단계 건너뛴다.
	ACharacter::NotifyControllerChanged();
	BuildInputAssets();
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = MovementKit::GetInputSubsystem(Cast<APlayerController>(Controller)))
	{
		Subsystem->AddMappingContext(IMC_Default, 0);
		if (MouseLookMappingContext)
		{
			Subsystem->AddMappingContext(MouseLookMappingContext, 0);
		}
	}
}

void AD0102_InputCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// 부모는 템플릿 액션(IA_Look/IA_MouseLook 등)을 바인딩 — 시점 회전만 그대로 쓴다
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	BuildInputAssets();

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		return;
	}

	// 이동: Triggered 는 누르고 있는 동안 매 프레임 — 로그 대신 횟수만 센다
	Input->BindActionInstanceLambda(IA_Move, ETriggerEvent::Started, [this](const FInputActionInstance& I) { LogEvent(TEXT("IA_Move"), ETriggerEvent::Started, I); });
	Input->BindActionInstanceLambda(IA_Move, ETriggerEvent::Completed, [this](const FInputActionInstance& I) { LastMove = FVector2D::ZeroVector; LogEvent(TEXT("IA_Move"), ETriggerEvent::Completed, I); });
	Input->BindActionInstanceLambda(IA_Move, ETriggerEvent::Triggered, [this](const FInputActionInstance& I)
	{
		LastMove = I.GetValue().Get<FVector2D>();
		++MoveTriggeredCount;
		DoMove(LastMove.X, LastMove.Y);
	});

	// 점프: Started 에서 뛰고 Completed 에서 멈춘다 (템플릿과 같은 연결)
	Input->BindActionInstanceLambda(IA_Jump, ETriggerEvent::Started, [this](const FInputActionInstance& I) { DoJumpStart(); LogEvent(TEXT("IA_Jump"), ETriggerEvent::Started, I); });
	Input->BindActionInstanceLambda(IA_Jump, ETriggerEvent::Completed, [this](const FInputActionInstance& I) { DoJumpEnd(); LogEvent(TEXT("IA_Jump"), ETriggerEvent::Completed, I); });

	// 공격 (Pressed)
	Input->BindActionInstanceLambda(IA_Attack, ETriggerEvent::Triggered, [this](const FInputActionInstance& I) { SpawnAttackFlash(false); LogEvent(TEXT("IA_Attack"), ETriggerEvent::Triggered, I); });
	Input->BindActionInstanceLambda(IA_Attack, ETriggerEvent::Completed, [this](const FInputActionInstance& I) { LogEvent(TEXT("IA_Attack"), ETriggerEvent::Completed, I); });

	// 강공격 (Hold 0.5초): Started → Ongoing… → Triggered, 일찍 떼면 Canceled
	Input->BindActionInstanceLambda(IA_HeavyAttack, ETriggerEvent::Started, [this](const FInputActionInstance& I) { LogEvent(TEXT("IA_HeavyAttack"), ETriggerEvent::Started, I); });
	Input->BindActionInstanceLambda(IA_HeavyAttack, ETriggerEvent::Ongoing, [this](const FInputActionInstance& I) { HeavyHoldTime = I.GetElapsedTime(); });
	Input->BindActionInstanceLambda(IA_HeavyAttack, ETriggerEvent::Triggered, [this](const FInputActionInstance& I) { HeavyHoldTime = 0.f; SpawnAttackFlash(true); LogEvent(TEXT("IA_HeavyAttack"), ETriggerEvent::Triggered, I); });
	Input->BindActionInstanceLambda(IA_HeavyAttack, ETriggerEvent::Canceled, [this](const FInputActionInstance& I) { HeavyHoldTime = 0.f; LogEvent(TEXT("IA_HeavyAttack"), ETriggerEvent::Canceled, I); });
	Input->BindActionInstanceLambda(IA_HeavyAttack, ETriggerEvent::Completed, [this](const FInputActionInstance& I) { HeavyHoldTime = 0.f; LogEvent(TEXT("IA_HeavyAttack"), ETriggerEvent::Completed, I); });
}

void AD0102_InputCharacter::LogEvent(const TCHAR* ActionName, ETriggerEvent Event, const FInputActionInstance& Instance)
{
	const TCHAR* EventName = TEXT("?");
	switch (Event)
	{
	case ETriggerEvent::Started:   EventName = TEXT("Started"); break;
	case ETriggerEvent::Ongoing:   EventName = TEXT("Ongoing"); break;
	case ETriggerEvent::Triggered: EventName = TEXT("Triggered"); break;
	case ETriggerEvent::Completed: EventName = TEXT("Completed"); break;
	case ETriggerEvent::Canceled:  EventName = TEXT("Canceled"); break;
	default: break;
	}
	const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	EventLog.Insert(FString::Printf(TEXT("%6.2f  %-15s %-9s 값 %s · 경과 %.2fs"),
		Time, ActionName, EventName, *Instance.GetValue().ToString(), Instance.GetElapsedTime()), 0);
	if (EventLog.Num() > 12)
	{
		EventLog.SetNum(12);
	}
}

void AD0102_InputCharacter::SpawnAttackFlash(bool bHeavy)
{
	// 공격이 나갔다는 것을 보이기 위한 짧은 섬광 (몸 앞의 구)
	const float Size = bHeavy ? 220.f : 90.f;
	const FVector Where = GetActorLocation() + GetActorForwardVector() * (bHeavy ? 160.f : 110.f) + FVector(0.f, 0.f, 20.f);
	if (AStaticMeshActor* Flash = GlossaryKit::SpawnShape(GetWorld(), GlossaryKit::EShape::Sphere, Where, FVector(Size),
		bHeavy ? GlossaryKit::Colors::Enemy : GlossaryKit::Colors::Accent))
	{
		Flash->SetActorEnableCollision(false);
		GlossaryKit::SetColor(Flash->GetStaticMeshComponent(), bHeavy ? GlossaryKit::Colors::Enemy : GlossaryKit::Colors::Accent, 0.4f, 0.f, 4.f);
		Flash->SetLifeSpan(0.2f);
	}
}
