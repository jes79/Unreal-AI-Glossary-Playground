#include "Demo0815_LevelStreaming.h"

#include "Components/BoxComponent.h"
#include "D08_Common.h"
#include "Engine/Level.h"
#include "Engine/LevelStreaming.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/World.h"
#include "GlossaryKit.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"

using namespace GlossaryKit;

const TCHAR* ADemo0815_LevelStreaming::SubLevelA = TEXT("/Game/Glossary/08_Systems/L_0815_SubA");
const TCHAR* ADemo0815_LevelStreaming::SubLevelB = TEXT("/Game/Glossary/08_Systems/L_0815_SubB");

ADemo0815_LevelStreaming::ADemo0815_LevelStreaming()
{
	DemoId = TEXT("0815");
	// 플레이어 컨트롤러(입력 처리) 다음에 Tick 해서 WasKeyPressed 를 놓치지 않게
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	Controls = TEXT("WASD 이동 · 바닥의 파란/보라 트리거 칸에 들어가면 서브레벨 Load, 나오면 Unload\n")
		TEXT("1 : A 칸으로 순간이동 · 2 : B 칸으로 · 3 : 시작 지점으로 (테스트)\n")
		TEXT("관찰: 트리거 너머의 건물·나무(서브레벨 액터)가 들어갈 때 나타나고 나오면 사라진다");
	Showcase = TEXT("서브레벨 2개를 트리거 박스에 들어가면 LoadStreamLevel, 나오면 UnloadStreamLevel — 상태 실시간 표시");
}

#if WITH_EDITOR
void ADemo0815_LevelStreaming::BuildLevel(UWorld* World)
{
	// 퍼시스턴트 레벨의 Levels 창에 서브레벨 두 개를 등록한다 (처음에는 로드하지 않음)
	for (const TCHAR* Path : { SubLevelA, SubLevelB })
	{
		if (!FPackageName::DoesPackageExist(Path))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Glossary] 0815 서브레벨 없음: %s (08_Systems 에셋 빌더 먼저)"), Path);
			continue;
		}
		ULevelStreamingDynamic* Streaming = NewObject<ULevelStreamingDynamic>(World, ULevelStreamingDynamic::StaticClass(), NAME_None, RF_NoFlags);
		Streaming->SetWorldAssetByPackageName(FName(Path));
		Streaming->bInitiallyLoaded = false;
		Streaming->bInitiallyVisible = false;
		Streaming->SetShouldBeLoaded(false);
		Streaming->SetShouldBeVisible(false);
		World->AddStreamingLevel(Streaming);
	}
}
#endif

UBoxComponent* ADemo0815_LevelStreaming::MakeTrigger(const FVector& Location, const FLinearColor& Color, const FString& Label)
{
	UBoxComponent* Box = NewObject<UBoxComponent>(this);
	Box->SetupAttachment(Root);
	Box->SetWorldLocation(Location + FVector(0.f, 0.f, TriggerSize.Z * 0.5f));
	Box->SetBoxExtent(TriggerSize * 0.5f);
	Box->SetCollisionProfileName(TEXT("Trigger"));
	Box->RegisterComponent();
	// 바닥에 칸 표시
	SpawnBlock(GetWorld(), Location, FVector(TriggerSize.X, TriggerSize.Y, 3.f), Color);
	SpawnLabel(GetWorld(), Label, Location + FVector(0.f, 0.f, 80.f), Color, 20.f);
	return Box;
}

void ADemo0815_LevelStreaming::BeginPlay()
{
	Super::BeginPlay();
	TriggerA = MakeTrigger(FVector(300.f, -700.f, 0.f), Colors::Cyan, TEXT("트리거 A → L_0815_SubA"));
	TriggerB = MakeTrigger(FVector(300.f, 700.f, 0.f), Colors::Purple, TEXT("트리거 B → L_0815_SubB"));
	TriggerA->OnComponentBeginOverlap.AddDynamic(this, &ADemo0815_LevelStreaming::OnEnterA);
	TriggerA->OnComponentEndOverlap.AddDynamic(this, &ADemo0815_LevelStreaming::OnExitA);
	TriggerB->OnComponentBeginOverlap.AddDynamic(this, &ADemo0815_LevelStreaming::OnEnterB);
	TriggerB->OnComponentEndOverlap.AddDynamic(this, &ADemo0815_LevelStreaming::OnExitB);

	// 서브레벨 자리 표시 (비어 있을 때도 어디에 나타나는지 보이게)
	SpawnLabel(GetWorld(), TEXT("서브레벨 A 자리"), FVector(1400.f, -800.f, 40.f), Colors::Cyan, 16.f);
	SpawnLabel(GetWorld(), TEXT("서브레벨 B 자리"), FVector(1400.f, 800.f, 40.f), Colors::Purple, 16.f);

	Panel = UD08_SidePanel::Create(GetPC(), TEXT("Streaming Levels"), UD08_SidePanel::EPlace::TopRight, 460.f);
	if (GetWorld()->GetStreamingLevels().Num() == 0)
	{
		LastEvent = TEXT("등록된 서브레벨이 없다 — tools/glossary-build.sh -filter=08 로 다시 빌드");
	}
}

