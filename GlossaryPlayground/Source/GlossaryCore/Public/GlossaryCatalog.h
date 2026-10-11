#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GlossaryCatalog.generated.h"

/** 용어집 항목 하나 = 예제 레벨 하나. 데이터는 GlossaryCatalogData.inl (용어집 문서에서 추출) */
USTRUCT(BlueprintType)
struct GLOSSARYCORE_API FGlossaryEntry
{
	GENERATED_BODY()

	/** "0105" 처럼 분야 2자리 + 순번 2자리 */
	UPROPERTY(BlueprintReadOnly, Category = Glossary) FString Id;
	/** 코드용 영문 이름. 레벨 L_{Id}_{Name}, 클래스 Demo{Id}_{Name} */
	UPROPERTY(BlueprintReadOnly, Category = Glossary) FString Name;
	/** 용어 (문서 표의 첫 열) */
	UPROPERTY(BlueprintReadOnly, Category = Glossary) FString Term;
	/** 쉽게 말하면 */
	UPROPERTY(BlueprintReadOnly, Category = Glossary) FString Plain;
	/** 이렇게 요청하세요 (따옴표 없이) */
	UPROPERTY(BlueprintReadOnly, Category = Glossary) FString Request;

	int32 GetCategoryIndex() const;
};

USTRUCT(BlueprintType)
struct GLOSSARYCORE_API FGlossaryCategory
{
	GENERATED_BODY()

	/** "01" */
	UPROPERTY(BlueprintReadOnly, Category = Glossary) FString Code;
	/** "01_Movement" — 레벨 폴더와 생성 에셋 폴더 이름 */
	UPROPERTY(BlueprintReadOnly, Category = Glossary) FString Folder;
	/** "캐릭터 이동·조작·카메라" */
	UPROPERTY(BlueprintReadOnly, Category = Glossary) FString Title;
	/** 데모 클래스가 들어 있는 C++ 모듈 이름 ("GlossaryMovement") */
	UPROPERTY(BlueprintReadOnly, Category = Glossary) FString Module;
};

UCLASS()
class GLOSSARYCORE_API UGlossaryCatalog : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = Glossary)
	static const TArray<FGlossaryEntry>& GetEntries();

	UFUNCTION(BlueprintPure, Category = Glossary)
	static const TArray<FGlossaryCategory>& GetCategories();

	/** Id로 찾기. 없으면 false */
	UFUNCTION(BlueprintPure, Category = Glossary)
	static bool FindEntry(const FString& Id, FGlossaryEntry& OutEntry);

	static const FGlossaryEntry* Find(const FString& Id);
	static const FGlossaryCategory& GetCategory(const FGlossaryEntry& Entry);

	/** /Game/Levels/01_Movement/L_0105_CoyoteTime */
	UFUNCTION(BlueprintPure, Category = Glossary)
	static FString GetLevelPackage(const FGlossaryEntry& Entry);

	/** /Script/GlossaryMovement.Demo0105_CoyoteTime */
	UFUNCTION(BlueprintPure, Category = Glossary)
	static FString GetDemoClassPath(const FGlossaryEntry& Entry);

	/** /Game/Glossary/01_Movement — 분야별 생성 에셋 폴더 */
	static FString GetAssetFolder(const FString& Id);

	static const TCHAR* HubLevelPackage() { return TEXT("/Game/Levels/L_0000_Hub"); }
};
