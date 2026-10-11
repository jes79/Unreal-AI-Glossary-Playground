#pragma once

#include "CoreMinimal.h"

struct FGlossaryEntry;

/** 카탈로그 항목마다 예제 레벨(/Game/Levels/{NN_분야}/L_{Id}_{Name})을 코드로 만든다. Unity 판의 Build All Scenes */
namespace GlossaryLevelBuilder
{
	struct FResult
	{
		int32 Built = 0;
		int32 Skipped = 0;   // 데모 클래스가 아직 없음
		int32 Failed = 0;
		TArray<FString> Messages;
	};

	/** Filter: "" = 전부, "05" = 05 분야, "0105,0106" = 지정 항목 */
	GLOSSARYEDITOR_API FResult BuildLevels(const FString& Filter);
	GLOSSARYEDITOR_API bool BuildHubLevel();
	GLOSSARYEDITOR_API bool BuildLevel(const FGlossaryEntry& Entry, FString& OutMessage);

	/** 에셋 빌더 → 허브 → 레벨 순서로 전부 */
	GLOSSARYEDITOR_API FResult BuildAll(const FString& Filter);

	/** 필터에 맞는지 */
	GLOSSARYEDITOR_API bool MatchesFilter(const FString& Id, const FString& Filter);
}
