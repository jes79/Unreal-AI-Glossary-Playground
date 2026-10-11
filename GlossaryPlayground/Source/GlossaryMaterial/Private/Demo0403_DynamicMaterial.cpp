#include "Demo0403_DynamicMaterial.h"

#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace GlossaryKit;

ADemo0403_DynamicMaterial::ADemo0403_DynamicMaterial()
{
	DemoId = TEXT("0403");
	Controls = TEXT("Space / 왼쪽 클릭 : 적을 때린다 (Emissive 0.1초 빨강)   A : 자동 피격 ON/OFF\n")
		TEXT("T : 슬로 모션(×0.05) — 0.1초 플래시를 천천히 보기   F : 플래시 상태 고정(확인용)\n")
		TEXT("관찰: 에셋은 그대로, 실행 중 만든 Dynamic Material Instance 의 FlashAmount 만 0↔1");
	Showcase = TEXT("피격 시 CreateDynamicMaterialInstance 로 만든 MID 의 Emissive 를 0.1초 빨갛게 깜빡임");
}

void ADemo0403_DynamicMaterial::BeginPlay()
{
	Super::BeginPlay();
	SetupFixedCamera(FVector(-520.f, -60.f, 170.f), FVector(0.f, -230.f, 110.f), 55.f);

	Enemy = SpawnMannequin(FVector(0.f, 0.f, 0.f), FRotator(0.f, 90.f, 0.f), LoadMaterial(TEXT("M_HitFlash")));
	USkeletalMeshComponent* Mesh = Enemy->GetSkeletalMeshComponent();
	// 핵심: 슬롯마다 Dynamic Material Instance 를 만들어 끼운다 (원본 에셋은 건드리지 않는다)
	for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
	{
		UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(Slot);
		MID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.45f, 0.12f, 0.1f));
		FlashMaterials.Add(MID);
	}
	SpawnLabel(GetWorld(), TEXT("적 (M_HitFlash 의 MID)"), FVector(0.f, 0.f, 210.f), Colors::Enemy, 16.f);
}

void ADemo0403_DynamicMaterial::Hit()
{
	++HitCount;
	FlashRemaining = FlashDuration;
}

void ADemo0403_DynamicMaterial::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (WasKeyPressed(EKeys::SpaceBar) || WasKeyPressed(EKeys::LeftMouseButton)) { Hit(); }
	if (WasKeyPressed(EKeys::A)) { bAutoHit = !bAutoHit; AutoHitTimer = 0.f; }
	if (WasKeyPressed(EKeys::F)) { bHoldFlash = !bHoldFlash; }
	if (WasKeyPressed(EKeys::T))
	{
		bSlowMotion = !bSlowMotion;
		UGameplayStatics::SetGlobalTimeDilation(this, bSlowMotion ? SlowMotionScale : 1.f);
	}

	if (bAutoHit)
	{
		AutoHitTimer += DeltaSeconds;
		if (AutoHitTimer >= AutoHitInterval)
		{
			AutoHitTimer = 0.f;
			Hit();
		}
	}

	FlashRemaining = FMath::Max(0.f, FlashRemaining - DeltaSeconds);
	const float FlashAmount = (FlashRemaining > 0.f || bHoldFlash) ? 1.f : 0.f;
	for (UMaterialInstanceDynamic* MID : FlashMaterials)
	{
		MID->SetScalarParameterValue(TEXT("FlashAmount"), FlashAmount);
	}

	SetStatus(FString::Printf(TEXT("피격 %d회 · FlashAmount %.0f (남은 %.3f초 / %.2f초)%s%s%s"),
		HitCount, FlashAmount, FlashRemaining, FlashDuration,
		bAutoHit ? TEXT(" · 자동 피격") : TEXT(""),
		bSlowMotion ? TEXT(" · 슬로 모션") : TEXT(""),
		bHoldFlash ? TEXT(" · 플래시 고정") : TEXT("")));
}
