#include "Editor.h"
#include "FileHelpers.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GlossaryAssetKit.h"
#include "GlossaryCatalog.h"
#include "GlossaryLevelBuilder.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "GlossaryEditor"

/** Tools > Glossary 메뉴. Unity 판의 Tools > Glossary 와 같은 구성 */
class FGlossaryEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FGlossaryEditorModule::RegisterMenus));
	}

	virtual void ShutdownModule() override
	{
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
	}

private:
	static void Notify(const FString& Text)
	{
		FNotificationInfo Info(FText::FromString(Text));
		Info.ExpireDuration = 6.f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}

	static void RunBuild(const FString& Filter)
	{
		const GlossaryLevelBuilder::FResult Result = GlossaryLevelBuilder::BuildAll(Filter);
		Notify(FString::Printf(TEXT("Glossary 빌드 완료: 생성 %d · 건너뜀 %d · 실패 %d"), Result.Built, Result.Skipped, Result.Failed));
		UEditorLoadingAndSavingUtils::LoadMap(UGlossaryCatalog::HubLevelPackage());
	}

	void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(this);
		UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
		FToolMenuSection& Section = ToolsMenu->FindOrAddSection("Glossary", LOCTEXT("GlossarySection", "Glossary"));

		Section.AddSubMenu("GlossaryMenu", LOCTEXT("GlossaryMenu", "Glossary"), LOCTEXT("GlossaryMenuTip", "언리얼 AI 요청 용어집 예제 레벨"),
			FNewToolMenuDelegate::CreateLambda([](UToolMenu* Menu)
			{
				FToolMenuSection& Main = Menu->AddSection("Main");
				Main.AddMenuEntry("BuildAll", LOCTEXT("BuildAll", "Build All Levels"),
					LOCTEXT("BuildAllTip", "생성 에셋과 예제 레벨 전체를 코드로 다시 만든다"), FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([] { RunBuild(FString()); })));
				Main.AddMenuEntry("BuildAssets", LOCTEXT("BuildAssets", "Build Assets Only"),
					LOCTEXT("BuildAssetsTip", "머티리얼·텍스처·사운드 같은 생성 에셋만 다시 만든다"), FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([] { GlossaryAssetKit::RunBuilders(); Notify(TEXT("Glossary 에셋 빌드 완료")); })));
				Main.AddMenuEntry("OpenHub", LOCTEXT("OpenHub", "Open Hub Level"),
					LOCTEXT("OpenHubTip", "L_0000_Hub 열기"), FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([] { UEditorLoadingAndSavingUtils::LoadMap(UGlossaryCatalog::HubLevelPackage()); })));

				Main.AddSubMenu("BuildCategory", LOCTEXT("BuildCategory", "Build Category"), LOCTEXT("BuildCategoryTip", "분야 하나만 다시 만들기"),
					FNewToolMenuDelegate::CreateLambda([](UToolMenu* SubMenu)
					{
						FToolMenuSection& Categories = SubMenu->AddSection("Categories");
						for (const FGlossaryCategory& Category : UGlossaryCatalog::GetCategories())
						{
							const FString Code = Category.Code;
							Categories.AddMenuEntry(FName(*Category.Folder), FText::FromString(Category.Folder + TEXT("  ") + Category.Title), FText::GetEmpty(), FSlateIcon(),
								FUIAction(FExecuteAction::CreateLambda([Code] { RunBuild(Code); })));
						}
					}));
			}));
	}
};

IMPLEMENT_MODULE(FGlossaryEditorModule, GlossaryEditor);

#undef LOCTEXT_NAMESPACE
