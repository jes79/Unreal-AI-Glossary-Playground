#include "D07Common.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "DrawDebugHelpers.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryDemoBase.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NavAreas/NavArea.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"
#include "Navigation/PathFollowingComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

// ============================================================ AD07Enemy

AD07Enemy::AD07Enemy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	// AI 가 스폰되거나 레벨에 놓이면 AIControllerClass 의 컨트롤러가 자동으로 빙의한다 (Auto Possess AI)
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 이동 방향으로 몸을 돌린다 (컨트롤러 회전은 따르지 않음)
	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 540.f, 0.f);
	Move->MaxWalkSpeed = 350.f;
	Move->BrakingDecelerationWalking = 1500.f;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> MannyAnim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Attack(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01"));
	if (MannyMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MannyMesh.Object);
		GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));
	}
	if (MannyAnim.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(MannyAnim.Class);
	}
	AttackAnim = Attack.Object;

	Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
	Marker->SetupAttachment(RootComponent);
	GlossaryKit::InitShapeComponent(Marker, GlossaryKit::EShape::Sphere, FVector(24.f));
	Marker->SetRelativeLocation(FVector(0.f, 0.f, 125.f));
	Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Marker->SetCanEverAffectNavigation(false);
	Marker->SetCastShadow(false);
}

void AD07Enemy::SetBodyColor(const FLinearColor& Color)
{
	USkeletalMeshComponent* SkelMesh = GetMesh();
	for (int32 Index = 0; Index < SkelMesh->GetNumMaterials(); ++Index)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(GlossaryKit::GetColorMaterial(), SkelMesh);
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetScalarParameterValue(TEXT("Roughness"), 0.55f);
		SkelMesh->SetMaterial(Index, MID);
	}
}

void AD07Enemy::SetMarkerColor(const FLinearColor& Color, float Emissive)
{
	GlossaryKit::SetColor(Marker, Color, 0.4f, 0.f, Emissive);
}

void AD07Enemy::SetLabel(const FString& Text, const FLinearColor& Color)
{
	if (!Label)
	{
		Label = GlossaryKit::SpawnLabel(GetWorld(), Text, GetActorLocation() + FVector(0.f, 0.f, 175.f), Color, 16.f);
		if (Label)
		{
			Label->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		}
		return;
	}
	Label->SetText(Text);
	Label->SetColor(Color);
}

void AD07Enemy::PlayAttack()
{
	++AttackCount;
	if (UAnimInstance* Anim = GetMesh()->GetAnimInstance())
	{
		if (AttackAnim)
		{
			// ABP_Unarmed 에 DefaultSlot 이 있어서 몽타주가 이동 블렌드 위에 덮어씌워진다
			Anim->PlaySlotAnimationAsDynamicMontage(AttackAnim, TEXT("DefaultSlot"), 0.1f, 0.15f, 1.2f);
		}
	}
}

void AD07Enemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Label)
	{
		Label->Destroy();
		Label = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

// ============================================================ UD07SidePanel

void UD07SidePanel::BuildTree()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	UBorder* Border = MakeBorder(FLinearColor(0.02f, 0.03f, 0.05f, 0.82f), FMargin(14.f));
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	TitleText = MakeText(PendingTitle, 17.f, FLinearColor(1.f, 0.8f, 0.2f), true);
	BodyText = MakeText(PendingBody, 14.f);
	Box->AddChildToVerticalBox(TitleText);
	Box->AddChildToVerticalBox(BodyText)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	Border->SetContent(Box);
	// 오른쪽 위 (정보 패널은 왼쪽 위)
	AddToCanvas(Canvas, Border, FAnchors(1.f, 0.f), FVector2D(-16.f, 16.f), FVector2D::ZeroVector, FVector2D(1.f, 0.f));
}

void UD07SidePanel::SetPanelText(const FString& InTitle, const FString& InBody)
{
	PendingTitle = InTitle;
	PendingBody = InBody;
	if (TitleText)
	{
		TitleText->SetText(FText::FromString(InTitle));
	}
	if (BodyText)
	{
		BodyText->SetText(FText::FromString(InBody));
	}
}

// ============================================================ D07 helpers

namespace D07
{
	AD07Enemy* SpawnEnemy(UWorld* World, const FVector& GroundLocation, float Yaw, TSubclassOf<AAIController> ControllerClass,
		const FLinearColor& BodyColor, TSubclassOf<AD07Enemy> PawnClass)
	{
		if (!World)
		{
			return nullptr;
		}
		UClass* Class = PawnClass ? PawnClass.Get() : AD07Enemy::StaticClass();
		const FTransform Transform(FRotator(0.f, Yaw, 0.f), GroundLocation + FVector(0.f, 0.f, 98.f));
		AD07Enemy* Enemy = World->SpawnActorDeferred<AD07Enemy>(Class, Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Enemy)
		{
			return nullptr;
		}
		if (ControllerClass)
		{
			// 스폰을 마치기 전에 바꿔야 PostInitializeComponents 의 자동 빙의가 이 클래스를 쓴다
			Enemy->AIControllerClass = ControllerClass;
		}
		Enemy->FinishSpawning(Transform);
		Enemy->SetBodyColor(BodyColor);
		Enemy->SetMarkerColor(BodyColor);
		return Enemy;
	}

