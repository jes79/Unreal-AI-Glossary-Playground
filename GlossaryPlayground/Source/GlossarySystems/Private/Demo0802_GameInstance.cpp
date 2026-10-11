#include "Demo0802_GameInstance.h"

#include "D08_Common.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GlossaryKit.h"
#include "Kismet/GameplayStatics.h"

using namespace GlossaryKit;

DEFINE_LOG_CATEGORY_STATIC(LogGlossary0802, Log, All);

void UD0802_CoinKeeper::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CreatedAt = FPlatformTime::Seconds();
	// GameInstance 가 만들어질 때 딱 한 번 (레벨을 열 때마다가 아님)
	UE_LOG(LogGlossary0802, Display, TEXT("[0802] GameInstanceSubsystem 생성 (게임 실행 중 한 번)"));
}

ADemo0802_GameInstance::ADemo0802_GameInstance()
{
	DemoId = TEXT("0802");
	Controls = TEXT("WASD 이동 · 코인에 닿으면 획득 · C : 가장 가까운 코인 줍기(테스트)\n")
		TEXT("N : 다음 스테이지 (OpenLevel 로 레벨을 새로 연다)\n")
		TEXT("관찰: 레벨을 다시 열면 액터 변수는 0, GameInstance 의 코인은 그대로");
	Showcase = TEXT("코인을 모은 뒤 레벨을 다시 열면 액터 변수는 0, GameInstance(서브시스템) 코인은 유지");
	Implementation = EGlossaryImplementation::Alternative;
	AlternativeNote = TEXT("GameInstance 클래스는 프로젝트 전체에 하나라서, 같은 수명의 GameInstanceSubsystem 에 코인을 저장");
}

UD0802_CoinKeeper* ADemo0802_GameInstance::GetKeeper() const
{
	// GameInstance 는 레벨이 바뀌어도 같은 객체 — 어디서든 GetGameInstance() 로 얻는다
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UD0802_CoinKeeper>() : nullptr;
}

void ADemo0802_GameInstance::BeginPlay()
{
	Super::BeginPlay();
	UD0802_CoinKeeper* Keeper = GetKeeper();
	if (Keeper)
	{
		++Keeper->LevelLoads;
		UE_LOG(LogGlossary0802, Display, TEXT("[0802] 레벨 로드 #%d · 스테이지 %d · GameInstance 코인 %d · 액터 코인 %d"),
			Keeper->LevelLoads, Keeper->Stage, Keeper->Coins, ActorCoins);
	}
	const int32 Stage = Keeper ? Keeper->Stage : 1;

	// 스테이지마다 기둥 색이 바뀌어 '새 레벨'임을 눈으로 알 수 있게
	const FLinearColor StageColors[] = { Colors::Cyan, Colors::Purple, Colors::Accent, Colors::Ally };
	const FLinearColor StageColor = StageColors[(Stage - 1) % 4];
	for (int32 i = 0; i < 4; ++i)
	{
		SpawnBlock(GetWorld(), FVector(1100.f, -900.f + i * 600.f, 0.f), FVector(150.f, 150.f, 400.f + i * 80.f), StageColor);
	}
	SpawnLabel(GetWorld(), FString::Printf(TEXT("스테이지 %d"), Stage), FVector(1100.f, 0.f, 700.f), StageColor, 40.f);

	for (int32 i = 0; i < CoinsPerStage; ++i)
	{
		const float Angle = 2.f * PI * i / CoinsPerStage;
		const FVector Location(300.f + FMath::Cos(Angle) * 450.f, FMath::Sin(Angle) * 450.f, 70.f);
		AStaticMeshActor* Coin = SpawnShape(GetWorld(), EShape::Cylinder, Location, FVector(60.f, 60.f, 10.f), Colors::Item, FRotator(90.f, 0.f, 0.f));
		Coin->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetColor(Coin->GetStaticMeshComponent(), Colors::Item, 0.3f, 0.8f, 0.6f);
		Coins.Add(Coin);
	}
	// 다음 스테이지로 가는 문(시각용)
	SpawnBlock(GetWorld(), FVector(300.f, 0.f, 0.f), FVector(60.f, 260.f, 20.f), Colors::Purple);
	SpawnLabel(GetWorld(), TEXT("N : 다음 스테이지"), FVector(300.f, 0.f, 120.f), Colors::Purple, 20.f);

	Panel = UD08_SidePanel::Create(GetPC(), TEXT("코인 비교"), UD08_SidePanel::EPlace::TopRight, 460.f);
}

