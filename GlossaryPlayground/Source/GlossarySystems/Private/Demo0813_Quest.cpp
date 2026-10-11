#include "Demo0813_Quest.h"

#include "D08_Common.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0813_Quest::ADemo0813_Quest()
{
	DemoId = TEXT("0813");
	Controls = TEXT("WASD 이동 · 슬라임(초록)·박쥐(보라)에 닿으면 처치 · K : 가장 가까운 슬라임 처치(테스트) · R : 퀘스트 초기화\n")
		TEXT("관찰: 오른쪽 위 퀘스트 HUD 가 슬라임만 세고, 5마리가 되면 완료 + 보상 골드가 들어온다");
	Showcase = TEXT("'슬라임 5마리 처치' 퀘스트 Data Asset, HUD 진행도 바, 완료 시 골드 보상 (박쥐는 집계 제외)");
	Quest = TSoftObjectPtr<UD0813_QuestData>(FSoftObjectPath(TEXT("/Game/Glossary/08_Systems/DA_0813_SlimeHunt.DA_0813_SlimeHunt")));
}

void ADemo0813_Quest::BeginPlay()
{
	Super::BeginPlay();
	LoadedQuest = Quest.LoadSynchronous();
	Hud = UD08_SidePanel::Create(GetPC(), TEXT("퀘스트"), UD08_SidePanel::EPlace::TopRight, 420.f);
	if (!LoadedQuest)
	{
		SetStatus(TEXT("DA_0813_SlimeHunt 가 없다 — tools/glossary-build.sh -filter=08"));
		return;
	}
	SpawnMobs();
	LastEvent = FString::Printf(TEXT("퀘스트 수락: %s"), *LoadedQuest->Title);
	UpdateHud();
}

void ADemo0813_Quest::SpawnMobs()
{
	for (FMob& Mob : Mobs)
	{
		if (AActor* A = Mob.Actor.Get())
		{
			A->Destroy();
		}
	}
	Mobs.Reset();
	// 슬라임 7, 박쥐 3
	for (int32 i = 0; i < 10; ++i)
	{
		const bool bSlime = i < 7;
		const float Angle = 2.f * PI * i / 10.f + 0.3f;
		const FVector Home(350.f + FMath::Cos(Angle) * 650.f, FMath::Sin(Angle) * 650.f, bSlime ? 35.f : 180.f);
		AStaticMeshActor* A = bSlime
			? SpawnShape(GetWorld(), EShape::Sphere, Home, FVector(110.f, 110.f, 70.f), Colors::Ally)
			: SpawnShape(GetWorld(), EShape::Cone, Home, FVector(60.f, 60.f, 60.f), Colors::Purple, FRotator(180.f, 0.f, 0.f));
		A->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetColor(A->GetStaticMeshComponent(), bSlime ? Colors::Ally : Colors::Purple, 0.2f, 0.f, 0.3f);
		FMob Mob;
		Mob.Actor = A;
		Mob.TypeId = bSlime ? FName(TEXT("Slime")) : FName(TEXT("Bat"));
		Mob.Home = Home;
		Mob.Phase = i * 0.7f;
		Mobs.Add(Mob);
	}
}

void ADemo0813_Quest::Kill(int32 Index)
{
	FMob Mob = Mobs[Index];
	Mobs.RemoveAt(Index);
	if (AActor* A = Mob.Actor.Get())
	{
		AD08_FloatingText::Spawn(GetWorld(), Mob.TypeId == TEXT("Slime") ? TEXT("슬라임 처치") : TEXT("박쥐 처치"), A->GetActorLocation() + FVector(0, 0, 80), FLinearColor::White, 20.f);
		A->Destroy();
	}
	OnEnemyKilled(Mob.TypeId);
}

