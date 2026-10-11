#include "Demo0516_FootstepDust.h"

#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EffectsDemoKit.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GlossaryKit.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- Anim Notify

void UAnimNotify_FootstepDust::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	UWorld* World = MeshComp ? MeshComp->GetWorld() : nullptr;
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	// 발 위치에서 아래로 트레이스 → 맞은 바닥의 Physical Material
	const FVector Foot = MeshComp->GetSocketLocation(FootBone);
	FHitResult Hit;
	FCollisionQueryParams Params(TEXT("FootstepDust"), false, MeshComp->GetOwner());
	Params.bReturnPhysicalMaterial = true;
	FLinearColor Color = DefaultColor;
	FString Surface = TEXT("기본");
	FVector SpawnAt = Foot;
	if (World->LineTraceSingleByChannel(Hit, Foot + FVector(0.f, 0.f, 30.f), Foot - FVector(0.f, 0.f, 60.f), ECC_Visibility, Params))
	{
		SpawnAt = Hit.ImpactPoint;
		if (UPhysicalMaterial* PhysMat = Hit.PhysMaterial.Get())
		{
			if (const FLinearColor* Found = SurfaceColors.Find(PhysMat))
			{
				Color = *Found;
			}
			Surface = PhysMat->GetName();
		}
	}
	if (UNiagaraComponent* Dust = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, DustSystem, SpawnAt))
	{
		Dust->SetVariableLinearColor(TEXT("Color"), Color);   // 지면 재질 색 → User.Color
	}
	if (ADemo0516_FootstepDust* Demo = Cast<ADemo0516_FootstepDust>(MeshComp->GetOwner()))
	{
		Demo->ReportFootstep(FootBone, Surface);
	}
}

// ---------------------------------------------------------------- 데모

ADemo0516_FootstepDust::ADemo0516_FootstepDust()
{
	DemoId = TEXT("0516");
	Controls = TEXT("관찰: Jog 애니메이션의 발 착지 시각에 Anim Notify(FootstepDust)가 들어 있어 발이 닿을 때마다 먼지가 난다.\n")
		TEXT("Notify 는 발 아래 바닥의 Physical Material 을 읽어 흙(PM_Dirt)이면 갈색, 눈(PM_Snow)이면 흰 먼지를 스폰한다");
	Showcase = TEXT("달리는 마네킹의 발 착지 Anim Notify 에서 먼지 Niagara 스폰, 흙/눈 Physical Material 에 따라 색 변경");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Sunset;
}

