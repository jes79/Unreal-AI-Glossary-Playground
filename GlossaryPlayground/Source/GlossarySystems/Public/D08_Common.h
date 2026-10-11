#pragma once

#include "CoreMinimal.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GlossaryLabel.h"
#include "GlossaryWidgets.h"
#include "D08_Common.generated.h"

class UTextBlock;
class UProgressBar;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APlayerController;

/**
 * 08 분야 데모가 같이 쓰는 화면 상자: 제목 + 본문 + (선택) 진행 막대.
 * 정보 패널(왼쪽 위 620×440)을 피해 오른쪽 위 또는 아래 가운데에 놓는다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API UD08_SidePanel : public UGlossaryWidgetBase
{
	GENERATED_BODY()

public:
	enum class EPlace : uint8 { TopRight, BottomRight, BottomCenter, Center };

	/** 만들어서 뷰포트에 올린다 */
	static UD08_SidePanel* Create(APlayerController* PC, const FString& Title, EPlace Place = EPlace::TopRight, float Width = 420.f);

	void SetTitle(const FString& InTitle);
	void SetBody(const FString& InBody);
	/** Percent 0~1. 음수면 막대를 숨긴다 */
	void SetProgress(float Percent, const FLinearColor& Color = FLinearColor(0.2f, 0.8f, 0.3f));

protected:
	virtual void BuildTree() override;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BodyText;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> Bar;

	EPlace Place = EPlace::TopRight;
	float Width = 420.f;
	FString PendingTitle;
	FString PendingBody;
	float PendingPercent = -1.f;
	FLinearColor PendingColor = FLinearColor(0.2f, 0.8f, 0.3f);
};

/** 위로 떠오르다 사라지는 숫자/글자 (데미지 숫자 등) */
UCLASS()
class GLOSSARYSYSTEMS_API AD08_FloatingText : public AGlossaryLabel
{
	GENERATED_BODY()

public:
	AD08_FloatingText();

	static AD08_FloatingText* Spawn(UWorld* World, const FString& Text, const FVector& Location,
		const FLinearColor& Color = FLinearColor::White, float FontSize = 24.f, float LifeTime = 1.2f);

protected:
	virtual void Tick(float DeltaSeconds) override;

	float RiseSpeed = 70.f;
};

/**
 * 맞으면 체력이 깎이는 표적(원기둥). 머리 위 라벨에 체력, 맞을 때 숫자가 뜬다.
 * 0 이 되면 잠깐 쓰러졌다가 다시 채워진다(데모 반복용).
 */
UCLASS()
class GLOSSARYSYSTEMS_API AD08_Dummy : public AActor
{
	GENERATED_BODY()

public:
	AD08_Dummy();

	UPROPERTY(EditAnywhere, Category = "Dummy") FString DisplayName = TEXT("표적");
	UPROPERTY(EditAnywhere, Category = "Dummy") float MaxHealth = 100.f;
	UPROPERTY(VisibleAnywhere, Category = "Dummy") float Health = 100.f;
	UPROPERTY(EditAnywhere, Category = "Dummy") FLinearColor BaseColor = FLinearColor(0.9f, 0.12f, 0.1f);
	UPROPERTY(EditAnywhere, Category = "Dummy") float RespawnDelay = 2.f;

	/** 체력을 Amount 만큼 깎고 숫자를 띄운다. Note 는 숫자 옆에 붙는 설명(예: "화염 ×2") */
	virtual void ApplyHit(float Amount, const FLinearColor& Color, const FString& Note = FString());
	void ResetHealth();
	/** 스폰 뒤(BeginPlay 이후) 이름·체력·색을 바꾸고 다시 채운다 */
	void Configure(const FString& InName, float InMaxHealth, const FLinearColor& InColor);
	void Flash(const FLinearColor& Color, float Duration = 0.15f);
	void SetBodyColor(const FLinearColor& Color, float Emissive = 0.f);
	bool IsDead() const { return Health <= 0.f; }
	UStaticMeshComponent* GetBody() const { return Body; }
	/** 최근 Window 초 동안 받은 데미지 합 (DPS 계산용) */
	float GetRecentDamage(float Window) const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	void RefreshLabel();

	UPROPERTY(VisibleAnywhere, Category = "Dummy") TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BodyMID;
	UPROPERTY(Transient) TObjectPtr<AGlossaryLabel> HpLabel;

	float FlashTimer = 0.f;
	float DeadTimer = 0.f;
	TArray<TPair<float, float>> DamageLog; // (시각, 양)
};

/**
 * 목표 지점까지 날아가서 도착하면 OnArrive 를 부르는 단순 투사체(구).
 * 물리 충돌 대신 목표 액터를 따라가므로 데모에서 항상 맞는다.
 */
UCLASS()
class GLOSSARYSYSTEMS_API AD08_Projectile : public AActor
{
	GENERATED_BODY()

public:
	AD08_Projectile();

	static AD08_Projectile* Launch(UWorld* World, const FVector& From, AActor* Target, const FLinearColor& Color,
		float Size, float Speed, TFunction<void(AActor*)> OnArrive);

	TWeakObjectPtr<AActor> Target;
	float Speed = 1500.f;
	TFunction<void(AActor*)> OnArrive;

protected:
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Projectile") TObjectPtr<UStaticMeshComponent> Mesh;
};

namespace D08
{
	/** 플레이어 폰 기준 가장 가까운 T 액터 */
	template <typename T>
	T* FindNearest(UWorld* World, const FVector& From, float MaxDistance = 1e9f);

	/** 초 → "12.3초" */
	inline FString Sec(float Seconds) { return FString::Printf(TEXT("%.1f초"), Seconds); }

	/** ■■■□□ 형태 막대 문자열 */
	GLOSSARYSYSTEMS_API FString Bar(float Percent, int32 Cells = 10);
}


template <typename T>
T* D08::FindNearest(UWorld* World, const FVector& From, float MaxDistance)
{
	T* Best = nullptr;
	float BestDist = MaxDistance;
	for (TActorIterator<T> It(World); It; ++It)
	{
		const float Dist = FVector::Dist(It->GetActorLocation(), From);
		if (Dist < BestDist)
		{
			BestDist = Dist;
			Best = *It;
		}
	}
	return Best;
}
