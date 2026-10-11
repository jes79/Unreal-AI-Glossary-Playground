#pragma once

#include "CoreMinimal.h"

class UWidgetTree;
class UTextBlock;
class UBorder;
class UButton;
class UProgressBar;
class USizeBox;
class UWidget;

/**
 * 06 UI 분야 데모끼리 공유하는 작은 위젯 헬퍼.
 * UGlossaryWidgetBase 를 상속하지 않는 위젯(UUserWidget 직접 상속 · Common UI 위젯)에서도 쓰도록 WidgetTree 를 받는다.
 */
namespace GlossaryUIKit
{
	GLOSSARYUI_API UTextBlock* MakeText(UWidgetTree* Tree, const FString& Text, float Size = 14.f,
		const FLinearColor& Color = FLinearColor::White, bool bBold = false);

	GLOSSARYUI_API UBorder* MakeBorder(UWidgetTree* Tree, const FLinearColor& Background, const FMargin& Padding = FMargin(12.f));

	GLOSSARYUI_API UButton* MakeButton(UWidgetTree* Tree, const FString& Label, float FontSize = 16.f,
		const FLinearColor& Background = FLinearColor(0.12f, 0.14f, 0.18f, 0.95f));

	/** 단색 채우기 ProgressBar (Fill / Background 를 색 브러시로 바꿔 모양을 단순하게) */
	GLOSSARYUI_API UProgressBar* MakeBar(UWidgetTree* Tree, const FLinearColor& Fill, const FLinearColor& Background);

	/** 고정 크기 상자로 감싼다 (Width/Height 가 0 이면 그 축은 자유) */
	GLOSSARYUI_API USizeBox* WrapSize(UWidgetTree* Tree, UWidget* Content, float Width, float Height);
}

class AActor;
namespace GlossaryUIKit
{
	/** 고정 카메라(ACameraActor)를 스폰해 플레이어 시점으로 지정한다. UI 데모처럼 폰이 필요 없는 레벨용 */
	GLOSSARYUI_API void SetupFixedCamera(AActor* WorldContext, const FVector& Location, const FRotator& Rotation, float FOV = 90.f);
}
