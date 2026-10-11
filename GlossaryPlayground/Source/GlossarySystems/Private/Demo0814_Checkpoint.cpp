#include "Demo0814_Checkpoint.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "D08_Common.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- 깃발

AD0814_CheckpointFlag::AD0814_CheckpointFlag()
{
	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	RootComponent = Trigger;
	Trigger->SetBoxExtent(FVector(120.f, 250.f, 150.f));
	Trigger->SetCollisionProfileName(TEXT("Trigger")); // Pawn 과 Overlap
}

void AD0814_CheckpointFlag::BeginPlay()
{
	Super::BeginPlay();
	AddShape(this, EShape::Cylinder, FVector(0.f, -220.f, 0.f), FVector(12.f, 12.f, 300.f), Colors::Neutral);
	Flag = AddShape(this, EShape::Cube, FVector(0.f, -170.f, 110.f), FVector(6.f, 100.f, 60.f), Colors::Neutral);
	for (UActorComponent* C : GetComponents())
	{
		if (UStaticMeshComponent* M = Cast<UStaticMeshComponent>(C))
		{
			M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AD0814_CheckpointFlag::OnOverlap);
}

void AD0814_CheckpointFlag::SetActive(bool bActive)
{
	SetColor(Flag, bActive ? Colors::Ally : Colors::Neutral, 0.5f, 0.f, bActive ? 2.f : 0.f);
}

void AD0814_CheckpointFlag::OnOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (Cast<APawn>(Other) && Cast<APawn>(Other)->IsPlayerControlled() && Demo.IsValid())
	{
		Demo->ReachCheckpoint(this);
	}
}

// ---------------------------------------------------------------- 용암

AD0814_KillZone::AD0814_KillZone()
{
	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	RootComponent = Trigger;
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
}

void AD0814_KillZone::SetSize(const FVector& Size)
{
	Trigger->SetBoxExtent(Size * 0.5f);
	if (Lava)
	{
		Lava->SetRelativeScale3D(FVector(Size.X, Size.Y, 4.f) / 100.f);
	}
}

void AD0814_KillZone::BeginPlay()
{
	Super::BeginPlay();
	Lava = AddShape(this, EShape::Cube, FVector::ZeroVector, FVector(100.f, 100.f, 4.f), FLinearColor(1.f, 0.25f, 0.02f));
	SetColor(Lava, FLinearColor(1.f, 0.25f, 0.02f), 0.3f, 0.f, 4.f);
	Lava->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AD0814_KillZone::OnOverlap);
}

void AD0814_KillZone::OnOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (Cast<APawn>(Other) && Cast<APawn>(Other)->IsPlayerControlled() && Demo.IsValid())
	{
		Demo->KillPlayer(TEXT("용암에 빠짐"));
	}
}

// ---------------------------------------------------------------- 데모

ADemo0814_Checkpoint::ADemo0814_Checkpoint()
{
	DemoId = TEXT("0814");
	// 플레이어 컨트롤러(입력 처리) 다음에 Tick 해서 WasKeyPressed 를 놓치지 않게
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	Controls = TEXT("WASD 이동 · Space 점프로 용암(주황)을 건넌다 · 깃발을 지나면 체크포인트 저장(초록)\n")
		TEXT("K : 즉사(테스트) · T : 다음 깃발로 순간이동(테스트)\n")
		TEXT("관찰: 죽으면 1초 뒤 마지막으로 지난 깃발에서 RestartPlayerAtTransform 으로 새 폰이 생긴다");
	Showcase = TEXT("깃발 Trigger 로 체크포인트 저장, 용암에 빠지면 GameMode::RestartPlayerAtTransform 으로 마지막 깃발에서 부활");
}

void ADemo0814_Checkpoint::BeginPlay()
{
	Super::BeginPlay();
	SavedTransform = PlayerStartTransform;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 길: 깃발 3개 사이사이에 용암 띠
	const float FlagX[] = { 0.f, 1000.f, 2000.f };
	for (int32 i = 0; i < 3; ++i)
	{
		AD0814_CheckpointFlag* Flag = GetWorld()->SpawnActor<AD0814_CheckpointFlag>(FVector(FlagX[i], 0.f, 150.f), FRotator::ZeroRotator, Params);
		Flag->Index = i;
		Flag->Demo = this;
		Flag->SetActive(false);
		Flags.Add(Flag);
		SpawnLabel(GetWorld(), FString::Printf(TEXT("체크포인트 %d"), i + 1), FVector(FlagX[i], -220.f, 360.f), Colors::Ally, 20.f);
	}
	const float LavaX[] = { 500.f, 1500.f };
	for (float X : LavaX)
	{
		AD0814_KillZone* Zone = GetWorld()->SpawnActor<AD0814_KillZone>(FVector(X, 0.f, 10.f), FRotator::ZeroRotator, Params);
		Zone->Demo = this;
		Zone->SetSize(FVector(220.f, 1200.f, 60.f));
		SpawnLabel(GetWorld(), TEXT("용암"), FVector(X, 0.f, 120.f), FLinearColor(1.f, 0.4f, 0.1f), 20.f);
	}
	// 양옆 벽 (용암을 돌아가지 못하게)
	SpawnBlock(GetWorld(), FVector(800.f, -650.f, 0.f), FVector(3200.f, 60.f, 250.f), Colors::Wall);
	SpawnBlock(GetWorld(), FVector(800.f, 650.f, 0.f), FVector(3200.f, 60.f, 250.f), Colors::Wall);

	Panel = UD08_SidePanel::Create(GetPC(), TEXT("체크포인트"), UD08_SidePanel::EPlace::TopRight, 420.f);
	LastEvent = TEXT("시작 지점 (PlayerStart) 저장됨");
}

