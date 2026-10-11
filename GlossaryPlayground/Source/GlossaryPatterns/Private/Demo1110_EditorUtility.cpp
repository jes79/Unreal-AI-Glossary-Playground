#include "Demo1110_EditorUtility.h"

#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"

using namespace GlossaryKit;

static const FName MessyTag(TEXT("D1110_Messy"));

// ================================================================ 공유 계산 (에디터 도구 + 미리보기)

TArray<FVector> D1110_GridAlign::ComputeGrid(const FVector& Origin, int32 Count, int32 Columns, float Spacing)
{
	TArray<FVector> Result;
	Result.Reserve(Count);
	Columns = FMath::Max(1, Columns);
	for (int32 i = 0; i < Count; ++i)
	{
		const int32 Row = i / Columns;
		const int32 Col = i % Columns;
		Result.Add(Origin + FVector(Row * Spacing, Col * Spacing, 0.f));
	}
	return Result;
}

FString D1110_GridAlign::MakeName(const FString& Prefix, int32 Index)
{
	return FString::Printf(TEXT("%s_%02d"), *Prefix, Index);
}

void D1110_GridAlign::SortForAlign(TArray<AActor*>& Actors)
{
	Actors.Sort([](const AActor& A, const AActor& B)
	{
		const FVector LA = A.GetActorLocation(), LB = B.GetActorLocation();
		return LA.X != LB.X ? LA.X < LB.X : LA.Y < LB.Y;
	});
}

// ================================================================ ADemo1110

ADemo1110_EditorUtility::ADemo1110_EditorUtility()
{
	DemoId = TEXT("1110");
	Controls = TEXT("1 : 미리보기 — 에디터 도구와 같은 계산으로 격자 정렬 + 이름 일괄 변경   2 : 원래대로\n")
		TEXT("실제 도구(에디터): 이 레벨을 열고 흩어진 상자를 선택 → Tools > Glossary > Patterns > Align Selected\n")
		TEXT("또는 Tools > Glossary > Patterns > Open Align Tool 창에서 열 수·간격·이름 접두사를 정하고 실행 (Ctrl+Z 로 되돌리기)");
	Showcase = TEXT("선택 액터 격자 정렬·일괄 이름 변경 에디터 도구(메뉴+Slate 창)와 같은 계산의 실행 중 미리보기");
	Implementation = EGlossaryImplementation::EditorTool;
	AlternativeNote = TEXT("Editor Utility Widget(WBP)은 C++ 로 만들 수 없어, 같은 기능을 ToolMenus 메뉴 + Slate 탭 + UEditorActorSubsystem 으로 구현");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-700.f, 0.f, 100.f));
}

#if WITH_EDITOR
void ADemo1110_EditorUtility::BuildLevel(UWorld* World)
{
	// 에디터에서 도구를 바로 시험할 수 있도록 '흩어진 상자 12개'를 레벨에 저장한다
	const TCHAR* MessyNames[] = { TEXT("Cube7"), TEXT("box_old"), TEXT("SM_Thing (3)"), TEXT("crate-final"), TEXT("Cube_copy"), TEXT("asdf"),
		TEXT("Box2_new"), TEXT("tmp"), TEXT("Crate_FINAL_v2"), TEXT("Cube12"), TEXT("thing"), TEXT("SM_Box_bak") };
	FRandomStream Rand(1110);
	for (int32 i = 0; i < UE_ARRAY_COUNT(MessyNames); ++i)
	{
		const FVector Loc(Rand.FRandRange(0.f, 1100.f), Rand.FRandRange(-650.f, 650.f), 50.f);
		AStaticMeshActor* Box = SpawnShape(World, EShape::Cube, Loc, FVector(100.f), FLinearColor(0.55f, 0.35f, 0.15f), FRotator(0.f, Rand.FRandRange(0.f, 90.f), 0.f));
		Box->Tags.Add(MessyTag);
		Box->SetActorLabel(MessyNames[i]);
		Box->SetFolderPath(TEXT("1110_MessyBoxes"));
	}
}
#endif

FString ADemo1110_EditorUtility::DisplayName(const AActor* Actor)
{
#if WITH_EDITOR
	return Actor->GetActorLabel(); // 아웃라이너에 보이는 이름(에디터 데이터)
#else
	return Actor->GetName();
#endif
}

