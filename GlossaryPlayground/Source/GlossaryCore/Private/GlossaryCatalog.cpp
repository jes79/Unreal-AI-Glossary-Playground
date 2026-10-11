#include "GlossaryCatalog.h"

int32 FGlossaryEntry::GetCategoryIndex() const
{
	return FCString::Atoi(*Id.Left(2)) - 1;
}

const TArray<FGlossaryEntry>& UGlossaryCatalog::GetEntries()
{
	static const TArray<FGlossaryEntry> Entries = []()
	{
		TArray<FGlossaryEntry> Result;
#define GLOSSARY_ENTRY(InId, InName, InTerm, InPlain, InRequest) \
		{ FGlossaryEntry E; E.Id = TEXT(InId); E.Name = TEXT(InName); E.Term = TEXT(InTerm); E.Plain = TEXT(InPlain); E.Request = TEXT(InRequest); Result.Add(E); }
#include "GlossaryCatalogData.inl"
#undef GLOSSARY_ENTRY
		return Result;
	}();
	return Entries;
}

const TArray<FGlossaryCategory>& UGlossaryCatalog::GetCategories()
{
	static const TArray<FGlossaryCategory> Categories = []()
	{
		auto Make = [](const TCHAR* Code, const TCHAR* Folder, const TCHAR* Title, const TCHAR* Module)
		{
			FGlossaryCategory C; C.Code = Code; C.Folder = Folder; C.Title = Title; C.Module = Module; return C;
		};
		return TArray<FGlossaryCategory>{
			Make(TEXT("01"), TEXT("01_Movement"),  TEXT("캐릭터 이동·조작·카메라"), TEXT("GlossaryMovement")),
			Make(TEXT("02"), TEXT("02_Physics"),   TEXT("물리·충돌 (Chaos)"), TEXT("GlossaryPhysics")),
			Make(TEXT("03"), TEXT("03_Animation"), TEXT("애니메이션"), TEXT("GlossaryAnimation")),
			Make(TEXT("04"), TEXT("04_Material"),  TEXT("머티리얼·셰이더"), TEXT("GlossaryMaterial")),
			Make(TEXT("05"), TEXT("05_Effects"),   TEXT("이펙트 (Niagara·포스트 프로세스)"), TEXT("GlossaryEffects")),
			Make(TEXT("06"), TEXT("06_UI"),        TEXT("UI·UX (UMG)"), TEXT("GlossaryUI")),
			Make(TEXT("07"), TEXT("07_AI"),        TEXT("적 AI·길찾기"), TEXT("GlossaryAI")),
			Make(TEXT("08"), TEXT("08_Systems"),   TEXT("게임 시스템·프레임워크"), TEXT("GlossarySystems")),
			Make(TEXT("09"), TEXT("09_Audio"),     TEXT("오디오"), TEXT("GlossaryAudio")),
			Make(TEXT("10"), TEXT("10_Rendering"), TEXT("조명·렌더링·최적화"), TEXT("GlossaryRendering")),
			Make(TEXT("11"), TEXT("11_Patterns"),  TEXT("코드 구조·블루프린트·에디터"), TEXT("GlossaryPatterns")),
		};
	}();
	return Categories;
}

const FGlossaryEntry* UGlossaryCatalog::Find(const FString& Id)
{
	return GetEntries().FindByPredicate([&Id](const FGlossaryEntry& E) { return E.Id == Id; });
}

bool UGlossaryCatalog::FindEntry(const FString& Id, FGlossaryEntry& OutEntry)
{
	if (const FGlossaryEntry* Found = Find(Id))
	{
		OutEntry = *Found;
		return true;
	}
	return false;
}

const FGlossaryCategory& UGlossaryCatalog::GetCategory(const FGlossaryEntry& Entry)
{
	const TArray<FGlossaryCategory>& Categories = GetCategories();
	return Categories[FMath::Clamp(Entry.GetCategoryIndex(), 0, Categories.Num() - 1)];
}

FString UGlossaryCatalog::GetLevelPackage(const FGlossaryEntry& Entry)
{
	return FString::Printf(TEXT("/Game/Levels/%s/L_%s_%s"), *GetCategory(Entry).Folder, *Entry.Id, *Entry.Name);
}

FString UGlossaryCatalog::GetDemoClassPath(const FGlossaryEntry& Entry)
{
	return FString::Printf(TEXT("/Script/%s.Demo%s_%s"), *GetCategory(Entry).Module, *Entry.Id, *Entry.Name);
}

FString UGlossaryCatalog::GetAssetFolder(const FString& Id)
{
	if (const FGlossaryEntry* Entry = Find(Id))
	{
		return FString::Printf(TEXT("/Game/Glossary/%s"), *GetCategory(*Entry).Folder);
	}
	return TEXT("/Game/Glossary/Common");
}