void ADemo0802_GameInstance::Collect(AActor* Coin)
{
	Coins.Remove(Coin);
	AD08_FloatingText::Spawn(GetWorld(), TEXT("+1"), Coin->GetActorLocation() + FVector(0, 0, 60), Colors::Item, 26.f);
	Coin->Destroy();
	++ActorCoins; // 레벨 액터 변수 (레벨과 함께 사라짐)
	if (UD0802_CoinKeeper* Keeper = GetKeeper())
	{
		++Keeper->Coins; // GameInstance 쪽 (레벨이 바뀌어도 남음)
	}
}

void ADemo0802_GameInstance::GoToNextStage()
{
	if (UD0802_CoinKeeper* Keeper = GetKeeper())
	{
		++Keeper->Stage;
	}
	// 같은 레벨을 처음부터 다시 연다 → 이 액터를 포함한 월드 전체가 새로 만들어진다
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetPackage()->GetName()));
}

void ADemo0802_GameInstance::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APawn* Pawn = GetPlayerPawn();
	AActor* ToCollect = nullptr;
	AActor* NearestCoin = nullptr;
	float Nearest = 1e9f;
	for (AActor* Coin : Coins)
	{
		if (!IsValid(Coin))
		{
			continue;
		}
		Coin->AddActorWorldRotation(FRotator(0.f, 180.f * DeltaSeconds, 0.f));
		if (Pawn)
		{
			const float Dist = FVector::Dist2D(Coin->GetActorLocation(), Pawn->GetActorLocation());
			if (Dist < PickupRadius)
			{
				ToCollect = Coin;
			}
			if (Dist < Nearest)
			{
				Nearest = Dist;
				NearestCoin = Coin;
			}
		}
	}
	if (!ToCollect && WasKeyPressed(EKeys::C))
	{
		ToCollect = NearestCoin;
	}
	if (ToCollect)
	{
		Collect(ToCollect);
	}
	if (WasKeyPressed(EKeys::N))
	{
		GoToNextStage();
		return;
	}

	UD0802_CoinKeeper* Keeper = GetKeeper();
	const int32 KeptCoins = Keeper ? Keeper->Coins : -1;
	const int32 Stage = Keeper ? Keeper->Stage : 1;
	const int32 Loads = Keeper ? Keeper->LevelLoads : 0;
	if (Panel)
	{
		Panel->SetTitle(FString::Printf(TEXT("스테이지 %d"), Stage));
		Panel->SetBody(FString::Printf(
			TEXT("GameInstance 코인 : %d   ← 레벨이 바뀌어도 유지\n")
			TEXT("액터 변수 코인   : %d   ← 레벨을 열면 0\n\n")
			TEXT("레벨 로드 횟수 %d · GameInstance 수명 %.0f초\n")
			TEXT("코드: GetGameInstance()->GetSubsystem<UD0802_CoinKeeper>()->Coins"),
			KeptCoins, ActorCoins, Loads, Keeper ? FPlatformTime::Seconds() - Keeper->CreatedAt : 0.0));
	}
	SetStatus(FString::Printf(TEXT("스테이지 %d · GameInstance 코인 %d · 액터 코인 %d · 남은 코인 %d · 레벨 로드 %d회"),
		Stage, KeptCoins, ActorCoins, Coins.Num(), Loads));
}
