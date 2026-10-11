#include "PhysicsDemoUtil.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryDemoBase.h"
#include "GlossaryKit.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

namespace PhysicsDemoUtil
{
	ACameraActor* SpawnFixedCamera(UWorld* World, const FVector& Location, const FRotator& Rotation, float FOV)
	{
		if (!World)
		{
			return nullptr;
		}
		ACameraActor* Camera = World->SpawnActor<ACameraActor>(Location, Rotation);
		if (Camera)
		{
			Camera->GetCameraComponent()->SetFieldOfView(FOV);
			Camera->GetCameraComponent()->bConstrainAspectRatio = false;
		}
		return Camera;
	}

	void KeepViewTarget(const AGlossaryDemoBase* Demo, ACameraActor* Camera)
	{
		APlayerController* PC = Demo ? Demo->GetPC() : nullptr;
		if (PC && Camera && PC->GetViewTarget() != Camera)
		{
			PC->SetViewTarget(Camera);
		}
	}

	bool GetViewRay(const AGlossaryDemoBase* Demo, FVector& OutStart, FVector& OutDir)
	{
		APlayerController* PC = Demo ? Demo->GetPC() : nullptr;
		if (!PC)
		{
			return false;
		}
		int32 SizeX = 0, SizeY = 0;
		PC->GetViewportSize(SizeX, SizeY);
		return PC->DeprojectScreenPositionToWorld(SizeX * 0.5f, SizeY * 0.5f, OutStart, OutDir);
	}

	bool TraceScreenCenter(const AGlossaryDemoBase* Demo, FHitResult& OutHit, float Distance, ECollisionChannel Channel, const AActor* Ignore)
	{
		FVector Start, Dir;
		if (!GetViewRay(Demo, Start, Dir))
		{
			return false;
		}
		FCollisionQueryParams Params(SCENE_QUERY_STAT(PhysicsDemoCenterTrace), false);
		if (Ignore)
		{
			Params.AddIgnoredActor(Ignore);
		}
		if (APawn* Pawn = Demo->GetPlayerPawn())
		{
			Params.AddIgnoredActor(Pawn);
		}
		return Demo->GetWorld()->LineTraceSingleByChannel(OutHit, Start, Start + Dir * Distance, Channel, Params);
	}

	bool TraceCursor(const AGlossaryDemoBase* Demo, FHitResult& OutHit, ECollisionChannel Channel)
	{
		APlayerController* PC = Demo ? Demo->GetPC() : nullptr;
		if (!PC)
		{
			return false;
		}
		if (PC->bShowMouseCursor && PC->GetHitResultUnderCursor(Channel, false, OutHit))
		{
			return true;
		}
		return TraceScreenCenter(Demo, OutHit, 100000.f, Channel);
	}

	UPhysicalMaterial* LoadOrMakePhysMat(UObject* Outer, const FString& Path, float Friction, float Restitution, bool bFrictionMin, bool bRestitutionMax)
	{
		if (UPhysicalMaterial* Loaded = GlossaryKit::LoadAsset<UPhysicalMaterial>(Path))
		{
			return Loaded;
		}
		// 빌더를 아직 돌리지 않았을 때도 데모가 동작하도록 같은 값으로 런타임 생성
		UPhysicalMaterial* PM = NewObject<UPhysicalMaterial>(Outer);
		PM->Friction = Friction;
		PM->StaticFriction = Friction;
		PM->Restitution = Restitution;
		if (bFrictionMin)
		{
			PM->bOverrideFrictionCombineMode = true;
			PM->FrictionCombineMode = EFrictionCombineMode::Min;
		}
		if (bRestitutionMax)
		{
			PM->bOverrideRestitutionCombineMode = true;
			PM->RestitutionCombineMode = EFrictionCombineMode::Max;
		}
		return PM;
	}
}
