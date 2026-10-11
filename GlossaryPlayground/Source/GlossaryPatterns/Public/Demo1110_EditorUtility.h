#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "D11_SidePanel.h"
#include "Demo1110_EditorUtility.generated.h"

class AGlossaryLabel;

/**
 * 1110 정렬 도구의 계산 부분. 에디터 도구(D1110_AlignTool.cpp, WITH_EDITOR)와 레벨 안 미리보기가 같은 코드를 쓴다.
 */
namespace D1110_GridAlign
{
	/** Origin 에서 +X(행)·+Y(열) 방향으로 Columns 열짜리 격자 위치 Count 개 */
	GLOSSARYPATTERNS_API TArray<FVector> ComputeGrid(const FVector& Origin, int32 Count, int32 Columns, float Spacing);

	/** "Crate" + 3 → "Crate_03" (일괄 이름 변경 규칙) */
	GLOSSARYPATTERNS_API FString MakeName(const FString& Prefix, int32 Index);

	/** 정렬 순서를 일정하게: 현재 위치의 X, 그다음 Y 로 정렬 */
	GLOSSARYPATTERNS_API void SortForAlign(TArray<AActor*>& Actors);
}

#if WITH_EDITOR
/** 에디터 전용 도구 등록 (모듈 StartupModule/ShutdownModule 에서 호출) */
namespace D1110_EditorTool
{
	void Register();
	void Unregister();
	/** 선택한 액터를 격자로 정렬하고 Prefix_01.. 로 이름을 바꾼다 (Undo 가능) */
	GLOSSARYPATTERNS_API int32 AlignSelected(int32 Columns, float Spacing, const FString& Prefix);
}
#endif

/**
 * 1110 Editor Utility Widget
 * Editor Utility Widget 은 WBP 에셋이라 C++ 로 만들 수 없다. 대신 같은 일을 하는 에디터 도구를
 * 이 모듈이 C++ 로 등록한다: Tools > Glossary > Patterns > Align Selected (ToolMenus 메뉴) +
 * 'Glossary Align Tool' 창(Slate 탭). 내부는 UEditorActorSubsystem 으로 선택한 액터를 읽고 옮기고 이름을 바꾼다.
 * 레벨 안에서는 같은 계산을 미리보기로 보여 준다(1 = 정렬, 2 = 되돌리기).
 */
UCLASS()
class GLOSSARYPATTERNS_API ADemo1110_EditorUtility : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1110_EditorUtility();

#if WITH_EDITOR
	virtual void BuildLevel(UWorld* World) override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Align Tool", meta = (ClampMin = 1))
	int32 Columns = 4;

	UPROPERTY(EditAnywhere, Category = "Align Tool", meta = (ClampMin = 50))
	float Spacing = 250.f;

	UPROPERTY(EditAnywhere, Category = "Align Tool")
	FString NamePrefix = TEXT("Crate");

private:
	static FString DisplayName(const AActor* Actor);

	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Messy;
	UPROPERTY(Transient) TArray<TObjectPtr<AGlossaryLabel>> NameLabels;
	UPROPERTY(Transient) TObjectPtr<UD11_SidePanel> Panel;

	TArray<FVector> Original;
	TArray<FRotator> OriginalRot;
	TArray<FVector> Goal;
	TArray<FString> OriginalNames;
	TArray<FString> GoalNames;
	bool bAligned = false;
};
