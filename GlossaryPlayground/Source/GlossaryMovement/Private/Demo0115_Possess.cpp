#include "Demo0115_Possess.h"

#include "D0115_Drone.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"
#include "MovementKit.h"
#include "MovementSidePanel.h"

using namespace GlossaryKit;

ADemo0115_Possess::ADemo0115_Possess()
{
	DemoId = TEXT("0115");
	Controls = TEXT("캐릭터: WASD · Space · 드론 근처에서 E = 드론에 Possess\n")
		TEXT("드론: WASD 수평 · Space 상승 · Ctrl 하강 · 마우스 방향 · E = 캐릭터로 돌아오기\n")
		TEXT("관찰: 같은 PlayerController 가 폰만 바꿔 조종한다. 빙의할 때 NotifyControllerChanged 가 매핑 컨텍스트를 다시 등록");
	Showcase = TEXT("E 로 근처 드론 Pawn 에 Possess(비행·자기 카메라·IMC_Drone), 다시 E 로 캐릭터 복귀, 현재 폰·컨텍스트 표시");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-500.f, 0.f, 100.f));
}

void ADemo0115_Possess::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	Character = GetPlayerPawn();

	// 드론 착륙대 + 드론
	SpawnBlock(World, FVector(0.f, 250.f, 0.f), FVector(260.f, 260.f, 40.f), Colors::Dark);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Drone = World->SpawnActor<AD0115_Drone>(FVector(0.f, 250.f, 120.f), FRotator::ZeroRotator, Params);
	Prompt = SpawnLabel(World, TEXT("E : 드론에 Possess"), FVector(0.f, 250.f, 260.f), Colors::Item, 20.f);

	// 드론으로만 갈 수 있는 높은 탑들
	for (int32 i = 0; i < 4; ++i)
	{
		const FVector P(900.f + i * 500.f, -400.f + (i % 2) * 800.f, 0.f);
		SpawnBlock(World, P, FVector(200.f, 200.f, 400.f + i * 250.f), i % 2 ? Colors::Cyan : Colors::Accent);
		SpawnShape(World, EShape::Sphere, P + FVector(0.f, 0.f, 400.f + i * 250.f + 60.f), FVector(80.f), Colors::Item);
	}
	SpawnLabel(World, TEXT("드론으로 탑 꼭대기를 둘러보세요"), FVector(1500.f, 0.f, 900.f), FLinearColor::White, 18.f);

	Panel = UMovementSidePanel::Create(GetPC(), TEXT("Possess 상태"), 440.f);
}

void ADemo0115_Possess::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Panel)
	{
		Panel->RemoveFromParent();
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0115_Possess::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APlayerController* PC = GetPC();
	if (!PC || !Drone || !Character)
	{
		return;
	}
	APawn* Current = PC->GetPawn();
	const bool bInDrone = Current == Drone;
	const bool bNear = FVector::Dist(Character->GetActorLocation(), Drone->GetActorLocation()) <= PossessDistance;

	if (WasKeyPressed(EKeys::E))
	{
		if (!bInDrone && bNear)
		{
			// ★ 핵심: 컨트롤러가 다른 폰을 조종 — 이전 폰은 UnPossessed, 새 폰은 PossessedBy → NotifyControllerChanged
			PC->Possess(Drone);
			LastEvent = TEXT("PC->Possess(Drone)");
			++SwitchCount;
		}
		else if (bInDrone)
		{
			PC->Possess(Character);
			LastEvent = TEXT("PC->Possess(Character)");
			++SwitchCount;
		}
		else
		{
			LastEvent = TEXT("너무 멀다 — 드론 가까이에서 E");
		}
	}

	if (AGlossaryLabel* L = Prompt.Get())
	{
		L->SetActorHiddenInGame(bInDrone || !bNear);
		L->SetActorLocation(Drone->GetActorLocation() + FVector(0.f, 0.f, 140.f));
	}

	if (Panel)
	{
		const FString Contexts = MovementKit::DescribeContexts(PC, {
			{ TEXT("IMC_Drone (드론, 우선순위 1)"), Drone->GetDroneContext() },
			{ TEXT("IMC_Default (캐릭터 템플릿)"), MovementKit::LoadTemplateDefaultContext() },
			{ TEXT("IMC_MouseLook (템플릿)"), MovementKit::LoadTemplateMouseLookContext() },
		});
		Panel->SetBody(FString::Printf(TEXT("PlayerController: %s\n조종 중인 Pawn: %s\n캐릭터의 Controller: %s\n드론의 Controller: %s\n\n%s\n\n마지막: %s"),
			*PC->GetName(),
			bInDrone ? TEXT("드론 (AD0115_Drone)") : TEXT("캐릭터 (AGlossaryCharacter)"),
			Character->GetController() ? *Character->GetController()->GetName() : TEXT("없음 (빈 몸)"),
			Drone->GetController() ? *Drone->GetController()->GetName() : TEXT("없음"),
			*Contexts, LastEvent.IsEmpty() ? TEXT("-") : *LastEvent));
	}

	SetStatus(FString::Printf(TEXT("%s · 전환 %d회\n%s"),
		bInDrone ? TEXT("드론 조종 중") : TEXT("캐릭터 조종 중"), SwitchCount,
		bInDrone ? *FString::Printf(TEXT("드론 고도 %.0fcm · 속도 %.0f cm/s · E 로 복귀"), Drone->GetActorLocation().Z, Drone->GetVelocity().Size())
		         : (bNear ? TEXT("드론 옆: E 를 누르면 빙의") : TEXT("오른쪽 앞 주황 드론으로 가세요"))));
}
