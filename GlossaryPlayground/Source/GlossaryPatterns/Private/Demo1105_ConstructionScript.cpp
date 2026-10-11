#include "Demo1105_ConstructionScript.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

static const FName FencePieceTag(TEXT("FencePiece"));

// ================================================================ AD1105_Fence

AD1105_Fence::AD1105_Fence()
{
	PrimaryActorTick.bCanEverTick = false; // 울타리는 매 프레임 할 일이 없다
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void AD1105_Fence::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// 에디터: 액터를 놓을 때·옮길 때·디테일 값을 바꿀 때마다 호출. 게임: 스폰될 때 한 번.
	++ConstructionCount;
	RebuildFence();
}

UStaticMeshComponent* AD1105_Fence::AddPiece(const FVector& Location, const FVector& Size, const FLinearColor& Color)
{
	UStaticMeshComponent* Piece = NewObject<UStaticMeshComponent>(this);
	// ▼ "Construction Script 가 만든 컴포넌트" 표시: 재실행 때 엔진이 자동 정리(DestroyConstructedComponents)
	Piece->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	Piece->ComponentTags.Add(FencePieceTag);
	Piece->SetMobility(EComponentMobility::Movable);
	InitShapeComponent(Piece, EShape::Cube, Size);
	Piece->SetupAttachment(Root);
	Piece->SetRelativeLocation(Location);
	Piece->RegisterComponent();
	SetColor(Piece, Color);
	return Piece;
}

void AD1105_Fence::DestroyPieces()
{
	// RerunConstructionScripts 경로에서는 엔진이 이미 지웠으므로 여기서 찾는 것이 없다.
	// RebuildFence 를 직접 부르는 경로(패키지 게임)에서만 이전 기둥이 남아 있어 직접 지운다.
	TArray<UActorComponent*> Old;
	GetComponents(Old);
	for (UActorComponent* Comp : Old)
	{
		if (Comp && Comp->ComponentHasTag(FencePieceTag))
		{
			Comp->DestroyComponent();
		}
	}
}

void AD1105_Fence::RebuildFence()
{
	DestroyPieces();

	const float Length = Spacing * (PostCount - 1);
	for (int32 i = 0; i < PostCount; ++i)
	{
		// 가운데 정렬: 0 번 기둥이 -Length/2, 마지막이 +Length/2
		const float Y = -Length * 0.5f + Spacing * i;
		AddPiece(FVector(0.f, Y, PostHeight * 0.5f), FVector(24.f, 24.f, PostHeight), FLinearColor(0.45f, 0.28f, 0.12f));
	}
	if (bRails && PostCount >= 2)
	{
		for (const float Z : { PostHeight * 0.35f, PostHeight * 0.8f })
		{
			AddPiece(FVector(0.f, 0.f, Z), FVector(10.f, Length, 14.f), FLinearColor(0.6f, 0.4f, 0.2f));
		}
	}
}

int32 AD1105_Fence::CountPostComponents() const
{
	int32 Count = 0;
	TInlineComponentArray<UStaticMeshComponent*> Meshes(this);
	for (const UStaticMeshComponent* Mesh : Meshes)
	{
		Count += Mesh->ComponentHasTag(FencePieceTag) ? 1 : 0;
	}
	return Count;
}

// ================================================================ ADemo1105

ADemo1105_ConstructionScript::ADemo1105_ConstructionScript()
{
	DemoId = TEXT("1105");
	Controls = TEXT("[ / ] : 기둥 개수 -1 / +1    , / . : 간격 -25 / +25    R : 레일 켜기/끄기\n")
		TEXT("값을 바꿀 때마다 RerunConstructionScripts() → OnConstruction 이 울타리를 처음부터 다시 조립\n")
		TEXT("에디터에서: 레벨의 Fence 를 선택하고 디테일 패널 Post Count 를 바꾸면 똑같이 즉시 재조립된다");
	Showcase = TEXT("기둥 개수·간격을 바꾸면 OnConstruction 이 울타리를 다시 배치, 호출 횟수·컴포넌트 수 표시");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-700.f, 0.f, 100.f));
}

#if WITH_EDITOR
void ADemo1105_ConstructionScript::BuildLevel(UWorld* World)
{
	// 에디터에서 직접 만져 볼 수 있도록 울타리를 레벨에 저장해 둔다(기둥 컴포넌트는 저장되지 않고 로드 때 재생성)
	AD1105_Fence* Placed = World->SpawnActor<AD1105_Fence>(FVector(400.f, 0.f, 0.f), FRotator::ZeroRotator);
	Placed->SetActorLabel(TEXT("Fence (Post Count 를 바꿔 보세요)"));
}
#endif