	ACameraActor* SetupOverviewCamera(AGlossaryDemoBase* Demo, const FVector& Location, const FRotator& Rotation, float FOV)
	{
		UWorld* World = Demo ? Demo->GetWorld() : nullptr;
		if (!World)
		{
			return nullptr;
		}
		ACameraActor* Camera = World->SpawnActor<ACameraActor>(Location, Rotation);
		Camera->GetCameraComponent()->SetFieldOfView(FOV);
		Camera->GetCameraComponent()->bConstrainAspectRatio = false;
		if (APlayerController* PC = Demo->GetPC())
		{
			PC->bAutoManageActiveCameraTarget = false;
			PC->SetViewTarget(Camera);
			// WASD 가 화면 기준으로 움직이도록 컨트롤 회전을 카메라 Yaw 에 맞춘다
			PC->SetControlRotation(FRotator(0.f, Rotation.Yaw, 0.f));
		}
		return Camera;
	}

	void ToggleOverviewCamera(AGlossaryDemoBase* Demo, ACameraActor* Camera)
	{
		APlayerController* PC = Demo ? Demo->GetPC() : nullptr;
		if (!PC || !Camera)
		{
			return;
		}
		if (PC->GetViewTarget() == Camera && PC->GetPawn())
		{
			PC->SetViewTarget(PC->GetPawn());
		}
		else
		{
			PC->SetViewTarget(Camera);
			PC->SetControlRotation(FRotator(0.f, Camera->GetActorRotation().Yaw, 0.f));
		}
	}

	UD07SidePanel* CreateSidePanel(AGlossaryDemoBase* Demo)
	{
		APlayerController* PC = Demo ? Demo->GetPC() : nullptr;
		if (!PC)
		{
			return nullptr;
		}
		UD07SidePanel* Panel = CreateWidget<UD07SidePanel>(PC, UD07SidePanel::StaticClass());
		Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
		Panel->AddToViewport(90);
		return Panel;
	}

	void DrawCurrentPath(AAIController* Controller, const FColor& Color, float Thickness)
	{
		if (!Controller || !Controller->GetPathFollowingComponent())
		{
			return;
		}
		const FNavPathSharedPtr Path = Controller->GetPathFollowingComponent()->GetPath();
		if (!Path.IsValid() || !Path->IsValid())
		{
			return;
		}
		const TArray<FNavPathPoint>& Points = Path->GetPathPoints();
		UWorld* World = Controller->GetWorld();
		for (int32 Index = 0; Index + 1 < Points.Num(); ++Index)
		{
			DrawDebugLine(World, Points[Index].Location + FVector(0.f, 0.f, 15.f), Points[Index + 1].Location + FVector(0.f, 0.f, 15.f), Color, false, -1.f, 0, Thickness);
		}
		for (const FNavPathPoint& Point : Points)
		{
			DrawDebugSphere(World, Point.Location + FVector(0.f, 0.f, 15.f), 14.f, 8, Color, false, -1.f, 0, 2.f);
		}
	}

	// NavMesh 폴리곤 캐시 (월드 하나만 그리므로 정적 캐시면 충분)
	struct FNavDrawCache
	{
		TWeakObjectPtr<UWorld> World;
		double LastRefresh = -1.0;
		TMap<FColor, TArray<FVector>> FillVerts;
		TMap<FColor, TArray<int32>> FillIndices;
		TArray<TPair<FVector, FVector>> Edges;
	};
	static FNavDrawCache GNavDrawCache;

