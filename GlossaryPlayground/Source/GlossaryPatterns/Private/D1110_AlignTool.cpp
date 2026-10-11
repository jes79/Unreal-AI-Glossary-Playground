// 1110 에디터 도구: "선택한 액터를 격자로 정렬하고 이름을 일괄 변경"
// Editor Utility Widget(WBP) 대신 C++ 로 같은 도구를 만든다. 전부 WITH_EDITOR 안이라 패키지 게임에는 들어가지 않는다.
//   - 메뉴:  Tools > Glossary > Patterns > Align Selected / Open Align Tool...   (ToolMenus)
//   - 창:    'Glossary Align Tool' 노매드 탭 (Slate)                            (FGlobalTabmanager)
//   - 동작:  UEditorActorSubsystem 으로 선택 액터 읽기 → 이동 → SetActorLabel, FScopedTransaction 으로 Undo 지원
#include "Demo1110_EditorUtility.h"

#if WITH_EDITOR

#include "D11_SidePanel.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/IConsoleManager.h"
#include "ScopedTransaction.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "GlossaryPatternsAlignTool"

namespace D1110_EditorTool
{
	static const FName TabName(TEXT("GlossaryAlignTool"));
	static const FName MenuOwner(TEXT("GlossaryPatterns"));
	static FDelegateHandle StartupHandle;
	static bool bTabRegistered = false;

	/** 창에서 고친 값(세션 동안 유지) */
	static int32 ToolColumns = 4;
	static float ToolSpacing = 250.f;
	static FString ToolPrefix = TEXT("Crate");

	static void Notify(const FString& Text, bool bSuccess)
	{
		FNotificationInfo Info(FText::FromString(Text));
		Info.ExpireDuration = 4.f;
		TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Info);
		if (Item.IsValid())
		{
			Item->SetCompletionState(bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
		}
	}

	static int32 NumSelected()
	{
		UEditorActorSubsystem* Actors = GEditor ? GEditor->GetEditorSubsystem<UEditorActorSubsystem>() : nullptr;
		return Actors ? Actors->GetSelectedLevelActors().Num() : 0;
	}

	int32 AlignSelected(int32 Columns, float Spacing, const FString& Prefix)
	{
		UEditorActorSubsystem* ActorSubsystem = GEditor ? GEditor->GetEditorSubsystem<UEditorActorSubsystem>() : nullptr;
		if (!ActorSubsystem)
		{
			return 0;
		}
		TArray<AActor*> Selected = ActorSubsystem->GetSelectedLevelActors();
		if (Selected.Num() == 0)
		{
			Notify(TEXT("정렬할 액터를 먼저 선택하세요"), false);
			return 0;
		}

		// 격자 시작점 = 선택 영역의 최소 X·Y (높이는 평균)
		FVector Origin(TNumericLimits<float>::Max(), TNumericLimits<float>::Max(), 0.f);
		for (const AActor* Actor : Selected)
		{
			const FVector L = Actor->GetActorLocation();
			Origin.X = FMath::Min(Origin.X, L.X);
			Origin.Y = FMath::Min(Origin.Y, L.Y);
			Origin.Z += L.Z / Selected.Num();
		}

		D1110_GridAlign::SortForAlign(Selected);
		const TArray<FVector> Grid = D1110_GridAlign::ComputeGrid(Origin, Selected.Num(), Columns, Spacing);

		// 한 번의 Ctrl+Z 로 전부 되돌아가도록 트랜잭션 하나로 묶는다
		const FScopedTransaction Transaction(LOCTEXT("AlignSelected", "Glossary: Align Selected to Grid"));
		for (int32 i = 0; i < Selected.Num(); ++i)
		{
			AActor* Actor = Selected[i];
			Actor->Modify(); // Undo 버퍼에 이전 상태 기록
			Actor->SetActorLocationAndRotation(Grid[i], FRotator::ZeroRotator);
			Actor->SetActorLabel(D1110_GridAlign::MakeName(Prefix, i + 1));
		}
		GEditor->RedrawLevelEditingViewports();
		Notify(FString::Printf(TEXT("%d개 액터를 %d열 격자로 정렬하고 %s_01.. 로 이름 변경"), Selected.Num(), Columns, *Prefix), true);
		UE_LOG(LogGlossaryPatterns, Log, TEXT("AlignSelected: %d actors, columns %d, spacing %.0f, prefix %s"), Selected.Num(), Columns, Spacing, *Prefix);
		return Selected.Num();
	}

