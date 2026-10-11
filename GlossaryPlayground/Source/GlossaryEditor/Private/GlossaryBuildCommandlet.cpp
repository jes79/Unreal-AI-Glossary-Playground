#include "GlossaryBuildCommandlet.h"

#include "GlossaryAssetKit.h"
#include "GlossaryLevelBuilder.h"

DEFINE_LOG_CATEGORY_STATIC(LogGlossaryCommandlet, Log, All);

UGlossaryBuildCommandlet::UGlossaryBuildCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UGlossaryBuildCommandlet::Main(const FString& Params)
{
	FString Filter;
	FParse::Value(*Params, TEXT("filter="), Filter);
	const bool bAssetsOnly = FParse::Param(*Params, TEXT("assetsonly"));
	const bool bLevelsOnly = FParse::Param(*Params, TEXT("levelsonly"));

	GlossaryLevelBuilder::FResult Result;
	if (bAssetsOnly)
	{
		GlossaryAssetKit::RunBuilders(Filter);
	}
	else if (bLevelsOnly)
	{
		if (Filter.IsEmpty())
		{
			GlossaryLevelBuilder::BuildHubLevel();
		}
		Result = GlossaryLevelBuilder::BuildLevels(Filter);
	}
	else
	{
		Result = GlossaryLevelBuilder::BuildAll(Filter);
	}

	UE_LOG(LogGlossaryCommandlet, Display, TEXT("[Glossary] 결과: 생성 %d · 건너뜀 %d · 실패 %d"), Result.Built, Result.Skipped, Result.Failed);
	return Result.Failed > 0 ? 1 : 0;
}
