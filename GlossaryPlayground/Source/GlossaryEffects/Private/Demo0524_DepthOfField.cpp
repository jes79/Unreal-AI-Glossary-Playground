#include "Demo0524_DepthOfField.h"

#include "Animation/AnimSequence.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "EffectsDemoKit.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkeletalMesh.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0524_DepthOfField::ADemo0524_DepthOfField()
{
	DemoId = TEXT("0524");
	Controls = TEXT("C : Depth of Field ON/OFF   F : 초점을 대화 상대 ↔ 먼 배경\n")
		TEXT("관찰: Cinematic DOF 의 Focal Distance 를 카메라→대화 상대 거리로 맞추면 상대만 또렷하고\n")
		TEXT("뒤쪽 기둥들은 거리가 멀수록 더 흐려진다. Aperture(F-stop) 1.4 = 얕은 심도");
	Showcase = TEXT("Cinematic DOF 로 대화 상대에게 Focal Distance 를 맞추고 배경 흐림, 초점 전환·ON/OFF");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Sunset;
}

void ADemo0524_DepthOfField::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 대화 상대: 카메라 쪽을 보고 서 있는 마네킹 (Idle 재생)
	ASkeletalMeshActor* Mannequin = World->SpawnActor<ASkeletalMeshActor>(FVector(0.f, 0.f, 0.f), FRotator(0.f, 90.f, 0.f));
	USkeletalMeshComponent* Mesh = Mannequin->GetSkeletalMeshComponent();
	Mesh->SetSkeletalMesh(LoadAsset<USkeletalMesh>(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")));
	if (UAnimSequence* Idle = LoadAsset<UAnimSequence>(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle")))
	{
		Mesh->PlayAnimation(Idle, true);
	}
	Partner = Mannequin;

	// 배경: 멀어질수록 흐려지는 것을 보기 위해 거리별 기둥
	for (int32 i = 0; i < 10; ++i)
	{
		const float X = 600.f + i * 400.f;
		const FLinearColor Color = i % 3 == 0 ? Colors::Item : i % 3 == 1 ? Colors::Cyan : Colors::Enemy;
		SpawnBlock(World, FVector(X, (i % 2 == 0 ? -1.f : 1.f) * (250.f + i * 40.f), 0.f), FVector(100.f, 100.f, 300.f + i * 30.f), Color);
		SpawnLabel(World, FString::Printf(TEXT("%.0fm"), (X + 350.f) / 100.f), FVector(X, (i % 2 == 0 ? -1.f : 1.f) * (250.f + i * 40.f), 400.f + i * 30.f), FLinearColor::White, 16.f);
	}

	CameraLocation = FVector(-350.f, 60.f, 160.f);
	EffectsKit::SetupFixedCamera(this, CameraLocation, FVector(0.f, 0.f, 150.f), 50.f);
	Volume = EffectsKit::SpawnUnboundPostProcess(World);
}

void ADemo0524_DepthOfField::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::C)) bDOF = !bDOF;
	if (WasKeyPressed(EKeys::F)) bFocusFar = !bFocusFar;

	// 핵심: Focal Distance = 카메라에서 대화 상대(머리 높이)까지 거리
	const FVector FocusPoint = Partner ? Partner->GetActorLocation() + FVector(0.f, 0.f, 150.f) : FVector::ZeroVector;
	const float PartnerDistance = FVector::Dist(CameraLocation, FocusPoint);
	const float FocalDistance = bFocusFar ? 3500.f : PartnerDistance;
	if (Volume)
	{
		FPostProcessSettings& S = Volume->Settings;
		S.bOverride_DepthOfFieldFocalDistance = true;
		S.bOverride_DepthOfFieldFstop = true;
		S.bOverride_DepthOfFieldMinFstop = true;
		S.DepthOfFieldMinFstop = 1.2f;
		S.DepthOfFieldFocalDistance = bDOF ? FocalDistance : 0.f;   // 0 = DOF 끔
		S.DepthOfFieldFstop = Aperture;
	}
	SetStatus(FString::Printf(TEXT("Depth of Field %s · Focal Distance %.0f cm (%s) · Aperture f/%.1f"),
		bDOF ? TEXT("ON") : TEXT("OFF"), FocalDistance, bFocusFar ? TEXT("먼 배경") : TEXT("대화 상대"), Aperture));
}
