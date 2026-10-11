#pragma once

#include "CoreMinimal.h"
#include "GlossaryDemoBase.h"
#include "GlossaryWidgets.h"
#include "D11_SidePanel.h"
#include "Demo1111_PythonScripting.generated.h"

class UTexture2D;
class UTextBlock;
class UBorder;

/** 텍스처 썸네일 + 현재 압축 설정을 보여 주는 줄 (화면 아래) */
UCLASS()
class GLOSSARYPATTERNS_API UD1111_TextureStrip : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	void SetTextures(const TArray<UTexture2D*>& InTextures);
	/** 스크립트 규칙에 걸리는 텍스처를 강조 (미리보기) */
	void SetHighlight(const TArray<bool>& InHighlight);
	void Refresh();

protected:
	virtual void BuildTree() override;

	UPROPERTY(Transient) TArray<TObjectPtr<UTexture2D>> Textures;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> Captions;
	UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> Frames;
	TArray<bool> Highlight;
};

/**
 * 1111 Python 에디터 스크립팅
 * Content/Python/glossary_texture_normalmap.py 가 /Game/Glossary/11_Patterns/Textures 의 텍스처를 모두 찾아
 * 이름이 _N 으로 끝나거나 Normal 이 들어간 것의 CompressionSettings 를 TC_Normalmap 으로 바꾼다.
 * 레벨은 그 텍스처들의 현재 설정을 실행 중에 읽어 보여 준다 → 에디터에서 스크립트를 돌린 뒤 다시 실행하면 값이 바뀌어 있다.
 */
UCLASS()
class GLOSSARYPATTERNS_API ADemo1111_PythonScripting : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo1111_PythonScripting();

	/** 스크립트와 같은 판정 규칙(C++ 판) */
	static bool IsNormalMapName(const FString& AssetName);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(Transient) TObjectPtr<UD1111_TextureStrip> Strip;
	UPROPERTY(Transient) TObjectPtr<UD11_SidePanel> Panel;
	UPROPERTY(Transient) TArray<TObjectPtr<UTexture2D>> Textures;
	bool bPreview = false;
};
