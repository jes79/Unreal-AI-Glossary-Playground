#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "Demo0805_DataTable.h"
#include "Demo0806_DataAsset.h"
#include "Demo0813_Quest.h"
#include "Demo0815_LevelStreaming.h"
#include "Demo0818_Dialogue.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GlossaryAssetKit.h"
#include "GlossaryKit.h"
#include "Materials/MaterialInstanceConstant.h"

DEFINE_LOG_CATEGORY_STATIC(LogGlossarySystems, Log, All);

namespace
{
	const FString Folder = TEXT("/Game/Glossary/08_Systems/");

	/** CSV 문자열로 Data Table 에셋을 만든다 (에디터의 Import CSV 와 같은 경로) */
	void BuildDataTable(const FString& Name, UScriptStruct* RowStruct, const TCHAR* CSV)
	{
		UDataTable* Table = GlossaryAssetKit::CreateOrLoad<UDataTable>(Folder + Name);
		Table->RowStruct = RowStruct;
		const TArray<FString> Problems = Table->CreateTableFromCSVString(CSV);
		for (const FString& Problem : Problems)
		{
			UE_LOG(LogGlossarySystems, Error, TEXT("[Glossary] %s CSV 문제: %s"), *Name, *Problem);
		}
		GlossaryAssetKit::SaveAsset(Table);
		UE_LOG(LogGlossarySystems, Display, TEXT("[Glossary] %s 행 %d개"), *Name, Table->GetRowMap().Num());
	}

	void BuildWeapon(const FString& Name, const FString& WeaponName, float Damage, float FireRate,
		GlossaryKit::EShape Shape, const FVector& Size, const FLinearColor& Color)
	{
		UD0806_WeaponData* Data = GlossaryAssetKit::CreateOrLoad<UD0806_WeaponData>(Folder + Name);
		Data->WeaponName = WeaponName;
		Data->Damage = Damage;
		Data->FireRate = FireRate;
		Data->Mesh = GlossaryKit::GetShapeMesh(Shape);
		Data->MeshSize = Size;
		Data->Color = Color;
		GlossaryAssetKit::SaveAsset(Data);
	}

	/** 0815 서브레벨: 빈 맵에 색 상자로 '건물'을 짓고 저장 */
	void BuildSubLevel(const FString& PackagePath, const FVector& Center, UMaterialInterface* Material, bool bTower)
	{
		UWorld* World = UEditorLoadingAndSavingUtils::NewBlankMap(false);
		if (!World)
		{
			UE_LOG(LogGlossarySystems, Error, TEXT("[Glossary] NewBlankMap 실패: %s"), *PackagePath);
			return;
		}
		auto AddBox = [World, Material](GlossaryKit::EShape Shape, const FVector& Location, const FVector& Size)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator, Params);
			Actor->GetStaticMeshComponent()->SetStaticMesh(GlossaryKit::GetShapeMesh(Shape));
			Actor->GetStaticMeshComponent()->SetMaterial(0, Material);
			Actor->SetActorScale3D(Size / 100.f);
		};
		if (bTower)
		{
			// 탑 + 주변 기둥
			AddBox(GlossaryKit::EShape::Cube, Center + FVector(0.f, 0.f, 300.f), FVector(300.f, 300.f, 600.f));
			AddBox(GlossaryKit::EShape::Cone, Center + FVector(0.f, 0.f, 700.f), FVector(380.f, 380.f, 200.f));
			for (int32 i = 0; i < 4; ++i)
			{
				const float A = PI * 0.5f * i;
				AddBox(GlossaryKit::EShape::Cylinder, Center + FVector(FMath::Cos(A) * 450.f, FMath::Sin(A) * 450.f, 150.f), FVector(80.f, 80.f, 300.f));
			}
		}
		else
		{
			// 나무 숲 (원기둥 줄기 + 구 잎)
			for (int32 i = 0; i < 7; ++i)
			{
				const FVector P = Center + FVector((i % 3 - 1) * 300.f, (i / 3 - 1) * 300.f + (i % 2) * 80.f, 0.f);
				AddBox(GlossaryKit::EShape::Cylinder, P + FVector(0.f, 0.f, 120.f), FVector(50.f, 50.f, 240.f));
				AddBox(GlossaryKit::EShape::Sphere, P + FVector(0.f, 0.f, 300.f), FVector(200.f));
			}
		}
		if (!UEditorLoadingAndSavingUtils::SaveMap(World, PackagePath))
		{
			UE_LOG(LogGlossarySystems, Error, TEXT("[Glossary] 서브레벨 저장 실패: %s"), *PackagePath);
		}
		else
		{
			UE_LOG(LogGlossarySystems, Display, TEXT("[Glossary] 서브레벨 생성: %s"), *PackagePath);
		}
	}
}