void ADemo0815_LevelStreaming::Stream(const TCHAR* LevelPath, bool bLoad)
{
	// Latent 함수라 UUID 가 호출마다 달라야 한다. 완료되면 OnStreamingDone 이 불린다
	FLatentActionInfo Latent;
	Latent.CallbackTarget = this;
	Latent.ExecutionFunction = GET_FUNCTION_NAME_CHECKED(ADemo0815_LevelStreaming, OnStreamingDone);
	Latent.Linkage = 0;
	Latent.UUID = NextUUID++;
	const FName LevelName(*FPackageName::GetShortName(LevelPath));
	if (bLoad)
	{
		// 핵심: 이름으로 서브레벨을 불러와 보이게 (bShouldBlockOnLoad=false → 끊김 없이 백그라운드 로드)
		UGameplayStatics::LoadStreamLevel(this, LevelName, /*bMakeVisibleAfterLoad*/ true, /*bShouldBlockOnLoad*/ false, Latent);
	}
	else
	{
		UGameplayStatics::UnloadStreamLevel(this, LevelName, Latent, /*bShouldBlockOnUnload*/ false);
	}
	LastEvent = FString::Printf(TEXT("%s(\"%s\") 요청"), bLoad ? TEXT("LoadStreamLevel") : TEXT("UnloadStreamLevel"), *LevelName.ToString());
}

void ADemo0815_LevelStreaming::OnEnterA(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (Other == GetPlayerPawn()) { Stream(SubLevelA, true); }
}
void ADemo0815_LevelStreaming::OnExitA(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32)
{
	if (Other == GetPlayerPawn()) { Stream(SubLevelA, false); }
}
void ADemo0815_LevelStreaming::OnEnterB(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (Other == GetPlayerPawn()) { Stream(SubLevelB, true); }
}
void ADemo0815_LevelStreaming::OnExitB(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32)
{
	if (Other == GetPlayerPawn()) { Stream(SubLevelB, false); }
}

void ADemo0815_LevelStreaming::OnStreamingDone()
{
	++CallbackCount;
	LastEvent += TEXT(" → 완료");
}

FString ADemo0815_LevelStreaming::DescribeLevel(const TCHAR* LevelPath) const
{
	const FName LevelName(*FPackageName::GetShortName(LevelPath));
	ULevelStreaming* Streaming = UGameplayStatics::GetStreamingLevel(this, LevelName);
	if (!Streaming)
	{
		return FString::Printf(TEXT("%s : 등록 안 됨"), *LevelName.ToString());
	}
	const bool bLoaded = Streaming->IsLevelLoaded();
	const bool bVisible = Streaming->IsLevelVisible();
	const int32 Actors = bLoaded && Streaming->GetLoadedLevel() ? Streaming->GetLoadedLevel()->Actors.Num() : 0;
	return FString::Printf(TEXT("%s : %s%s · 액터 %d개"), *LevelName.ToString(),
		bLoaded ? TEXT("로드됨") : (Streaming->ShouldBeLoaded() ? TEXT("로드 중…") : TEXT("언로드")),
		bLoaded ? (bVisible ? TEXT(" · 보임") : TEXT(" · 숨김")) : TEXT(""), Actors);
}

void ADemo0815_LevelStreaming::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (APawn* Pawn = GetPlayerPawn())
	{
		// 테스트용 순간이동 (Sweep 없이 옮겨도 다음 물리 갱신에서 Overlap 이 갱신된다)
		const FVector Targets[] = { FVector(300.f, -700.f, 100.f), FVector(300.f, 700.f, 100.f), PlayerStartTransform.GetLocation() };
		const FKey Keys[] = { EKeys::One, EKeys::Two, EKeys::Three };
		for (int32 i = 0; i < 3; ++i)
		{
			if (WasKeyPressed(Keys[i]))
			{
				Pawn->SetActorLocation(Targets[i], false, nullptr, ETeleportType::TeleportPhysics);
				Pawn->UpdateOverlaps();
			}
		}
	}
	if (Panel)
	{
		Panel->SetBody(FString::Printf(TEXT("%s\n%s\n\n완료 콜백 %d회\nUGameplayStatics::LoadStreamLevel / UnloadStreamLevel"),
			*DescribeLevel(SubLevelA), *DescribeLevel(SubLevelB), CallbackCount));
	}
	SetStatus(LastEvent.IsEmpty() ? TEXT("트리거 칸으로 들어가 보자") : LastEvent);
}
