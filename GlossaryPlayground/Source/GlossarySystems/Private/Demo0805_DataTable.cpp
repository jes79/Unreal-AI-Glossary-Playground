#include "Demo0805_DataTable.h"

#include "Components/StaticMeshComponent.h"
#include "D08_Common.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

// 첫 열(---)은 행 이름(Row Name). 쉼표가 들어간 값(색)은 큰따옴표로 감싼다.
const TCHAR* ADemo0805_DataTable::ItemsCSV =
	TEXT("---,DisplayName,Price,Weight,Rarity,Color,Description\n")
	TEXT("Potion,체력 물약,50,0.2,Common,\"(R=0.9,G=0.1,B=0.15,A=1.0)\",HP 50 회복\n")
	TEXT("Sword,강철 검,300,3.5,Rare,\"(R=0.7,G=0.75,B=0.8,A=1.0)\",공격력 +12\n")
	TEXT("Shield,나무 방패,120,5.0,Common,\"(R=0.5,G=0.3,B=0.1,A=1.0)\",방어력 +6\n")
	TEXT("Bow,요정의 활,800,1.2,Epic,\"(R=0.2,G=0.85,B=0.3,A=1.0)\",원거리 공격 · 사거리 1500\n")
	TEXT("Gem,용의 보석,5000,0.1,Legendary,\"(R=0.6,G=0.2,B=0.95,A=1.0)\",판매용 · 아주 비싸다\n");

ADemo0805_DataTable::ADemo0805_DataTable()
{
	DemoId = TEXT("0805");
	Controls = TEXT("1~5 : 해당 행 이름으로 FindRow · 6 : 없는 행 이름(\"Axe\") 찾기\n")
		TEXT("관찰: 받침대의 아이템 이름·색·가격이 모두 표(CSV)에서 온다. 코드를 바꾸지 않고 표만 고치면 바뀐다");
	Showcase = TEXT("CSV 로 만든 DataTable 의 5개 행을 받침대에 표시하고 FindRow 로 카드 조회");
	ItemTable = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Glossary/08_Systems/DT_0805_Items.DT_0805_Items")));
}

void ADemo0805_DataTable::BeginPlay()
{
	Super::BeginPlay();
	LoadedTable = ItemTable.LoadSynchronous();
	Card = UD08_SidePanel::Create(GetPC(), TEXT("FindRow 결과"), UD08_SidePanel::EPlace::TopRight, 440.f);
	if (!LoadedTable)
	{
		SetStatus(TEXT("DT_0805_Items 가 없다 — tools/glossary-build.sh -filter=08 로 에셋을 만든다"));
		return;
	}

	// 표의 모든 행을 순서대로 받침대에 올린다 (행 수가 늘면 받침대도 늘어난다)
	RowNames = LoadedTable->GetRowNames();
	const float Spacing = 320.f;
	const float StartY = -Spacing * (RowNames.Num() - 1) * 0.5f;
	for (int32 i = 0; i < RowNames.Num(); ++i)
	{
		const FD0805_ItemRow* Row = LoadedTable->FindRow<FD0805_ItemRow>(RowNames[i], TEXT("0805 받침대"));
		if (!Row)
		{
			continue;
		}
		const FVector Base(300.f, StartY + i * Spacing, 0.f);
		SpawnBlock(GetWorld(), Base, FVector(160.f, 160.f, 100.f), Colors::Platform);
		const EShape Shape = (i % 3 == 0) ? EShape::Sphere : (i % 3 == 1 ? EShape::Cube : EShape::Cone);
		UStaticMeshComponent* Item = AddShape(this, Shape, Base + FVector(0.f, 0.f, 170.f), FVector(80.f), Row->Color);
		SetColor(Item, Row->Color, 0.3f, Row->Rarity == TEXT("Legendary") ? 0.8f : 0.f, 0.3f);
		Pedestals.Add(Item);
		SpawnLabel(GetWorld(), FString::Printf(TEXT("%d. %s\n%d G"), i + 1, *Row->DisplayName, Row->Price), Base + FVector(0.f, 0.f, 290.f), Row->Color, 18.f);
	}
	if (RowNames.Num() > 0)
	{
		Select(RowNames[0]);
	}
}

void ADemo0805_DataTable::Select(FName RowName)
{
	Selected = RowName;
	// 핵심 한 줄: 행 이름으로 구조체 포인터를 얻는다. 없으면 nullptr (ContextString 은 경고 로그용)
	const FD0805_ItemRow* Row = LoadedTable->FindRow<FD0805_ItemRow>(RowName, TEXT("0805 FindRow"), /*bWarnIfRowMissing*/ false);
	LastQuery = FString::Printf(TEXT("FindRow<FD0805_ItemRow>(\"%s\") → %s"), *RowName.ToString(), Row ? TEXT("찾음") : TEXT("nullptr (행 없음)"));
	if (!Card)
	{
		return;
	}
	if (Row)
	{
		Card->SetTitle(FString::Printf(TEXT("%s  [%s]"), *Row->DisplayName, *Row->Rarity));
		Card->SetBody(FString::Printf(TEXT("행 이름 : %s\n가격 : %d G\n무게 : %.1f kg\n설명 : %s\n\n%s"),
			*RowName.ToString(), Row->Price, Row->Weight, *Row->Description, *LastQuery));
	}
	else
	{
		Card->SetTitle(TEXT("행 없음"));
		Card->SetBody(FString::Printf(TEXT("%s\n→ 반환값을 꼭 nullptr 검사한다"), *LastQuery));
	}
}

void ADemo0805_DataTable::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!LoadedTable)
	{
		return;
	}
	const FKey Keys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five };
	for (int32 i = 0; i < 5 && i < RowNames.Num(); ++i)
	{
		if (WasKeyPressed(Keys[i]))
		{
			Select(RowNames[i]);
		}
	}
	if (WasKeyPressed(EKeys::Six))
	{
		Select(TEXT("Axe"));
	}

	// 선택된 아이템만 돌고 위아래로 뜬다
	SpinTime += DeltaSeconds;
	for (int32 i = 0; i < Pedestals.Num(); ++i)
	{
		const bool bSel = RowNames.IsValidIndex(i) && RowNames[i] == Selected;
		UStaticMeshComponent* Item = Pedestals[i];
		FVector Loc = Item->GetRelativeLocation();
		Loc.Z = 170.f + (bSel ? 25.f * FMath::Sin(SpinTime * 3.f) + 20.f : 0.f);
		Item->SetRelativeLocation(Loc);
		if (bSel)
		{
			Item->AddRelativeRotation(FRotator(0.f, 90.f * DeltaSeconds, 0.f));
		}
	}
	SetStatus(FString::Printf(TEXT("DT_0805_Items · 행 %d개 · RowStruct FD0805_ItemRow\n%s"), RowNames.Num(), *LastQuery));
}