/** 08_Systems 분야 데모가 쓰는 생성 에셋 (/Game/Glossary/08_Systems) */
static void BuildGlossarySystemsAssets()
{
	using namespace GlossaryKit;

	// 0805 Data Table: 아이템 표
	BuildDataTable(TEXT("DT_0805_Items"), FD0805_ItemRow::StaticStruct(), ADemo0805_DataTable::ItemsCSV);
	// 0818 Data Table: 대화 표
	BuildDataTable(TEXT("DT_0818_Dialogue"), FD0818_DialogueRow::StaticStruct(), ADemo0818_Dialogue::DialogueCSV);

	// 0806 무기 Data Asset 3개 — 같은 클래스, 다른 값
	BuildWeapon(TEXT("DA_0806_Pistol"), TEXT("권총"), 12.f, 2.f, EShape::Cylinder, FVector(40.f, 12.f, 12.f), FLinearColor(0.2f, 0.2f, 0.22f));
	BuildWeapon(TEXT("DA_0806_Rifle"), TEXT("소총"), 6.f, 8.f, EShape::Cube, FVector(90.f, 8.f, 12.f), FLinearColor(0.15f, 0.4f, 0.2f));
	BuildWeapon(TEXT("DA_0806_Shotgun"), TEXT("샷건"), 45.f, 0.8f, EShape::Cylinder, FVector(70.f, 20.f, 20.f), FLinearColor(0.55f, 0.3f, 0.1f));

	// 0813 퀘스트 Data Asset
	{
		UD0813_QuestData* Quest = GlossaryAssetKit::CreateOrLoad<UD0813_QuestData>(Folder + TEXT("DA_0813_SlimeHunt"));
		Quest->Title = TEXT("슬라임 사냥");
		Quest->Description = TEXT("마을 근처에 늘어난 슬라임을 5마리 처치하라.");
		Quest->TargetType = TEXT("Slime");
		Quest->RequiredCount = 5;
		Quest->RewardGold = 100;
		Quest->RewardItem = TEXT("체력 물약 ×2");
		GlossaryAssetKit::SaveAsset(Quest);
	}

	// 0815 서브레벨 2개 (+ 색 머티리얼 인스턴스)
	UMaterialInterface* Base = GetColorMaterial();
	UMaterialInstanceConstant* MatA = GlossaryAssetKit::CreateMaterialInstance(Folder + TEXT("MI_0815_SubA"), Base,
		{ { FName(TEXT("Roughness")), 0.5f } }, { { FName(TEXT("Color")), Colors::Cyan } });
	UMaterialInstanceConstant* MatB = GlossaryAssetKit::CreateMaterialInstance(Folder + TEXT("MI_0815_SubB"), Base,
		{ { FName(TEXT("Roughness")), 0.5f } }, { { FName(TEXT("Color")), FLinearColor(0.2f, 0.65f, 0.25f) } });
	BuildSubLevel(ADemo0815_LevelStreaming::SubLevelA, FVector(1500.f, -900.f, 0.f), MatA, true);
	BuildSubLevel(ADemo0815_LevelStreaming::SubLevelB, FVector(1500.f, 900.f, 0.f), MatB, false);
	// 레벨 빌더가 다음 맵을 만들 때 방금 저장한 서브레벨을 잡고 있지 않도록 빈 맵으로 바꿔 둔다
	UEditorLoadingAndSavingUtils::NewBlankMap(false);
}
#endif

class FGlossarySystemsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::RegisterBuilder(TEXT("08_Systems"), &BuildGlossarySystemsAssets);
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		GlossaryAssetKit::UnregisterBuilder(TEXT("08_Systems"));
#endif
	}
};

IMPLEMENT_MODULE(FGlossarySystemsModule, GlossarySystems);
