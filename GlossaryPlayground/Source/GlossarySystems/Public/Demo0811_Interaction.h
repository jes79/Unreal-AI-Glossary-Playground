#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GlossaryDemoBase.h"
#include "GlossaryKit.h"
#include "UObject/Interface.h"
#include "Demo0811_Interaction.generated.h"

class AGlossaryLabel;
class UD08_SidePanel;
class UStaticMeshComponent;

/** 상호작용 가능한 모든 것의 공통 규칙 (C++ 인터페이스) */
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UD0811_Interactable : public UInterface
{
	GENERATED_BODY()
};

class GLOSSARYSYSTEMS_API ID0811_Interactable
{
	GENERATED_BODY()

public:
	/** E 를 눌렀을 때 */
	virtual void Interact(AActor* InteractInstigator) = 0;
	/** 화면에 띄울 안내 ("문 열기") */
	virtual FString GetPrompt() const = 0;
	/** 바라보기 시작/끝 — 테두리(하이라이트) 표시 */
	virtual void SetFocused(bool bFocused) = 0;
};

/** 문·상자·NPC 의 공통 부모: 메시 하이라이트(Custom Depth + 발광)만 구현 */
UCLASS(Abstract)
class GLOSSARYSYSTEMS_API AD0811_InteractableBase : public AActor, public ID0811_Interactable
{
	GENERATED_BODY()

public:
	AD0811_InteractableBase();
	// UCLASS 는 CDO 를 만들어야 하므로 기본 구현을 둔다 (자식이 덮어씀)
	virtual void Interact(AActor* InteractInstigator) override {}
	virtual FString GetPrompt() const override { return FString(); }
	virtual void SetFocused(bool bFocused) override;

protected:
	virtual void BeginPlay() override;
	/** 하이라이트 대상 메시 등록 + Interact 트레이스 채널에 Block */
	UStaticMeshComponent* AddPart(GlossaryKit::EShape Shape, const FVector& Location, const FVector& Size, const FLinearColor& Color, USceneComponent* Parent = nullptr);

	UPROPERTY(VisibleAnywhere, Category = "Interaction") TObjectPtr<USceneComponent> Root;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	TArray<FLinearColor> PartColors;
	bool bIsFocused = false;
};

UCLASS()
class GLOSSARYSYSTEMS_API AD0811_Door : public AD0811_InteractableBase
{
	GENERATED_BODY()
public:
	AD0811_Door();
	virtual void Interact(AActor* InteractInstigator) override { bOpen = !bOpen; }
	virtual FString GetPrompt() const override { return bOpen ? TEXT("문 닫기") : TEXT("문 열기"); }
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> Hinge;
	bool bOpen = false;
	float Angle = 0.f;
};

UCLASS()
class GLOSSARYSYSTEMS_API AD0811_Chest : public AD0811_InteractableBase
{
	GENERATED_BODY()
public:
	AD0811_Chest();
	virtual void Interact(AActor* InteractInstigator) override;
	virtual FString GetPrompt() const override { return bOpen ? TEXT("상자 (비었음)") : TEXT("상자 열기"); }
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> LidHinge;
	bool bOpen = false;
	float Angle = 0.f;
};

UCLASS()
class GLOSSARYSYSTEMS_API AD0811_NPC : public AD0811_InteractableBase
{
	GENERATED_BODY()
public:
	AD0811_NPC();
	virtual void Interact(AActor* InteractInstigator) override;
	virtual FString GetPrompt() const override { return TEXT("말 걸기"); }
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UPROPERTY(Transient) TObjectPtr<AGlossaryLabel> Speech;
	int32 LineIndex = 0;
};

/**
 * 0811 상호작용 (Interface)
 * 카메라 중앙에서 Line Trace(Interact 채널) → 맞은 액터가 ID0811_Interactable 이면 하이라이트 + "E : …" 안내,
 * E 를 누르면 Interact 호출. 문·상자·NPC 는 서로 다른 클래스지만 같은 인터페이스로 다룬다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API ADemo0811_Interaction : public AGlossaryDemoBase
{
	GENERATED_BODY()

public:
	ADemo0811_Interaction();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 카메라에서 이 거리(cm)까지 트레이스 */
	UPROPERTY(EditAnywhere, Category = "Interaction") float TraceDistance = 900.f;

private:
	UPROPERTY(Transient) TWeakObjectPtr<AActor> Focused;
	UPROPERTY(Transient) TObjectPtr<UD08_SidePanel> Prompt;
	UPROPERTY(Transient) TObjectPtr<UD08_SidePanel> Crosshair;
	FString LastInteraction;
};