void ADemo1110_EditorUtility::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	TArray<AActor*> Found;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(MessyTag))
		{
			Found.Add(*It);
		}
	}
	if (Found.Num() == 0)
	{
		// 빌더를 안 거친 레벨이면 실행 중에 만든다
		FRandomStream Rand(1110);
		for (int32 i = 0; i < 12; ++i)
		{
			Found.Add(SpawnShape(World, EShape::Cube, FVector(Rand.FRandRange(0.f, 1100.f), Rand.FRandRange(-650.f, 650.f), 50.f), FVector(100.f),
				FLinearColor(0.55f, 0.35f, 0.15f), FRotator(0.f, Rand.FRandRange(0.f, 90.f), 0.f)));
		}
	}

	// 에디터 도구와 똑같이: 정렬 순서 → 격자 위치 → 새 이름
	D1110_GridAlign::SortForAlign(Found);
	Goal = D1110_GridAlign::ComputeGrid(FVector(100.f, -Spacing * (Columns - 1) * 0.5f, 50.f), Found.Num(), Columns, Spacing);
	for (int32 i = 0; i < Found.Num(); ++i)
	{
		Messy.Add(Found[i]);
		Original.Add(Found[i]->GetActorLocation());
		OriginalRot.Add(Found[i]->GetActorRotation());
		OriginalNames.Add(DisplayName(Found[i]));
		GoalNames.Add(D1110_GridAlign::MakeName(NamePrefix, i + 1));
		NameLabels.Add(SpawnLabel(World, OriginalNames.Last(), Found[i]->GetActorLocation() + FVector(0.f, 0.f, 110.f), FLinearColor::White, 13.f));
	}

	Panel = UD11_SidePanel::Create(GetPC(), TEXT("에디터 도구 사용법"), FVector2D(1.f, 0.f), 13.f);
}

void ADemo1110_EditorUtility::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::One)) { bAligned = true; }
	if (WasKeyPressed(EKeys::Two)) { bAligned = false; }

	// 미리보기는 부드럽게 움직인다(실제 도구는 즉시 SetActorLocation)
	for (int32 i = 0; i < Messy.Num(); ++i)
	{
		const FVector Target = bAligned ? Goal[i] : Original[i];
		const FVector Now = FMath::VInterpTo(Messy[i]->GetActorLocation(), Target, DeltaSeconds, 6.f);
		Messy[i]->SetActorLocation(Now);
		Messy[i]->SetActorRotation(FMath::RInterpTo(Messy[i]->GetActorRotation(), bAligned ? FRotator::ZeroRotator : OriginalRot[i], DeltaSeconds, 6.f));
		NameLabels[i]->SetActorLocation(Now + FVector(0.f, 0.f, 110.f));
		NameLabels[i]->SetText(bAligned ? GoalNames[i] : OriginalNames[i]);
		NameLabels[i]->SetColor(bAligned ? Colors::Ally : FLinearColor::White);
	}

	if (Panel)
	{
		Panel->SetBody(FString::Printf(
			TEXT("에디터에서 (이 레벨을 열고):\n")
			TEXT("  1) 아웃라이너 1110_MessyBoxes 폴더의 상자 선택\n")
			TEXT("  2) Tools > Glossary > Patterns > Align Selected\n")
			TEXT("     또는 Open Align Tool… 창에서 값 조절 후 실행\n")
			TEXT("  3) Ctrl+Z 로 되돌리기 (FScopedTransaction)\n")
			TEXT("  콘솔: Glossary.AlignSelected\n\n")
			TEXT("도구 내부 (D1110_AlignTool.cpp):\n")
			TEXT("  UEditorActorSubsystem::GetSelectedLevelActors()\n")
			TEXT("  → ComputeGrid(열 %d, 간격 %.0f)\n")
			TEXT("  → SetActorLocation + SetActorLabel(\"%s_01\"…)\n\n")
			TEXT("미리보기: %s"),
			Columns, Spacing, *NamePrefix, bAligned ? TEXT("정렬됨 (2 = 되돌리기)") : TEXT("흩어짐 (1 = 정렬)")));
	}
	SetStatus(FString::Printf(TEXT("대상 %d개 · %s\nEditor Utility Widget 을 쓰는 경우: 콘텐츠 브라우저 우클릭 > Editor Utilities > Editor Utility Widget"),
		Messy.Num(), bAligned ? TEXT("격자 정렬 + 이름 변경 미리보기") : TEXT("원래 위치·이름")));
}
