#include "Demo0511_SubUV.h"

#include "Components/StaticMeshComponent.h"
#include "EffectsDemoKit.h"
#include "GlossaryKit.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"

using namespace GlossaryKit;

ADemo0511_SubUV::ADemo0511_SubUV()
{
	DemoId = TEXT("0511");
	Controls = TEXT("Space : 다시 재생\n")
		TEXT("관찰: 오른쪽 판이 코드로 만든 8x8(64프레임) 폭발 시트. 왼쪽 입자는 Sub Image Size 8x8 로 시트를 나누고\n")
		TEXT("SubUV Animation 모듈이 수명에 맞춰 0→63 프레임을 넘겨 불덩이가 커지며 연기로 식는다");
	Showcase = TEXT("코드로 만든 8x8 폭발 플립북을 SubUV Animation 모듈로 재생, 원본 시트 나란히 표시");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Night;
	Environment.ExposureEV100 = 12.f;   // 환경 노출 보정 = EV100-10 이라 값이 클수록 밝다 (밤 장면을 조금 밝게)
}

void ADemo0511_SubUV::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	Flipbook = EffectsKit::AddNiagara(this, TEXT("NS_Flipbook"), FVector(0.f, -250.f, 220.f));

	// 원본 플립북 시트를 보여 주는 판 (Plane 을 세워 카메라 쪽으로)
	UStaticMeshComponent* Sheet = AddShape(this, EShape::Plane, FVector(0.f, 320.f, 230.f), FVector(360.f, 360.f, 1.f), FLinearColor::White,
		nullptr, FRotator(90.f, 0.f, 0.f));   // 법선(+Z)을 카메라(-X) 쪽으로
	if (UMaterialInterface* Preview = LoadAsset<UMaterialInterface>(EffectsKit::AssetPath(TEXT("M_FX_SheetPreview"))))
	{
		Sheet->SetMaterial(0, Preview);
	}
	SpawnLabel(World, TEXT("SubUV 재생 (Sub Image Size 8x8)"), FVector(0.f, -150.f, 470.f), Colors::Item, 20.f);
	SpawnLabel(World, TEXT("T_FX_Explosion8x8 원본 시트"), FVector(0.f, 320.f, 450.f), FLinearColor::White, 20.f);
	EffectsKit::SetupFixedCamera(this, FVector(-1100.f, -250.f, 260.f), FVector(0.f, -60.f, 240.f), 65.f);
}

void ADemo0511_SubUV::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Timer += DeltaSeconds;
	if (WasKeyPressed(EKeys::SpaceBar) || Timer >= Interval)
	{
		Timer = 0.f;
		if (Flipbook) Flipbook->ResetSystem();
	}
	const int32 Frame = FMath::Clamp(FMath::FloorToInt(Timer / 1.2f * 64.f), 0, 63);
	SetStatus(FString::Printf(TEXT("SubUV Animation: 수명 1.2초 동안 64프레임 · 지금 약 %d 프레임 (행 %d, 열 %d)"), Frame, Frame / 8, Frame % 8));
}
