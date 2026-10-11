#include "Demo0204_ShapeTrace.h"

#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"

using namespace GlossaryKit;

namespace D0204Local
{
	// 플레이어 시작(-600,0) 기준 앞쪽에 선 적들: 정면 / 옆 70 / 옆 -75 / 옆 160(범위 밖) / 멀리
	const FVector EnemySpots[] = {
		FVector(-430.f, 0.f, 90.f), FVector(-450.f, 70.f, 90.f), FVector(-420.f, -75.f, 90.f),
		FVector(-470.f, 165.f, 90.f), FVector(-230.f, 0.f, 90.f) };
}
using namespace D0204Local;

ADemo0204_ShapeTrace::ADemo0204_ShapeTrace()
{
	DemoId = TEXT("0204");
	Controls = TEXT("WASD 이동 · E 또는 좌클릭 : 근접 공격 (SphereTraceMulti 반지름 50)\n")
		TEXT("1 : 구(반지름 50) ↔ 선(Line Trace) 비교 · R : 적 체력 초기화\n")
		TEXT("관찰: 구는 두께가 있어 정면+양옆 적 3명을 한 번에 맞히고, 선은 정면 1명만 맞힌다");
	Showcase = TEXT("SphereTraceMulti 반지름 50 근접 판정으로 범위 안 적을 모두 타격, Line Trace 와 맞는 수 비교");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-600.f, 0.f, 100.f));
}

void ADemo0204_ShapeTrace::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	for (const FVector& Spot : EnemySpots)
	{
		AStaticMeshActor* Enemy = SpawnShape(World, EShape::Cylinder, Spot, FVector(60.f, 60.f, 180.f), Colors::Enemy);
		UStaticMeshComponent* Mesh = Enemy->GetStaticMeshComponent();
		// 트레이스 채널(Visibility)에 Overlap 으로 응답해야 Multi 트레이스가 막히지 않고 전부 돌려준다
		Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
		Enemies.Add(Enemy);
		EnemyLabels.Add(SpawnLabel(World, TEXT("HP 100"), Spot + FVector(0.f, 0.f, 140.f), FLinearColor::White, 14.f));
		Health.Add(100.f);
		FlashTime.Add(0.f);
		HomeLocations.Add(Spot);
	}
}

void ADemo0204_ShapeTrace::ResetEnemies()
{
	for (int32 i = 0; i < Enemies.Num(); ++i)
	{
		Health[i] = 100.f;
		Enemies[i]->SetActorLocation(HomeLocations[i]);
		EnemyLabels[i]->SetText(TEXT("HP 100"));
	}
	LastHitCount = -1;
}

void ADemo0204_ShapeTrace::Attack()
{
	APawn* Pawn = GetPlayerPawn();
	if (!Pawn)
	{
		return;
	}
	const FVector Forward = Pawn->GetActorForwardVector();
	const FVector Start = Pawn->GetActorLocation() + Forward * StartOffset + FVector(0.f, 0.f, 20.f);
	const FVector End = Pawn->GetActorLocation() + Forward * AttackRange + FVector(0.f, 0.f, 20.f);
	const TArray<AActor*> Ignore = { Pawn };
	TArray<FHitResult> Hits;

	// 핵심: 구를 Start→End 로 쓸어서(Sweep) 겹친 것을 모두 받는다. DrawDebugTrace 로 1초간 표시
	if (bLineMode)
	{
		UKismetSystemLibrary::LineTraceMulti(this, Start, End, UEngineTypes::ConvertToTraceType(ECC_Visibility), false, Ignore,
			EDrawDebugTrace::ForDuration, Hits, true, FLinearColor::Red, FLinearColor::Green, 1.f);
	}
	else
	{
		UKismetSystemLibrary::SphereTraceMulti(this, Start, End, SphereRadius, UEngineTypes::ConvertToTraceType(ECC_Visibility), false, Ignore,
			EDrawDebugTrace::ForDuration, Hits, true, FLinearColor::Red, FLinearColor::Green, 1.f);
	}

	// 같은 적이 여러 번 들어올 수 있으므로 액터 단위로 한 번만 처리
	TSet<AActor*> Processed;
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		const int32 Index = Enemies.IndexOfByKey(HitActor);
		if (Index == INDEX_NONE || Processed.Contains(HitActor))
		{
			continue;
		}
		Processed.Add(HitActor);
		Health[Index] = FMath::Max(0.f, Health[Index] - Damage);
		FlashTime[Index] = 0.25f;
		SetColor(Enemies[Index]->GetStaticMeshComponent(), FLinearColor::White, 0.5f, 0.f, 3.f);
		// 넉백: 플레이어 반대 방향으로 30cm
		FVector Push = HitActor->GetActorLocation() - Pawn->GetActorLocation();
		Push.Z = 0.f;
		HitActor->AddActorWorldOffset(Push.GetSafeNormal() * 30.f, true);
		EnemyLabels[Index]->SetText(FString::Printf(TEXT("HP %.0f"), Health[Index]));
	}
	LastHitCount = Processed.Num();
}

