#include "Demo0519_HitFlash.h"

#include "Components/StaticMeshComponent.h"
#include "EffectsDemoKit.h"
#include "GlossaryKit.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"

using namespace GlossaryKit;

ADemo0519_HitFlash::ADemo0519_HitFlash()
{
	DemoId = TEXT("0519");
	Controls = TEXT("Space : 때리기 (1.2초마다 자동)   C : Hit Flash ON/OFF\n")
		TEXT("관찰: 맞는 순간 몸 전체의 Dynamic Material Instance 파라미터 FlashAmount 가 1 이 되고\n")
		TEXT("0.15초 동안 0 으로 내려가 하얗게 번쩍였다가 원래 색으로 돌아온다");
	Showcase = TEXT("피격 시 DMI 의 FlashAmount 1→0 (0.15초)로 하얗게 번쩍이는 허수아비, ON/OFF 비교");
	PlayerPawnClass = nullptr;
}

void ADemo0519_HitFlash::BeginPlay()
{
	Super::BeginPlay();
	// 허수아비: 몸통·머리·팔을 같은 Hit Flash 머티리얼로 (각 부위마다 DMI 하나)
	Dummy = NewObject<USceneComponent>(this, TEXT("Dummy"));
	Dummy->SetupAttachment(GetRootComponent());
	Dummy->RegisterComponent();
	UMaterialInterface* FlashMat = LoadAsset<UMaterialInterface>(EffectsKit::AssetPath(TEXT("M_HitFlash")));
	struct FPart { EShape Shape; FVector Loc; FVector Size; FLinearColor Color; };
	const FPart Parts[] = {
		{ EShape::Cylinder, FVector(0.f, 0.f, 60.f), FVector(20.f, 20.f, 120.f), FLinearColor(0.3f, 0.18f, 0.08f) },
		{ EShape::Cube, FVector(0.f, 0.f, 170.f), FVector(70.f, 90.f, 110.f), FLinearColor(0.75f, 0.2f, 0.15f) },
		{ EShape::Sphere, FVector(0.f, 0.f, 265.f), FVector(70.f), FLinearColor(0.9f, 0.75f, 0.5f) },
		{ EShape::Cube, FVector(0.f, 0.f, 200.f), FVector(20.f, 220.f, 20.f), FLinearColor(0.3f, 0.18f, 0.08f) },
	};
	for (const FPart& Part : Parts)
	{
		UStaticMeshComponent* Mesh = AddShape(this, Part.Shape, Part.Loc, Part.Size, Part.Color, Dummy);
		if (FlashMat)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(FlashMat, this);
			MID->SetVectorParameterValue(TEXT("BaseColor"), Part.Color);
			MID->SetScalarParameterValue(TEXT("FlashAmount"), 0.f);
			Mesh->SetMaterial(0, MID);
			FlashMIDs.Add(MID);
		}
	}
	SpawnLabel(GetWorld(), TEXT("FlashAmount 1 → 0 (0.15초)"), FVector(0.f, 0.f, 360.f), Colors::Item, 20.f);
	EffectsKit::SetupFixedCamera(this, FVector(-600.f, -150.f, 220.f), FVector(0.f, 0.f, 170.f), 60.f);
}

void ADemo0519_HitFlash::Hit()
{
	++HitCount;
	FlashTime = 0.f;
	// 맞은 쪽으로 살짝 밀리는 반동 + 작은 불꽃 (Niagara 는 0513 의 NS_HitSpark 재사용)
	if (UNiagaraSystem* Spark = EffectsKit::LoadSystem(TEXT("NS_HitSpark")))
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Spark, GetActorLocation() + FVector(-40.f, 0.f, 180.f), FRotator(0.f, 180.f, 0.f));
	}
}

void ADemo0519_HitFlash::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Timer += DeltaSeconds;
	if (WasKeyPressed(EKeys::C)) { bFlashEnabled = !bFlashEnabled; }
	if (WasKeyPressed(EKeys::SpaceBar) || Timer >= AutoHitInterval)
	{
		Timer = 0.f;
		Hit();
	}
	FlashTime += DeltaSeconds;
	// 핵심: FlashAmount = 1 - 경과/0.15 (0 아래로는 0)
	const float FlashAmount = bFlashEnabled ? FMath::Clamp(1.f - FlashTime / FlashDuration, 0.f, 1.f) : 0.f;
	for (UMaterialInstanceDynamic* MID : FlashMIDs)
	{
		MID->SetScalarParameterValue(TEXT("FlashAmount"), FlashAmount);
	}
	// 반동: 맞은 직후 뒤로 기울었다가 돌아온다
	const float Recoil = FMath::Clamp(1.f - FlashTime / 0.3f, 0.f, 1.f);
	if (Dummy)
	{
		Dummy->SetRelativeRotation(FRotator(-8.f * Recoil * Recoil, 0.f, 0.f));
	}
	SetStatus(FString::Printf(TEXT("Hit Flash %s · 피격 %d · FlashAmount %.2f"), bFlashEnabled ? TEXT("ON") : TEXT("OFF"), HitCount, FlashAmount));
}
