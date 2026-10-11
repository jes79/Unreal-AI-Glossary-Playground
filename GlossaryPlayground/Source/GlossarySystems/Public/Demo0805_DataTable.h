#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GlossaryDemoBase.h"
#include "Demo0805_DataTable.generated.h"

class UD08_SidePanel;
class UStaticMeshComponent;

/** Data Table 한 행 = 아이템 하나. CSV 첫 줄의 열 이름이 이 변수 이름과 같아야 한다(첫 열은 행 이름). */
USTRUCT(BlueprintType)
struct FD0805_ItemRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item") FString DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item") int32 Price = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item") float Weight = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item") FString Rarity;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item") FLinearColor Color = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item") FString Description;
};

/**
 * 0805 Data Table
 * 빌더가 CSV 문자열로 만든 DT_0805_Items 에셋을 불러와 행 이름으로 FindRow 한다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API ADemo0805_DataTable : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0805_DataTable();

	/** 빌더가 쓰는 원본 CSV (실제 프로젝트에서는 엑셀에서 내보낸 .csv 파일을 Import) */
	static const TCHAR* ItemsCSV;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Data Table") TSoftObjectPtr<UDataTable> ItemTable;

private:
	void Select(FName RowName);

	UPROPERTY(Transient) TObjectPtr<UDataTable> LoadedTable;
	UPROPERTY(Transient) TObjectPtr<UD08_SidePanel> Card;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Pedestals;
	TArray<FName> RowNames;
	FName Selected;
	FString LastQuery;
	float SpinTime = 0.f;
};
