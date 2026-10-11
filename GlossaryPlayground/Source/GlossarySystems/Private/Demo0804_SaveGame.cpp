#include "Demo0804_SaveGame.h"

#include "D08_Common.h"
#include "Engine/StaticMeshActor.h"
#include "GlossaryKit.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"

using namespace GlossaryKit;

ADemo0804_SaveGame::ADemo0804_SaveGame()
{
	DemoId = TEXT("0804");
	// 플레이어 컨트롤러(입력 처리) 다음에 Tick 해서 WasKeyPressed 를 놓치지 않게
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	Controls = TEXT("WASD 이동 · H : 체력 -10 · C : 코인 +1\n")
		TEXT("1 : 저장 (SaveGameToSlot) · 2 : 불러오기 (LoadGameFromSlot) · 3 : 슬롯 삭제\n")
		TEXT("관찰: 저장 후 움직이고 체력을 깎은 뒤 2 를 누르면 저장 시점의 위치·체력·코인으로 돌아간다");
	Showcase = TEXT("위치·체력·코인을 USaveGame 슬롯 파일에 저장/불러오기, 저장 위치 표시");
}

void ADemo0804_SaveGame::BeginPlay()
{
	Super::BeginPlay();
	// 저장 위치를 보여 줄 표식 (보라 원판)
	AStaticMeshActor* Marker = SpawnShape(GetWorld(), EShape::Cylinder, FVector::ZeroVector, FVector(140.f, 140.f, 6.f), Colors::Purple);
	Marker->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetColor(Marker->GetStaticMeshComponent(), Colors::Purple, 0.5f, 0.f, 1.5f);
	SavedMarker = Marker;
	SavedLabel = SpawnLabel(GetWorld(), TEXT("저장 위치"), FVector::ZeroVector, Colors::Purple, 18.f);

	// 주변 지형 (움직였다는 것이 잘 보이게)
	for (int32 i = 0; i < 5; ++i)
	{
		SpawnBlock(GetWorld(), FVector(300.f + i * 350.f, (i % 2 ? 500.f : -500.f), 0.f), FVector(200.f, 200.f, 150.f + i * 60.f), Colors::Platform);
	}

	Panel = UD08_SidePanel::Create(GetPC(), TEXT("현재 상태"), UD08_SidePanel::EPlace::TopRight, 440.f);

	// 이전 실행의 저장 파일이 있으면 표식만 보여 준다(자동으로 불러오지는 않음)
	if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		if (UD0804_SaveGame* Data = Cast<UD0804_SaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex)))
		{
			UpdateMarker(Data->PlayerLocation, true);
			LastAction = FString::Printf(TEXT("이전 저장이 있다 (%s) — 2 로 불러오기"), *Data->SavedAt);
		}
	}
	else
	{
		UpdateMarker(FVector::ZeroVector, false);
		LastAction = TEXT("저장 파일 없음 — 1 로 저장");
	}
}

void ADemo0804_SaveGame::UpdateMarker(const FVector& Location, bool bVisible)
{
	const FVector Ground(Location.X, Location.Y, 4.f);
	SavedMarker->SetActorLocation(Ground);
	SavedMarker->SetActorHiddenInGame(!bVisible);
	SavedLabel->SetActorLocation(Ground + FVector(0.f, 0.f, 60.f));
	SavedLabel->SetActorHiddenInGame(!bVisible);
}

void ADemo0804_SaveGame::Save()
{
	APawn* Pawn = GetPlayerPawn();
	// 1) 저장 객체를 만들고 2) 값을 채우고 3) 슬롯에 쓴다
	UD0804_SaveGame* Data = Cast<UD0804_SaveGame>(UGameplayStatics::CreateSaveGameObject(UD0804_SaveGame::StaticClass()));
	Data->PlayerLocation = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	Data->PlayerRotation = Pawn ? Pawn->GetActorRotation() : FRotator::ZeroRotator;
	Data->HP = HP;
	Data->Coins = Coins;
	Data->SavedAt = FDateTime::Now().ToString(TEXT("%H:%M:%S"));
	const bool bOk = UGameplayStatics::SaveGameToSlot(Data, SlotName, UserIndex);
	UpdateMarker(Data->PlayerLocation, bOk);
	LastAction = bOk ? FString::Printf(TEXT("저장 완료 %s (HP %.0f · 코인 %d)"), *Data->SavedAt, HP, Coins) : TEXT("저장 실패");
	if (Pawn)
	{
		AD08_FloatingText::Spawn(GetWorld(), TEXT("저장!"), Pawn->GetActorLocation() + FVector(0, 0, 130), Colors::Purple, 26.f);
	}
}

