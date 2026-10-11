#include "GlossaryLevelBuilder.h"

#include "BSPOps.h"
#include "Builders/CubeBuilder.h"
#include "Components/BrushComponent.h"
#include "Editor.h"
#include "Engine/Polys.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerStart.h"
#include "GlossaryAssetKit.h"
#include "GlossaryCatalog.h"
#include "GlossaryDemoBase.h"
#include "GlossaryEnvironment.h"
#include "GlossaryHub.h"
#include "Misc/PackageName.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogGlossaryBuild, Log, All);

namespace GlossaryLevelBuilder
{
	bool MatchesFilter(const FString& Id, const FString& Filter)
	{
		if (Filter.IsEmpty())
		{
			return true;
		}
		TArray<FString> Parts;
		Filter.ParseIntoArray(Parts, TEXT(","), true);
		for (FString Part : Parts)
		{
			Part.TrimStartAndEndInline();
			if (Part.Len() == 2 ? Id.StartsWith(Part) : Id == Part)
			{
				return true;
			}
		}
		return false;
	}

	static ANavMeshBoundsVolume* SpawnNavBounds(UWorld* World, const FVector& Extent)
	{
		ANavMeshBoundsVolume* Volume = World->SpawnActor<ANavMeshBoundsVolume>(FVector(0.f, 0.f, Extent.Z * 0.5f - 100.f), FRotator::ZeroRotator);
		// UActorFactoryBoxVolume 과 같은 순서로 브러시를 만든다
		Volume->PolyFlags = 0;
		Volume->Brush = NewObject<UModel>(Volume, NAME_None, RF_Transactional);
		Volume->Brush->Initialize(nullptr, true);
		Volume->Brush->Polys = NewObject<UPolys>(Volume->Brush, NAME_None, RF_Transactional);
		Volume->GetBrushComponent()->Brush = Volume->Brush;
		UCubeBuilder* Builder = NewObject<UCubeBuilder>(Volume);
		Builder->X = Extent.X * 2.f;
		Builder->Y = Extent.Y * 2.f;
		Builder->Z = Extent.Z;
		Volume->BrushBuilder = Builder;
		Builder->Build(World, Volume);
		FBSPOps::csgPrepMovingBrush(Volume);
		Volume->GetBrushComponent()->RequestUpdateBrushCollision();
		Volume->PostEditChange();
		return Volume;
	}

	/** 새 빈 맵에 공통 액터(환경·데모·PlayerStart·NavMesh)를 놓고 저장 */
	static bool BuildMap(const FString& PackagePath, UClass* DemoClass, FString& OutMessage)
	{
		UWorld* World = UEditorLoadingAndSavingUtils::NewBlankMap(false);
		if (!World)
		{
			OutMessage = TEXT("NewBlankMap 실패");
			return false;
		}
		const AGlossaryDemoBase* Defaults = GetDefault<AGlossaryDemoBase>(DemoClass);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AGlossaryEnvironment* Environment = World->SpawnActor<AGlossaryEnvironment>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
		Environment->Settings = Defaults->Environment;
		Environment->ApplySettings();
		Environment->SetActorLabel(TEXT("GlossaryEnvironment"));

		AGlossaryDemoBase* Demo = World->SpawnActor<AGlossaryDemoBase>(DemoClass, FTransform::Identity, Params);
		Demo->SetActorLabel(FString::Printf(TEXT("Demo_%s"), *Defaults->DemoId));

		APlayerStart* Start = World->SpawnActor<APlayerStart>(Defaults->PlayerStartTransform.GetLocation(), Defaults->PlayerStartTransform.Rotator(), Params);
		Start->SetActorLabel(TEXT("PlayerStart"));

		if (Defaults->bNeedsNavMesh)
		{
			SpawnNavBounds(World, Defaults->NavBoundsExtent)->SetActorLabel(TEXT("NavMeshBounds"));
		}

		// 데모별 레벨 구성 (볼륨, 정적 라이트 등)
		Demo->BuildLevel(World);

		if (Defaults->bNeedsNavMesh)
		{
			if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
			{
				Nav->Build();
			}
		}

		if (!UEditorLoadingAndSavingUtils::SaveMap(World, PackagePath))
		{
			OutMessage = FString::Printf(TEXT("저장 실패: %s"), *PackagePath);
			return false;
		}
		OutMessage = FString::Printf(TEXT("생성: %s"), *PackagePath);
		return true;
	}

