#include "RenderingDemoKit.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/PostProcessComponent.h"
#include "DynamicRHI.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryDemoBase.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Kismet/KismetMathLibrary.h"
#include "RHIStats.h"
#include "RenderTimer.h"

#if WITH_EDITOR
#include "BSPOps.h"
#include "Builders/CubeBuilder.h"
#include "Components/BrushComponent.h"
#include "Engine/Polys.h"
#include "GameFramework/Volume.h"
#include "Model.h"
#endif

namespace RenderKit
{
	void FFrameStats::Update(float DeltaSeconds)
	{
		// 숫자가 너무 튀지 않게 지수 평활 (stat unit 도 평균값을 보여 준다)
		auto Smooth = [](float& Value, float Target) { Value = Value <= 0.f ? Target : FMath::Lerp(Value, Target, 0.1f); };
		Smooth(FrameMs, DeltaSeconds * 1000.f);
		FPS = FrameMs > 0.f ? 1000.f / FrameMs : 0.f;
		Smooth(GameMs, FPlatformTime::ToMilliseconds(GGameThreadTime));
		Smooth(DrawMs, FPlatformTime::ToMilliseconds(GRenderThreadTime));
		Smooth(GPUMs, FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles(0)));
		DrawCalls = GNumDrawCallsRHI[0];
		Primitives = GNumPrimitivesDrawnRHI[0];
	}

	FString FFrameStats::ToString(bool bWithDrawCalls) const
	{
		FString S = FString::Printf(TEXT("FPS %.0f · Frame %.1fms · Game %.1f · Draw %.1f · GPU %.1f"), FPS, FrameMs, GameMs, DrawMs, GPUMs);
		if (bWithDrawCalls)
		{
			S += FString::Printf(TEXT(" · DrawCalls %d · Tris %.1fK"), DrawCalls, Primitives / 1000.f);
		}
		return S;
	}

	FString FFrameStats::Bottleneck() const
	{
		if (GPUMs >= GameMs && GPUMs >= DrawMs)
		{
			return TEXT("GPU");
		}
		return GameMs >= DrawMs ? TEXT("Game") : TEXT("Draw");
	}

	void FCVarScope::Set(const TCHAR* Name, const FString& Value)
	{
		IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name);
		if (!CVar)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Glossary] 콘솔 변수 없음: %s"), Name);
			return;
		}
		if (!Originals.Contains(Name))
		{
			Originals.Add(Name, CVar->GetString());
		}
		// 콘솔에 직접 입력한 것과 같은 우선순위 (프로젝트 설정보다 높다)
		CVar->Set(*Value, ECVF_SetByConsole);
	}

	void FCVarScope::RestoreAll()
	{
		for (const TPair<FString, FString>& Pair : Originals)
		{
			if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*Pair.Key))
			{
				CVar->Set(*Pair.Value, ECVF_SetByConsole);
			}
		}
		Originals.Reset();
	}

	FString GetCVar(const TCHAR* Name)
	{
		IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name);
		return CVar ? CVar->GetString() : FString();
	}

	int32 GetCVarInt(const TCHAR* Name)
	{
		IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name);
		return CVar ? CVar->GetInt() : 0;
	}

	ACameraActor* SpawnViewCamera(AGlossaryDemoBase* Demo, const FVector& Location, const FRotator& Rotation, float FOV)
	{
		UWorld* World = Demo->GetWorld();
		ACameraActor* Camera = World->SpawnActor<ACameraActor>(Location, Rotation);
		Camera->GetCameraComponent()->SetFieldOfView(FOV);
		Camera->GetCameraComponent()->bConstrainAspectRatio = false;
		if (APlayerController* PC = Demo->GetPC())
		{
			PC->bAutoManageActiveCameraTarget = false;
			PC->SetViewTarget(Camera);
		}
		return Camera;
	}

	ACameraActor* SpawnLookAtCamera(AGlossaryDemoBase* Demo, const FVector& Location, const FVector& Target, float FOV)
	{
		return SpawnViewCamera(Demo, Location, UKismetMathLibrary::FindLookAtRotation(Location, Target), FOV);
	}

	void TickFlyCamera(AGlossaryDemoBase* Demo, ACameraActor* Camera, float DeltaSeconds, float Speed)
	{
		if (!Camera)
		{
			return;
		}
		const float Boost = Demo->IsKeyDown(EKeys::LeftShift) ? 4.f : 1.f;
		FVector Move = FVector::ZeroVector;
		if (Demo->IsKeyDown(EKeys::W)) Move.X += 1.f;
		if (Demo->IsKeyDown(EKeys::S)) Move.X -= 1.f;
		if (Demo->IsKeyDown(EKeys::D)) Move.Y += 1.f;
		if (Demo->IsKeyDown(EKeys::A)) Move.Y -= 1.f;
		if (Demo->IsKeyDown(EKeys::E)) Move.Z += 1.f;
		if (Demo->IsKeyDown(EKeys::Q)) Move.Z -= 1.f;

		FRotator Rot = Camera->GetActorRotation();
		if (Demo->IsKeyDown(EKeys::Left))  Rot.Yaw -= 60.f * DeltaSeconds;
		if (Demo->IsKeyDown(EKeys::Right)) Rot.Yaw += 60.f * DeltaSeconds;
		if (Demo->IsKeyDown(EKeys::Up))    Rot.Pitch = FMath::Clamp(Rot.Pitch + 40.f * DeltaSeconds, -89.f, 89.f);
		if (Demo->IsKeyDown(EKeys::Down))  Rot.Pitch = FMath::Clamp(Rot.Pitch - 40.f * DeltaSeconds, -89.f, 89.f);
		Camera->SetActorRotation(Rot);

		if (!Move.IsNearlyZero())
		{
			const FRotator Yaw(0.f, Rot.Yaw, 0.f);
			const FVector World = Yaw.RotateVector(FVector(Move.X, Move.Y, 0.f)) + FVector(0.f, 0.f, Move.Z);
			Camera->AddActorWorldOffset(World * Speed * Boost * DeltaSeconds);
		}
	}

	UPostProcessComponent* AddPostProcess(AActor* Owner, float Priority)
	{
		UPostProcessComponent* PP = NewObject<UPostProcessComponent>(Owner, TEXT("DemoPostProcess"));
		PP->bUnbound = true;
		PP->Priority = Priority;
		PP->SetupAttachment(Owner->GetRootComponent());
		PP->RegisterComponent();
		Owner->AddInstanceComponent(PP);
		return PP;
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
		// GlossaryLevelBuilder 의 NavMeshBounds 와 같은 순서(UActorFactoryBoxVolume)로 상자 브러시를 만든다
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
