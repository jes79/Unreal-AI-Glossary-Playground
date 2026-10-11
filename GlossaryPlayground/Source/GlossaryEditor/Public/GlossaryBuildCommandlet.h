#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GlossaryBuildCommandlet.generated.h"

/**
 * 명령줄 일괄 생성:
 *   UnrealEditor-Cmd.exe GlossaryPlayground.uproject -run=GlossaryBuild [-filter=05] [-assetsonly] [-levelsonly]
 * Unity 판의 GlossaryBatch.BuildAndValidate 에 해당
 */
UCLASS()
class GLOSSARYEDITOR_API UGlossaryBuildCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGlossaryBuildCommandlet();
	virtual int32 Main(const FString& Params) override;
};