void ADemo1105_ConstructionScript::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	for (TActorIterator<AD1105_Fence> It(World); It; ++It)
	{
		Fence = *It;
		break;
	}
	if (!Fence)
	{
		// 레벨에 없으면(빌더를 안 거친 경우) 스폰 — SpawnActor 도 OnConstruction 을 한 번 부른다
		Fence = World->SpawnActor<AD1105_Fence>(FVector(400.f, 0.f, 0.f), FRotator::ZeroRotator);
		Log.Add(TEXT("레벨에 울타리가 없어 SpawnActor (OnConstruction 1회)"));
	}
	else if (Fence->ConstructionCount > 0)
	{
		Log.Add(FString::Printf(TEXT("레벨에 저장된 울타리 로드 → OnConstruction %d회 실행됨"), Fence->ConstructionCount));
	}
	else
	{
		// 게임 실행에서는 로드 때 Construction Script 를 다시 돌리지 않고 저장된 조립 결과를 그대로 쓴다
		Log.Add(FString::Printf(TEXT("레벨에 저장된 울타리 로드 (게임에선 로드 때 재조립 안 함, 조각 %d개 그대로)"), Fence->CountPostComponents()));
	}
	SpawnLabel(World, TEXT("AD1105_Fence"), FVector(400.f, 0.f, 260.f), Colors::Item, 20.f);
	Panel = UD11_SidePanel::Create(GetPC(), TEXT("OnConstruction 기록"), FVector2D(1.f, 0.f), 14.f);
}

void ADemo1105_ConstructionScript::Rerun()
{
	const int32 Before = Fence->CountPostComponents();
#if WITH_EDITOR
	// 에디터가 디테일 값 변경 후 하는 일과 같다: 생성된 컴포넌트를 지우고 OnConstruction 을 다시 실행
	Fence->RerunConstructionScripts();
	const TCHAR* Path = TEXT("RerunConstructionScripts()");
#else
	// 패키지 게임에는 RerunConstructionScripts 가 없다(에디터 전용) → 같은 조립 함수를 직접 호출
	Fence->RebuildFence();
	const TCHAR* Path = TEXT("RebuildFence()");
#endif
	Log.Add(FString::Printf(TEXT("%s: 조각 %d → %d개 (기둥 %d · 간격 %.0f)"), Path, Before, Fence->CountPostComponents(), Fence->PostCount, Fence->Spacing));
}

void ADemo1105_ConstructionScript::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Fence)
	{
		return;
	}
	bool bChanged = false;
	if (WasKeyPressed(EKeys::LeftBracket))  { Fence->PostCount = FMath::Clamp(Fence->PostCount - 1, 2, 30); bChanged = true; }
	if (WasKeyPressed(EKeys::RightBracket)) { Fence->PostCount = FMath::Clamp(Fence->PostCount + 1, 2, 30); bChanged = true; }
	if (WasKeyPressed(EKeys::Comma))        { Fence->Spacing = FMath::Clamp(Fence->Spacing - 25.f, 50.f, 500.f); bChanged = true; }
	if (WasKeyPressed(EKeys::Period))       { Fence->Spacing = FMath::Clamp(Fence->Spacing + 25.f, 50.f, 500.f); bChanged = true; }
	if (WasKeyPressed(EKeys::R))            { Fence->bRails = !Fence->bRails; bChanged = true; }
	if (bChanged)
	{
		Rerun();
	}

	if (Panel)
	{
		Panel->SetBody(FString::Printf(TEXT("PostCount %d · Spacing %.0f · bRails %s\nOnConstruction 호출 %d회\n울타리 조각 컴포넌트 %d개 (기둥 %d + 레일 %d)\n\n")
			TEXT("조각마다 CreationMethod = UserConstructionScript\n→ 재실행 때 엔진이 지우므로 개수가 계속 늘지 않는다\n\n%s"),
			Fence->PostCount, Fence->Spacing, Fence->bRails ? TEXT("true") : TEXT("false"),
			Fence->ConstructionCount, Fence->CountPostComponents(), Fence->PostCount, Fence->bRails ? 2 : 0, *Log.Join()));
	}
	SetStatus(FString::Printf(TEXT("기둥 %d개 · 길이 %.0fcm · OnConstruction %d회"),
		Fence->PostCount, Fence->Spacing * (Fence->PostCount - 1), Fence->ConstructionCount));
}