void ADemo0804_SaveGame::Load()
{
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		LastAction = TEXT("불러오기 실패: 슬롯이 없다");
		return;
	}
	UD0804_SaveGame* Data = Cast<UD0804_SaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
	if (!Data)
	{
		LastAction = TEXT("불러오기 실패: 형식이 다르다");
		return;
	}
	// 저장된 값으로 게임 상태를 되돌린다
	HP = Data->HP;
	Coins = Data->Coins;
	if (APawn* Pawn = GetPlayerPawn())
	{
		Pawn->SetActorLocationAndRotation(Data->PlayerLocation, Data->PlayerRotation, false, nullptr, ETeleportType::TeleportPhysics);
		if (AController* Controller = Pawn->GetController())
		{
			Controller->SetControlRotation(Data->PlayerRotation);
		}
		AD08_FloatingText::Spawn(GetWorld(), TEXT("불러오기!"), Data->PlayerLocation + FVector(0, 0, 130), Colors::Cyan, 26.f);
	}
	UpdateMarker(Data->PlayerLocation, true);
	LastAction = FString::Printf(TEXT("불러오기 완료 (%s 저장본: HP %.0f · 코인 %d)"), *Data->SavedAt, Data->HP, Data->Coins);
}

void ADemo0804_SaveGame::DeleteSave()
{
	const bool bOk = UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);
	UpdateMarker(FVector::ZeroVector, false);
	LastAction = bOk ? TEXT("슬롯 삭제 완료") : TEXT("삭제할 슬롯이 없다");
}

void ADemo0804_SaveGame::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APawn* Pawn = GetPlayerPawn();
	if (WasKeyPressed(EKeys::H))
	{
		HP = FMath::Max(0.f, HP - 10.f);
		if (Pawn)
		{
			AD08_FloatingText::Spawn(GetWorld(), TEXT("-10"), Pawn->GetActorLocation() + FVector(0, 0, 110), Colors::Enemy, 24.f);
		}
	}
	if (WasKeyPressed(EKeys::C))
	{
		++Coins;
		if (Pawn)
		{
			AD08_FloatingText::Spawn(GetWorld(), TEXT("+1 코인"), Pawn->GetActorLocation() + FVector(0, 0, 110), Colors::Item, 24.f);
		}
	}
	if (WasKeyPressed(EKeys::One))   { Save(); }
	if (WasKeyPressed(EKeys::Two))   { Load(); }
	if (WasKeyPressed(EKeys::Three)) { DeleteSave(); }

	const bool bExists = UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
	const FString FilePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("SaveGames") / (SlotName + TEXT(".sav")));
	if (Panel)
	{
		Panel->SetTitle(FString::Printf(TEXT("HP %.0f / 100    코인 %d"), HP, Coins));
		Panel->SetProgress(HP / 100.f, HP > 30.f ? Colors::Ally : Colors::Enemy);
		Panel->SetBody(FString::Printf(TEXT("위치 (%.0f, %.0f, %.0f)\n슬롯 \"%s\" : %s\n%s"),
			Pawn ? Pawn->GetActorLocation().X : 0.f, Pawn ? Pawn->GetActorLocation().Y : 0.f, Pawn ? Pawn->GetActorLocation().Z : 0.f,
			*SlotName, bExists ? TEXT("있음") : TEXT("없음"), *FilePath));
	}
	SetStatus(LastAction);
}