void ADemo0204_ShapeTrace::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 시작 시점: 캐릭터 뒤 위에서 적들을 내려다보게
	if (!bViewInitialized && GetPC() && GetPlayerPawn())
	{
		GetPC()->SetControlRotation(FRotator(-25.f, 0.f, 0.f));
		bViewInitialized = true;
	}
	if (WasKeyPressed(EKeys::One)) { bLineMode = !bLineMode; LastHitCount = -1; }
	if (WasKeyPressed(EKeys::R)) { ResetEnemies(); }
	AttackCooldown -= DeltaSeconds;
	if ((WasKeyPressed(EKeys::E) || WasKeyPressed(EKeys::LeftMouseButton)) && AttackCooldown <= 0.f)
	{
		AttackCooldown = 0.25f;
		Attack();
	}

	for (int32 i = 0; i < Enemies.Num(); ++i)
	{
		if (FlashTime[i] > 0.f)
		{
			FlashTime[i] -= DeltaSeconds;
			if (FlashTime[i] <= 0.f)
			{
				SetColor(Enemies[i]->GetStaticMeshComponent(), Health[i] > 0.f ? Colors::Enemy : Colors::Dark);
			}
		}
		EnemyLabels[i]->SetActorLocation(Enemies[i]->GetActorLocation() + FVector(0.f, 0.f, 140.f));
	}

	// 공격 범위 미리보기 (얇은 선)
	if (APawn* Pawn = GetPlayerPawn())
	{
		const FVector Forward = Pawn->GetActorForwardVector();
		const FVector Start = Pawn->GetActorLocation() + Forward * StartOffset + FVector(0.f, 0.f, 20.f);
		const FVector End = Pawn->GetActorLocation() + Forward * AttackRange + FVector(0.f, 0.f, 20.f);
		if (bLineMode)
		{
			DrawDebugLine(GetWorld(), Start, End, FColor(255, 255, 255, 80), false, -1.f, 0, 0.5f);
		}
		else
		{
			DrawDebugCapsule(GetWorld(), (Start + End) * 0.5f, (End - Start).Size() * 0.5f + SphereRadius, SphereRadius,
				FRotationMatrix::MakeFromZ(End - Start).ToQuat(), FColor(255, 255, 255, 80));
		}
	}

	SetStatus(FString::Printf(TEXT("판정: %s · 길이 %.0f~%.0fcm\n%s"),
		bLineMode ? TEXT("LineTraceMulti (두께 0)") : *FString::Printf(TEXT("SphereTraceMulti 반지름 %.0f"), SphereRadius),
		StartOffset, AttackRange,
		LastHitCount < 0 ? TEXT("E 로 공격해 보세요") : *FString::Printf(TEXT("마지막 공격: 적 %d명 타격 (데미지 %.0f)"), LastHitCount, Damage)));
}
