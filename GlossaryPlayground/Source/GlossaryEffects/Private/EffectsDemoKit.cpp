#include "EffectsDemoKit.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryDemoBase.h"
#include "GlossaryKit.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogGlossaryEffects, Log, All);

namespace EffectsKit
{
	FString AssetPath(const FString& Name)
	{
		return TEXT("/Game/Glossary/05_Effects/") + Name;
	}

	UNiagaraSystem* LoadSystem(const FString& Name)
	{
		UNiagaraSystem* System = GlossaryKit::LoadAsset<UNiagaraSystem>(AssetPath(Name));
		if (!System)
		{
			UE_LOG(LogGlossaryEffects, Error, TEXT("[Glossary] Niagara System 없음: %s (tools/glossary-build.sh -filter=05 로 생성)"), *Name);
		}
		return System;
	}

	UNiagaraComponent* AddNiagara(AActor* Owner, const FString& SystemName, const FVector& RelativeLocation, USceneComponent* Parent,
		bool bAutoActivate, const FRotator& RelativeRotation)
	{
		UNiagaraComponent* Component = NewObject<UNiagaraComponent>(Owner);
		Component->SetAsset(LoadSystem(SystemName));
		Component->SetAutoActivate(bAutoActivate);
		Component->SetupAttachment(Parent ? Parent : Owner->GetRootComponent());
		Component->SetRelativeLocationAndRotation(RelativeLocation, RelativeRotation);
		Component->RegisterComponent();
		if (bAutoActivate)
		{
			Component->Activate(true);
		}
		return Component;
	}

	ACameraActor* SetupFixedCamera(AGlossaryDemoBase* Demo, const FVector& Location, const FVector& LookAt, float FOV)
	{
		UWorld* World = Demo->GetWorld();
		ACameraActor* Camera = World->SpawnActor<ACameraActor>(Location, (LookAt - Location).Rotation());
		Camera->GetCameraComponent()->SetFieldOfView(FOV);
		Camera->GetCameraComponent()->bConstrainAspectRatio = false;
		if (APlayerController* PC = Demo->GetPC())
		{
			PC->SetViewTarget(Camera);
		}
		return Camera;
	}

	UPointLightComponent* AddPointLight(AActor* Owner, const FVector& RelativeLocation, const FLinearColor& Color, float Intensity, float Radius, USceneComponent* Parent)
	{
		UPointLightComponent* Light = NewObject<UPointLightComponent>(Owner);
		Light->SetupAttachment(Parent ? Parent : Owner->GetRootComponent());
		Light->SetRelativeLocation(RelativeLocation);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetLightColor(Color);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(Intensity);
		Light->SetAttenuationRadius(Radius);
		Light->SetCastShadows(false);
		Light->RegisterComponent();
		return Light;
	}
}

// ---------------------------------------------------------------- 카메라 쉐이크

void UGlossaryDecayShakePattern::GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const
{
	OutInfo.Duration = FCameraShakeDuration(Duration);
}

void UGlossaryDecayShakePattern::StartShakePatternImpl(const FCameraShakePatternStartParams& Params)
{
	Elapsed = 0.f;
}

void UGlossaryDecayShakePattern::UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult)
{
	Elapsed += Params.DeltaTime;
	const float Strength = FMath::Clamp(1.f - Elapsed / Duration, 0.f, 1.f);   // 선형 감쇠
	const float Phase = Elapsed * Frequency * 2.f * PI;
	OutResult.Location = FVector(0.f, FMath::Sin(Phase), FMath::Sin(Phase * 1.37f + 1.f)) * LocationAmplitude * Strength;
	OutResult.Rotation = FRotator(FMath::Sin(Phase * 0.9f + 2.f), FMath::Sin(Phase * 1.1f), 0.f) * RotationAmplitude * Strength;
}

bool UGlossaryDecayShakePattern::IsFinishedImpl() const
{
	return Elapsed >= Duration;
}

UGlossaryExplosionShake::UGlossaryExplosionShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetRootShakePattern(CreateDefaultSubobject<UGlossaryDecayShakePattern>(TEXT("DecayPattern")));
}

// ---------------------------------------------------------------- 포스트 프로세스

#include "Engine/PostProcessVolume.h"
#if WITH_EDITOR
#include "BSPOps.h"
#include "Builders/CubeBuilder.h"
#include "Components/BrushComponent.h"
#include "Engine/Polys.h"
#include "GameFramework/Volume.h"
#include "Model.h"
#endif

namespace EffectsKit
{
	APostProcessVolume* SpawnUnboundPostProcess(UWorld* World, float Priority)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		APostProcessVolume* Volume = World->SpawnActor<APostProcessVolume>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
		Volume->bUnbound = true;      // Infinite Extent: 어디에 있든 화면 전체에 적용
		Volume->Priority = Priority;  // 환경 액터의 노출 고정용 후처리보다 높게
		Volume->BlendWeight = 1.f;
		return Volume;
	}

#if WITH_EDITOR
	AVolume* SpawnBoxVolume(UWorld* World, UClass* VolumeClass, const FVector& Center, const FVector& Size)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AVolume* Volume = World->SpawnActor<AVolume>(VolumeClass, Center, FRotator::ZeroRotator, Params);
		if (!Volume)
		{
			return nullptr;
		}
		// UActorFactoryBoxVolume 과 같은 순서로 상자 브러시를 만든다
		Volume->PolyFlags = 0;
		Volume->Brush = NewObject<UModel>(Volume, NAME_None, RF_Transactional);
		Volume->Brush->Initialize(nullptr, true);
		Volume->Brush->Polys = NewObject<UPolys>(Volume->Brush, NAME_None, RF_Transactional);
		Volume->GetBrushComponent()->Brush = Volume->Brush;
		UCubeBuilder* Builder = NewObject<UCubeBuilder>(Volume);
		Builder->X = Size.X;
		Builder->Y = Size.Y;
		Builder->Z = Size.Z;
		Volume->BrushBuilder = Builder;
		Builder->Build(World, Volume);
		FBSPOps::csgPrepMovingBrush(Volume);
		Volume->GetBrushComponent()->RequestUpdateBrushCollision();
		Volume->PostEditChange();
		return Volume;
	}
#endif
}