	static void RefreshNavCache(UWorld* World, float ZOffset)
	{
		FNavDrawCache& Cache = GNavDrawCache;
		Cache.World = World;
		Cache.LastRefresh = World->GetTimeSeconds();
		Cache.FillVerts.Reset();
		Cache.FillIndices.Reset();
		Cache.Edges.Reset();

		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		const ARecastNavMesh* NavMesh = NavSys ? Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate)) : nullptr;
		if (!NavMesh)
		{
			return;
		}
		TArray<FNavPoly> Polys;
		TArray<FVector> PolyVerts;
		const int32 TileCount = NavMesh->GetNavMeshTilesCount();
		for (int32 Tile = 0; Tile < TileCount; ++Tile)
		{
			Polys.Reset();
			if (!NavMesh->GetPolysInTile(Tile, Polys))
			{
				continue;
			}
			for (const FNavPoly& Poly : Polys)
			{
				PolyVerts.Reset();
				if (!NavMesh->GetPolyVerts(Poly.Ref, PolyVerts) || PolyVerts.Num() < 3)
				{
					continue;
				}
				// 폴리곤의 Nav Area 색 (UNavArea::DrawColor) — 비용 10 늪은 갈색, 기본은 초록
				FColor Color(40, 200, 90);
				if (const UClass* AreaClass = NavMesh->GetAreaClass(NavMesh->GetPolyAreaID(Poly.Ref)))
				{
					Color = GetDefault<UNavArea>(AreaClass)->DrawColor;
				}
				Color.A = 90;
				TArray<FVector>& Verts = Cache.FillVerts.FindOrAdd(Color);
				TArray<int32>& Indices = Cache.FillIndices.FindOrAdd(Color);
				const int32 Base = Verts.Num();
				for (const FVector& V : PolyVerts)
				{
					Verts.Add(V + FVector(0.f, 0.f, ZOffset));
				}
				for (int32 I = 1; I + 1 < PolyVerts.Num(); ++I)
				{
					Indices.Add(Base);
					Indices.Add(Base + I);
					Indices.Add(Base + I + 1);
				}
				for (int32 I = 0; I < PolyVerts.Num(); ++I)
				{
					Cache.Edges.Emplace(PolyVerts[I] + FVector(0.f, 0.f, ZOffset + 1.f), PolyVerts[(I + 1) % PolyVerts.Num()] + FVector(0.f, 0.f, ZOffset + 1.f));
				}
			}
		}
	}

	void DrawNavMesh(UWorld* World, float ZOffset, float RefreshInterval)
	{
		if (!World)
		{
			return;
		}
		FNavDrawCache& Cache = GNavDrawCache;
		if (Cache.World.Get() != World || World->GetTimeSeconds() - Cache.LastRefresh >= RefreshInterval || World->GetTimeSeconds() < Cache.LastRefresh)
		{
			RefreshNavCache(World, ZOffset);
		}
		for (const TPair<FColor, TArray<FVector>>& Pair : Cache.FillVerts)
		{
			DrawDebugMesh(World, Pair.Value, Cache.FillIndices[Pair.Key], Pair.Key, false, -1.f, 0);
		}
		for (const TPair<FVector, FVector>& Edge : Cache.Edges)
		{
			DrawDebugLine(World, Edge.Key, Edge.Value, FColor(20, 60, 30), false, -1.f, 0, 1.5f);
		}
	}

	static void DescribeNode(const UBTCompositeNode* Node, const UBTNode* Active, int32 Depth, const FString& Prefix, FString& Out)
	{
		const FString Indent = FString::ChrN(Depth * 4, TEXT(' '));
		Out += FString::Printf(TEXT("%s%s%s\n"), *Indent, *Prefix, *Node->GetNodeName());
		for (const UBTService* Service : Node->Services)
		{
			Out += FString::Printf(TEXT("%s    (서비스) %s\n"), *Indent, *Service->GetNodeName());
		}
		for (const FBTCompositeChild& Child : Node->Children)
		{
			FString ChildPrefix;
			for (const UBTDecorator* Decorator : Child.Decorators)
			{
				ChildPrefix += FString::Printf(TEXT("[%s] "), *Decorator->GetNodeName());
			}
			if (Child.ChildComposite)
			{
				DescribeNode(Child.ChildComposite, Active, Depth + 1, ChildPrefix, Out);
			}
			else if (Child.ChildTask)
			{
				const bool bActive = Child.ChildTask == Active;
				Out += FString::Printf(TEXT("%s    %s%s%s\n"), *Indent, bActive ? TEXT(">> ") : TEXT("    "), *ChildPrefix, *Child.ChildTask->GetNodeName());
			}
		}
	}

	FString DescribeBehaviorTree(const UBTCompositeNode* Root, const UBTNode* ActiveNode)
	{
		FString Out;
		if (Root)
		{
			DescribeNode(Root, ActiveNode, 0, FString(), Out);
		}
		return Out;
	}

	void* FindPropertyPtr(UObject* Object, FName PropertyName)
	{
		if (!Object)
		{
			return nullptr;
		}
		FProperty* Property = FindFProperty<FProperty>(Object->GetClass(), PropertyName);
		if (!Property)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Glossary] 07: 프로퍼티 없음 %s.%s"), *Object->GetClass()->GetName(), *PropertyName.ToString());
			return nullptr;
		}
		return Property->ContainerPtrToValuePtr<void>(Object);
	}

	bool SetBoolProperty(UObject* Object, FName PropertyName, bool bValue)
	{
		FBoolProperty* Property = Object ? FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName) : nullptr;
		if (!Property)
		{
			return false;
		}
		Property->SetPropertyValue_InContainer(Object, bValue);
		return true;
	}

	bool SetBlackboardKey(UObject* Object, FName PropertyName, FName KeyName)
	{
		FStructProperty* Property = Object ? FindFProperty<FStructProperty>(Object->GetClass(), PropertyName) : nullptr;
		if (!Property || Property->Struct != FBlackboardKeySelector::StaticStruct())
		{
			return false;
		}
		FBlackboardKeySelector* Selector = Property->ContainerPtrToValuePtr<FBlackboardKeySelector>(Object);
		Selector->SelectedKeyName = KeyName;
		return true;
	}
}