void ADemo0516_FootstepDust::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 바닥 두 종류: 왼쪽(Y<0) 흙, 오른쪽(Y>0) 눈. 각각 다른 Physical Material 을 덮어쓴다
	DirtMaterial = NewObject<UPhysicalMaterial>(this, TEXT("PM_Dirt"));
	SnowMaterial = NewObject<UPhysicalMaterial>(this, TEXT("PM_Snow"));
	AStaticMeshActor* Dirt = SpawnBlock(World, FVector(0.f, -600.f, 0.f), FVector(1600.f, 1200.f, 10.f), FLinearColor(0.32f, 0.2f, 0.1f));
	AStaticMeshActor* Snow = SpawnBlock(World, FVector(0.f, 600.f, 0.f), FVector(1600.f, 1200.f, 10.f), FLinearColor(0.9f, 0.92f, 0.95f));
	Dirt->GetStaticMeshComponent()->SetPhysMaterialOverride(DirtMaterial);
	Snow->GetStaticMeshComponent()->SetPhysMaterialOverride(SnowMaterial);
	SpawnLabel(World, TEXT("흙 — PM_Dirt"), FVector(0.f, -700.f, 250.f), FLinearColor(1.f, 0.7f, 0.4f), 22.f);
	SpawnLabel(World, TEXT("눈 — PM_Snow"), FVector(0.f, 700.f, 250.f), FLinearColor::White, 22.f);

	// 달리는 마네킹: Jog 를 제자리 재생(Single Node)하고 액터 쪽에서 원을 따라 이동시킨다
	RunnerRoot = NewObject<USceneComponent>(this, TEXT("RunnerRoot"));
	RunnerRoot->SetupAttachment(GetRootComponent());
	RunnerRoot->RegisterComponent();
	Runner = NewObject<USkeletalMeshComponent>(this, TEXT("Runner"));
	Runner->SetupAttachment(RunnerRoot);
	Runner->SetRelativeLocationAndRotation(FVector(0.f, 0.f, 10.f), FRotator(0.f, -90.f, 0.f));   // 마네킹 메시는 +Y 가 앞
	Runner->SetSkeletalMesh(LoadAsset<USkeletalMesh>(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")));
	Runner->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Runner->RegisterComponent();

	JogSequence = LoadAsset<UAnimSequence>(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd"));
	InjectFootstepNotifies();
	if (JogSequence)
	{
		Runner->PlayAnimation(JogSequence, true);
	}
	EffectsKit::SetupFixedCamera(this, FVector(-1300.f, 0.f, 650.f), FVector(0.f, 0.f, 50.f), 60.f);
}

TArray<float> ADemo0516_FootstepDust::FindFootContactTimes(UAnimSequence* Sequence, FName FootBone)
{
	// 애니메이션을 60번 샘플링해 발 뼈의 컴포넌트 공간 높이를 구하고, 가장 낮은 높이 근처로 내려오는 순간 = 착지
	TArray<float> Times;
	const FReferenceSkeleton& Ref = Sequence->GetSkeleton()->GetReferenceSkeleton();
	const int32 FootIndex = Ref.FindBoneIndex(FootBone);
	if (FootIndex == INDEX_NONE)
	{
		return Times;
	}
	constexpr int32 Samples = 60;
	const float Length = Sequence->GetPlayLength();
	TArray<float> Heights;
	for (int32 i = 0; i < Samples; ++i)
	{
		const double T = Length * i / Samples;
		FTransform ComponentSpace = FTransform::Identity;
		for (int32 Bone = FootIndex; Bone != INDEX_NONE; Bone = Ref.GetParentIndex(Bone))
		{
			FTransform Local;
			Sequence->GetBoneTransform(Local, FSkeletonPoseBoneIndex(Bone), FAnimExtractContext(T), false);
			ComponentSpace = ComponentSpace * Local;
		}
		Heights.Add(ComponentSpace.GetLocation().Z);
	}
	const float MinZ = FMath::Min(Heights);
	const float Threshold = MinZ + 2.f;
	for (int32 i = 0; i < Samples; ++i)
	{
		const float Prev = Heights[(i + Samples - 1) % Samples];
		if (Heights[i] <= Threshold && Prev > Threshold)
		{
			Times.Add(Length * i / Samples);
		}
	}
	return Times;
}

void ADemo0516_FootstepDust::InjectFootstepNotifies()
{
	if (!JogSequence)
	{
		return;
	}
	// 실제 프로젝트에서는 애니메이션 에디터의 Notifies 트랙에 직접 놓는다. 여기서는 실행 중 같은 이벤트를 넣는다
	UNiagaraSystem* Dust = EffectsKit::LoadSystem(TEXT("NS_FootDust"));
	for (const FName Foot : { FName(TEXT("foot_l")), FName(TEXT("foot_r")) })
	{
		for (const float Time : FindFootContactTimes(JogSequence, Foot))
		{
			UAnimNotify_FootstepDust* Notify = NewObject<UAnimNotify_FootstepDust>(JogSequence);
			Notify->FootBone = Foot;
			Notify->DustSystem = Dust;
			Notify->SurfaceColors.Add(DirtMaterial, FLinearColor(0.45f, 0.28f, 0.12f));
			Notify->SurfaceColors.Add(SnowMaterial, FLinearColor(1.f, 1.f, 1.f));
			FAnimNotifyEvent& Event = JogSequence->Notifies.AddDefaulted_GetRef();
			Event.Link(JogSequence, Time);
			Event.TriggerTimeOffset = GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::OffsetBefore);
			Event.Notify = Notify;
			Event.NotifyName = FName(*Notify->GetNotifyName_Implementation());
			AddedNotifies.Add(Notify);
			NotifyTimes += FString::Printf(TEXT("%s %.2fs  "), *Foot.ToString(), Time);
		}
	}
	JogSequence->SortNotifies();
	JogSequence->RefreshCacheData();
}

void ADemo0516_FootstepDust::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 공유 에셋에 넣은 Notify 를 다시 뺀다
	if (JogSequence)
	{
		JogSequence->Notifies.RemoveAll([this](const FAnimNotifyEvent& Event) { return AddedNotifies.Contains(Cast<UAnimNotify_FootstepDust>(Event.Notify)); });
		JogSequence->RefreshCacheData();
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0516_FootstepDust::ReportFootstep(FName Foot, const FString& Surface)
{
	++Steps;
	LastStep = FString::Printf(TEXT("%s → %s"), *Foot.ToString(), *Surface);
}

void ADemo0516_FootstepDust::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// 원을 따라 달린다 (반지름 PathRadius, 속도 RunSpeed)
	Angle += RunSpeed / PathRadius * DeltaSeconds;
	const FVector Pos(FMath::Cos(Angle) * PathRadius * 0.6f, FMath::Sin(Angle) * PathRadius, 0.f);
	const FVector Tangent(-FMath::Sin(Angle) * PathRadius * 0.6f, FMath::Cos(Angle) * PathRadius, 0.f);
	if (RunnerRoot)
	{
		RunnerRoot->SetRelativeLocationAndRotation(Pos, Tangent.Rotation());
	}
	SetStatus(FString::Printf(TEXT("발자국 %d · 마지막 %s\nNotify 시각: %s"), Steps, *LastStep, *NotifyTimes));
}