	/** 'Glossary Align Tool' 창: Editor Utility Widget 에서 디자이너로 만들 화면을 Slate 로 직접 쓴 것 */
	static TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args)
	{
		auto Row = [](const FText& Label, const TSharedRef<SWidget>& Field)
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.4f).VAlign(VAlign_Center)[SNew(STextBlock).Text(Label)]
				+ SHorizontalBox::Slot().FillWidth(0.6f)[Field];
		};

		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SNew(SBox).Padding(12.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
					[
						SNew(STextBlock).Text(LOCTEXT("Title", "선택한 액터를 격자로 정렬하고 이름을 일괄 변경합니다 (1110)"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
					[
						Row(LOCTEXT("Columns", "열 수"), SNew(SSpinBox<int32>).MinValue(1).MaxValue(20)
							.Value_Lambda([] { return ToolColumns; })
							.OnValueChanged_Lambda([](int32 V) { ToolColumns = V; }))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
					[
						Row(LOCTEXT("Spacing", "간격 (cm)"), SNew(SSpinBox<float>).MinValue(10.f).MaxValue(5000.f)
							.Value_Lambda([] { return ToolSpacing; })
							.OnValueChanged_Lambda([](float V) { ToolSpacing = V; }))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
					[
						Row(LOCTEXT("Prefix", "이름 접두사"), SNew(SEditableTextBox)
							.Text_Lambda([] { return FText::FromString(ToolPrefix); })
							.OnTextCommitted_Lambda([](const FText& T, ETextCommit::Type) { ToolPrefix = T.ToString(); }))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 4)
					[
						SNew(STextBlock).Text_Lambda([] { return FText::Format(LOCTEXT("SelCount", "선택된 액터: {0}개"), NumSelected()); })
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SButton)
						.Text(LOCTEXT("Run", "정렬 + 이름 변경 실행"))
						.IsEnabled_Lambda([] { return NumSelected() > 0; })
						.OnClicked_Lambda([]
						{
							AlignSelected(ToolColumns, ToolSpacing, ToolPrefix);
							return FReply::Handled();
						})
					]
				]
			];
	}

	static void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(MenuOwner);
		// GlossaryEditor 모듈이 만든 Tools > Glossary 하위 메뉴에 'Patterns' 를 덧붙인다
		UToolMenu* GlossaryMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools.GlossaryMenu");
		FToolMenuSection& Section = GlossaryMenu->FindOrAddSection("GlossaryPatterns", LOCTEXT("PatternsSection", "11 Patterns"));
		Section.AddSubMenu("Patterns", LOCTEXT("PatternsMenu", "Patterns"), LOCTEXT("PatternsMenuTip", "11 분야 에디터 도구 (1110)"),
			FNewToolMenuDelegate::CreateLambda([](UToolMenu* SubMenu)
			{
				FToolMenuSection& Tools = SubMenu->AddSection("AlignTools", LOCTEXT("AlignTools", "Align"));
				Tools.AddMenuEntry("AlignSelected", LOCTEXT("AlignSelected", "Align Selected"),
					LOCTEXT("AlignSelectedTip", "선택한 액터를 격자로 정렬하고 이름을 접두사_01.. 로 바꾼다 (창의 현재 설정 사용)"), FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([] { AlignSelected(ToolColumns, ToolSpacing, ToolPrefix); })));
				Tools.AddMenuEntry("OpenAlignTool", LOCTEXT("OpenAlignTool", "Open Align Tool..."),
					LOCTEXT("OpenAlignToolTip", "열 수·간격·이름 접두사를 정하는 도구 창 열기"), FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([] { FGlobalTabmanager::Get()->TryInvokeTab(TabName); })));
			}));
	}

	// 콘솔에서도 실행 가능 (자동화·검증용)
	static FAutoConsoleCommand CmdAlign(
		TEXT("Glossary.AlignSelected"),
		TEXT("1110: 선택한 액터를 격자로 정렬하고 이름 일괄 변경 (에디터 전용)"),
		FConsoleCommandDelegate::CreateLambda([] { AlignSelected(ToolColumns, ToolSpacing, ToolPrefix); }));
	static FAutoConsoleCommand CmdOpen(
		TEXT("Glossary.OpenAlignTool"),
		TEXT("1110: Glossary Align Tool 창 열기 (에디터 전용)"),
		FConsoleCommandDelegate::CreateLambda([] { if (GIsEditor) { FGlobalTabmanager::Get()->TryInvokeTab(TabName); } }));

	void Register()
	{
		// 커맨드렛(레벨 빌더)과 -game 실행에서는 에디터 UI 가 없으므로 등록하지 않는다
		if (!GIsEditor || IsRunningCommandlet())
		{
			return;
		}
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabName, FOnSpawnTab::CreateStatic(&SpawnTab))
			.SetDisplayName(LOCTEXT("TabTitle", "Glossary Align Tool"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);
		bTabRegistered = true;
		StartupHandle = UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateStatic(&RegisterMenus));
	}

	void Unregister()
	{
		if (StartupHandle.IsValid())
		{
			UToolMenus::UnRegisterStartupCallback(StartupHandle);
			StartupHandle.Reset();
		}
		if (UObjectInitialized() && UToolMenus::TryGet())
		{
			UToolMenus::UnregisterOwner(MenuOwner);
		}
		if (bTabRegistered && FSlateApplication::IsInitialized())
		{
			FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
			bTabRegistered = false;
		}
	}
}

#undef LOCTEXT_NAMESPACE

#endif // WITH_EDITOR