void ADemo0814_Checkpoint::ReachCheckpoint(AD0814_CheckpointFlag* Flag)
{
	if (!Flag || Flag->Index <= SavedIndex)
	{
		return; // 이미 지난 깃발로 되돌아가지 않는다
	}
	SavedIndex = Flag->Index;
	// 저장: 깃발 위치, 진행 방향(+X)을 바라보게
	SavedTransform = FTransform(FRotator::ZeroRotator, Flag->GetActorLocation() + FVector(0.f, 0.f, -50.f));
	for (AD0814_CheckpointFlag* F : Flags)
	{
		F->SetActive(F->Index <= SavedIndex);
	}
	LastEvent = FString::Printf(TEXT("체크포인트 %d 저장"), SavedIndex + 1);
	AD08_FloatingText::Spawn(GetWorld(), TEXT("체크포인트!"), Flag->GetActorLocation() + FVector(0, 0, 200), Colors::Ally, 28.f, 1.5f);
}

void ADemo0814_Checkpoint::KillPlayer(const FString& Reason)
{
	APlayerController* PC = GetPC();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn || RespawnTimer > 0.f)
	{
		return;
	}
	++Deaths;
	LastEvent = FString::Printf(TEXT("사망: %s → %.1f초 뒤 부활"), *Reason, RespawnDelay);
	AD08_FloatingText::Spawn(GetWorld(), TEXT("으악!"), Pawn->GetActorLocation() + FVector(0, 0, 120), Colors::Enemy, 30.f);
	// 폰을 없앤다 (컨트롤러·PlayerState 는 남는다)
	PC->UnPossess();
	Pawn->Destroy();
	RespawnTimer = RespawnDelay;
}

void ADemo0814_Checkpoint::Respawn()
{
	APlayerController* PC = GetPC();
	AGameModeBase* GM = GetWorld()->GetAuthGameMode();
	if (!PC || !GM)
	{
		return;
	}
	// 핵심: 저장해 둔 위치에서 기본 폰을 새로 스폰하고 빙의시킨다
	GM->RestartPlayerAtTransform(PC, SavedTransform);
	PC->SetControlRotation(SavedTransform.Rotator());
	LastEvent = FString::Printf(TEXT("부활: %s"), SavedIndex < 0 ? TEXT("시작 지점") : *FString::Printf(TEXT("체크포인트 %d"), SavedIndex + 1));
}

void ADemo0814_Checkpoint::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (RespawnTimer > 0.f)
	{
		RespawnTimer -= DeltaSeconds;
		if (RespawnTimer <= 0.f)
		{
			RespawnTimer = 0.f;
			Respawn();
		}
	}
	if (WasKeyPressed(EKeys::K))
	{
		KillPlayer(TEXT("K 키"));
	}
	if (WasKeyPressed(EKeys::T))
	{
		// 테스트: 다음 깃발의 Trigger 안으로 순간이동 → Overlap 이벤트로 저장된다
		const int32 Next = FMath::Min(SavedIndex + 1, Flags.Num() - 1);
		if (APawn* Pawn = GetPlayerPawn())
		{
			Pawn->SetActorLocation(Flags[Next]->GetActorLocation() + FVector(0.f, 0.f, -50.f), false, nullptr, ETeleportType::TeleportPhysics);
		}
	}

	const FVector Saved = SavedTransform.GetLocation();
	if (Panel)
	{
		Panel->SetTitle(SavedIndex < 0 ? TEXT("저장: 시작 지점") : FString::Printf(TEXT("저장: 체크포인트 %d"), SavedIndex + 1));
		Panel->SetBody(FString::Printf(TEXT("저장 위치 (%.0f, %.0f, %.0f)\n사망 %d회%s\nGameMode->RestartPlayerAtTransform(PC, Saved)"),
			Saved.X, Saved.Y, Saved.Z, Deaths, RespawnTimer > 0.f ? *FString::Printf(TEXT(" · 부활까지 %.1f초"), RespawnTimer) : TEXT("")));
	}
	SetStatus(LastEvent);
}