void ADemo0813_Quest::OnEnemyKilled(FName TypeId)
{
	// 퀘스트 조건: 데이터에 적힌 TargetType 과 같은 적만 센다
	if (!LoadedQuest || bCompleted)
	{
		return;
	}
	if (TypeId != LoadedQuest->TargetType)
	{
		LastEvent = FString::Printf(TEXT("%s 처치 — 퀘스트 대상(%s)이 아니라 집계 안 함"), *TypeId.ToString(), *LoadedQuest->TargetType.ToString());
		return;
	}
	Progress = FMath::Min(Progress + 1, LoadedQuest->RequiredCount);
	LastEvent = FString::Printf(TEXT("슬라임 처치 %d / %d"), Progress, LoadedQuest->RequiredCount);
	if (Progress >= LoadedQuest->RequiredCount)
	{
		// 완료 → 보상 지급
		bCompleted = true;
		Gold += LoadedQuest->RewardGold;
		LastEvent = FString::Printf(TEXT("퀘스트 완료! 보상 골드 +%d, %s"), LoadedQuest->RewardGold, *LoadedQuest->RewardItem);
		if (APawn* Pawn = GetPlayerPawn())
		{
			AD08_FloatingText::Spawn(GetWorld(), FString::Printf(TEXT("퀘스트 완료! +%d G"), LoadedQuest->RewardGold),
				Pawn->GetActorLocation() + FVector(0, 0, 150), Colors::Item, 30.f, 2.5f);
		}
	}
	UpdateHud();
}

void ADemo0813_Quest::UpdateHud()
{
	if (!Hud || !LoadedQuest)
	{
		return;
	}
	Hud->SetTitle(bCompleted ? FString::Printf(TEXT("[완료] %s"), *LoadedQuest->Title) : LoadedQuest->Title);
	Hud->SetProgress(float(Progress) / LoadedQuest->RequiredCount, bCompleted ? Colors::Item : Colors::Ally);
	Hud->SetBody(FString::Printf(TEXT("%s\n슬라임 처치 %d / %d\n보상: 골드 %d · %s\n\n보유 골드 %d"),
		*LoadedQuest->Description, Progress, LoadedQuest->RequiredCount, LoadedQuest->RewardGold, *LoadedQuest->RewardItem, Gold));
}

void ADemo0813_Quest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!LoadedQuest)
	{
		return;
	}
	Time += DeltaSeconds;
	if (WasKeyPressed(EKeys::R))
	{
		Progress = 0;
		bCompleted = false;
		SpawnMobs();
		LastEvent = TEXT("퀘스트 초기화");
		UpdateHud();
	}

	APawn* Pawn = GetPlayerPawn();
	int32 NearestSlime = INDEX_NONE;
	float NearestDist = 1e9f;
	for (int32 i = Mobs.Num() - 1; i >= 0; --i)
	{
		AActor* A = Mobs[i].Actor.Get();
		if (!A)
		{
			continue;
		}
		// 슬라임은 통통 튀고, 박쥐는 떠서 원을 그린다
		const float P = Time * 2.f + Mobs[i].Phase;
		const FVector Offset = Mobs[i].TypeId == TEXT("Slime")
			? FVector(FMath::Cos(P * 0.3f) * 60.f, FMath::Sin(P * 0.3f) * 60.f, FMath::Abs(FMath::Sin(P * 2.f)) * 40.f)
			: FVector(FMath::Cos(P) * 80.f, FMath::Sin(P) * 80.f, FMath::Sin(P * 3.f) * 20.f);
		A->SetActorLocation(Mobs[i].Home + Offset);
		if (Pawn)
		{
			const float Dist = FVector::Dist2D(A->GetActorLocation(), Pawn->GetActorLocation());
			if (Dist < StompRadius)
			{
				Kill(i);
				continue;
			}
			if (Mobs[i].TypeId == TEXT("Slime") && Dist < NearestDist)
			{
				NearestDist = Dist;
				NearestSlime = i;
			}
		}
	}
	if (WasKeyPressed(EKeys::K) && NearestSlime != INDEX_NONE && Mobs.IsValidIndex(NearestSlime))
	{
		Kill(NearestSlime);
	}
	SetStatus(FString::Printf(TEXT("%s · 진행 %d/%d · 골드 %d\n%s"), *LoadedQuest->GetName(), Progress, LoadedQuest->RequiredCount, Gold, *LastEvent));
}