	bool BuildLevel(const FGlossaryEntry& Entry, FString& OutMessage)
	{
		const FString ClassPath = UGlossaryCatalog::GetDemoClassPath(Entry);
		UClass* DemoClass = LoadClass<AGlossaryDemoBase>(nullptr, *ClassPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!DemoClass)
		{
			OutMessage = FString::Printf(TEXT("건너뜀(클래스 없음): %s"), *ClassPath);
			return false;
		}
		return BuildMap(UGlossaryCatalog::GetLevelPackage(Entry), DemoClass, OutMessage);
	}

	bool BuildHubLevel()
	{
		FString Message;
		const bool bOk = BuildMap(UGlossaryCatalog::HubLevelPackage(), AGlossaryHubDemo::StaticClass(), Message);
		UE_LOG(LogGlossaryBuild, Display, TEXT("[Glossary] 허브 %s"), *Message);
		return bOk;
	}

	FResult BuildLevels(const FString& Filter)
	{
		FResult Result;
		for (const FGlossaryEntry& Entry : UGlossaryCatalog::GetEntries())
		{
			if (!MatchesFilter(Entry.Id, Filter))
			{
				continue;
			}
			FString Message;
			const FString ClassPath = UGlossaryCatalog::GetDemoClassPath(Entry);
			if (!LoadClass<AGlossaryDemoBase>(nullptr, *ClassPath, nullptr, LOAD_NoWarn | LOAD_Quiet))
			{
				++Result.Skipped;
				Result.Messages.Add(FString::Printf(TEXT("%s 건너뜀 (클래스 없음)"), *Entry.Id));
				continue;
			}
			if (BuildLevel(Entry, Message))
			{
				++Result.Built;
			}
			else
			{
				++Result.Failed;
			}
			Result.Messages.Add(FString::Printf(TEXT("%s %s"), *Entry.Id, *Message));
			UE_LOG(LogGlossaryBuild, Display, TEXT("[Glossary] %s %s"), *Entry.Id, *Message);
		}
		// 마지막에 빈 맵을 열어 둬서 방금 만든 레벨 패키지가 잠기지 않게 한다
		UEditorLoadingAndSavingUtils::NewBlankMap(false);
		return Result;
	}

	FResult BuildAll(const FString& Filter)
	{
		// 에셋 빌더: 필터가 분야 코드("05")면 그 분야 + 공통만
		if (Filter.IsEmpty())
		{
			GlossaryAssetKit::RunBuilders();
		}
		else
		{
			// 공통 에셋은 없을 때만 만든다(여러 분야 빌드가 M_Color 를 동시에 덮어쓰다 파일이 사라지는 것 방지)
			if (!FPackageName::DoesPackageExist(TEXT("/Game/Glossary/Common/M_Color")))
			{
				GlossaryAssetKit::RunBuilders(TEXT("00_Common"));
			}
			TSet<FString> Categories;
			TArray<FString> Parts;
			Filter.ParseIntoArray(Parts, TEXT(","), true);
			for (const FString& Part : Parts)
			{
				Categories.Add(Part.TrimStartAndEnd().Left(2));
			}
			for (const FString& Code : Categories)
			{
				for (const FString& Name : GlossaryAssetKit::GetBuilderNames())
				{
					if (Name.StartsWith(Code + TEXT("_")))
					{
						GlossaryAssetKit::RunBuilders(Name);
					}
				}
			}
		}
		// 허브는 전체 빌드일 때만 다시 만든다(분야별 빌드가 동시에 허브를 덮어쓰지 않게)
		if (Filter.IsEmpty() || !FPackageName::DoesPackageExist(UGlossaryCatalog::HubLevelPackage()))
		{
			BuildHubLevel();
		}
		return BuildLevels(Filter);
	}
}
