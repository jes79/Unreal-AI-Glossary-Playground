#include "Demo0906_AudioVolume.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryAudioKit.h"
#include "GlossaryKit.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/AudioVolume.h"
#include "Sound/ReverbEffect.h"
#include "Sound/SoundWave.h"

#if WITH_EDITOR
#include "BSPOps.h"
#include "Builders/CubeBuilder.h"
#include "Components/BrushComponent.h"
#include "Engine/Polys.h"
#include "Model.h"
#endif

using namespace GlossaryKit;

namespace
{
	const FVector OutsideSpot(-250.f, 0.f, 100.f);
	const FVector InsideSpot(1150.f, 0.f, 100.f);
}

ADemo0906_AudioVolume::ADemo0906_AudioVolume()
{
	DemoId = TEXT("0906");
	Controls = TEXT("WASD 이동 · 마우스 시점 · T : 동굴 안/밖 순간이동 · F : 손뼉 · 2 : 자동 손뼉 ON/OFF\n")
		TEXT("C : Audio Volume 켜기/끄기\n")
		TEXT("관찰: 동굴(하늘색 경계) 안에서 손뼉을 치면 RVB_Cave 리버브로 4초 동안 울린다 — 메아리 링이 여러 겹 오래 퍼진다");
	Showcase = TEXT("동굴에 Audio Volume + Reverb Effect(RVB_Cave, Decay 4초), 들어가면 손뼉이 울림, 메아리 링·현재 볼륨 표시");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, OutsideSpot);
}

#if WITH_EDITOR
void ADemo0906_AudioVolume::BuildLevel(UWorld* World)
{
	// Audio Volume 은 브러시(모양)가 있어야 하므로 실행 중 스폰이 아니라 레벨에 저장한다 (UActorFactoryBoxVolume 과 같은 순서)
	AAudioVolume* Volume = World->SpawnActor<AAudioVolume>(CaveCenter, FRotator::ZeroRotator);
	Volume->PolyFlags = 0;
	Volume->Brush = NewObject<UModel>(Volume, NAME_None, RF_Transactional);
	Volume->Brush->Initialize(nullptr, true);
	Volume->Brush->Polys = NewObject<UPolys>(Volume->Brush, NAME_None, RF_Transactional);
	Volume->GetBrushComponent()->Brush = Volume->Brush;
	UCubeBuilder* Builder = NewObject<UCubeBuilder>(Volume);
	Builder->X = CaveExtent.X * 2.f;
	Builder->Y = CaveExtent.Y * 2.f;
	Builder->Z = CaveExtent.Z * 2.f;
	Volume->BrushBuilder = Builder;
	Builder->Build(World, Volume);
	FBSPOps::csgPrepMovingBrush(Volume);
	Volume->GetBrushComponent()->RequestUpdateBrushCollision();

	// 핵심: Reverb Settings 에 리버브 이펙트 에셋을 넣는다
	FReverbSettings Reverb;
	Reverb.bApplyReverb = true;
	Reverb.ReverbEffect = LoadObject<UReverbEffect>(nullptr, *(GlossaryAudioKit::AssetPath(TEXT("RVB_Cave")) + TEXT(".RVB_Cave")));
	Reverb.Volume = 0.8f;
	Reverb.FadeTime = 0.5f;
	Volume->SetReverbSettings(Reverb);
	Volume->SetPriority(1.f);
	Volume->SetActorLabel(TEXT("CaveAudioVolume"));
	Volume->PostEditChange();
}
#endif

void ADemo0906_AudioVolume::BeginPlay()
{
	Super::BeginPlay();
	GlossaryAudioKit::PrepareAudioDemo();
	UWorld* World = GetWorld();

	for (TActorIterator<AAudioVolume> It(World); It; ++It)
	{
		CaveVolume = *It;
	}
	ClapSound = LoadAsset<USoundWave>(GlossaryAudioKit::AssetPath(TEXT("S_Clap")));

	// 동굴: 양쪽 벽 + 천장 + 안쪽 벽 (입구는 -X 쪽)
	const FVector C = CaveCenter;
	const FVector E = CaveExtent;
	const FLinearColor Rock(0.22f, 0.2f, 0.18f);
	SpawnBlock(World, FVector(C.X, C.Y - E.Y - 50.f, 0.f), FVector(E.X * 2.f, 100.f, E.Z * 2.f + 40.f), Rock);
	SpawnBlock(World, FVector(C.X, C.Y + E.Y + 50.f, 0.f), FVector(E.X * 2.f, 100.f, E.Z * 2.f + 40.f), Rock);
	SpawnBlock(World, FVector(C.X + E.X + 50.f, C.Y, 0.f), FVector(100.f, E.Y * 2.f + 200.f, E.Z * 2.f + 40.f), Rock);
	SpawnBlock(World, FVector(C.X, C.Y, E.Z * 2.f + 10.f), FVector(E.X * 2.f + 100.f, E.Y * 2.f + 200.f, 60.f), Rock);
	SpawnLabel(World, TEXT("동굴 (Audio Volume + RVB_Cave)"), FVector(C.X - E.X - 40.f, C.Y, E.Z * 2.f + 140.f), Colors::Cyan, 20.f);

	// Audio Volume 경계를 하늘색 선(12개 모서리)으로 그린다 — 볼륨 자체는 게임에서 보이지 않으므로
	const FVector Min = C - E + FVector(0.f, 0.f, 0.f);
	const FVector Max = C + E;
	auto Edge = [&](const FVector& A, const FVector& B)
	{
		const FVector Mid = (A + B) * 0.5f;
		const FVector Size(FMath::Max(FMath::Abs(B.X - A.X), 6.f), FMath::Max(FMath::Abs(B.Y - A.Y), 6.f), FMath::Max(FMath::Abs(B.Z - A.Z), 6.f));
		UStaticMeshComponent* Line = AddShape(this, EShape::Cube, Mid, Size, Colors::Cyan);
		SetColor(Line, Colors::Cyan, 0.5f, 0.f, 4.f);
		Line->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Line->SetCastShadow(false);
	};
	for (const float Z : { Min.Z + 3.f, Max.Z - 3.f })
	{
		Edge(FVector(Min.X, Min.Y, Z), FVector(Max.X, Min.Y, Z));
		Edge(FVector(Min.X, Max.Y, Z), FVector(Max.X, Max.Y, Z));
		Edge(FVector(Min.X, Min.Y, Z), FVector(Min.X, Max.Y, Z));
		Edge(FVector(Max.X, Min.Y, Z), FVector(Max.X, Max.Y, Z));
	}
	for (const float X : { Min.X, Max.X })
	{
		for (const float Y : { Min.Y, Max.Y })
		{
			Edge(FVector(X, Y, Min.Z), FVector(X, Y, Max.Z));
		}
	}

	Panel = UGlossaryAudioPanel::Create(this, TEXT("Audio Volume · Reverb"));
	if (Panel)
	{
		Panel->AddRow(TEXT("리스너 위치"), Colors::Cyan);
		Panel->AddRow(TEXT("Reverb Decay"), Colors::Purple);
		Panel->AddRow(TEXT("Reverb Volume"), Colors::Ally);
		Panel->SetFooter(TEXT("UWorld::GetAudioSettings(리스너 위치) 가 돌려준 Audio Volume 의 Reverb Settings"));
		Panel->AddToViewport(1);
	}
	if (!CaveVolume)
	{
		SetStatus(TEXT("레벨에 Audio Volume 이 없음 — tools/glossary-build.sh -filter=0906 으로 레벨을 다시 만드세요"));
	}
}

void ADemo0906_AudioVolume::Clap()
{
	ACharacter* Character = GetPlayerCharacter();
	if (!ClapSound || !Character)
	{
		return;
	}
	const FVector Loc = Character->GetActorLocation();
	UGameplayStatics::PlaySoundAtLocation(this, ClapSound, Loc);
	++ClapCount;

	// 리버브가 걸리는지는 "리스너"(카메라) 위치의 Audio Volume 으로 정해진다
	FVector ListenerLoc;
	FRotator ListenerRot;
	FReverbSettings Reverb;
	FInteriorSettings Interior;
	AAudioVolume* Current = GlossaryAudioKit::GetListener(this, ListenerLoc, ListenerRot) ? GetWorld()->GetAudioSettings(ListenerLoc, &Reverb, &Interior) : nullptr;
	const bool bReverb = Current && Current->GetEnabled() && Reverb.bApplyReverb && Reverb.ReverbEffect;
	bInsideLastClap = bReverb;
	const FVector Floor(Loc.X, Loc.Y, 2.f);
	if (bReverb)
	{
		// 리버브: 메아리 링 4겹, Decay 시간 동안 천천히 퍼진다
		const float Decay = Reverb.ReverbEffect->DecayTime;
		for (int32 i = 0; i < 4; ++i)
		{
			SpawnEcho(Floor, i * 0.25f, Decay * (1.f - i * 0.15f), Colors::Cyan, 450.f + i * 120.f);
		}
	}
	else
	{
		SpawnEcho(Floor, 0.f, 0.35f, FLinearColor::White, 250.f);
	}
}

void ADemo0906_AudioVolume::SpawnEcho(const FVector& Location, float Delay, float Lifetime, const FLinearColor& Color, float MaxRadius)
{
	FEcho Echo;
	Echo.Ring = GlossaryAudioKit::SpawnRing(GetWorld(), Location, 100.f, Color, 40, 3.f, 3.f);
	if (Echo.Ring.IsValid())
	{
		Echo.Ring->SetActorHiddenInGame(Delay > 0.f);
		Echo.Ring->SetActorScale3D(FVector(0.2f, 0.2f, 1.f));
	}
	Echo.Delay = Delay;
	Echo.Lifetime = Lifetime;
	Echo.MaxRadius = MaxRadius;
	Echoes.Add(Echo);
}

void ADemo0906_AudioVolume::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (WasKeyPressed(EKeys::T))
	{
		if (ACharacter* Character = GetPlayerCharacter())
		{
			const bool bInside = Character->GetActorLocation().X > CaveCenter.X - CaveExtent.X;
			Character->SetActorLocation(bInside ? OutsideSpot : InsideSpot, false, nullptr, ETeleportType::TeleportPhysics);
			if (APlayerController* PC = GetPC())
			{
				PC->SetControlRotation(FRotator(-10.f, 0.f, 0.f));
			}
		}
	}
	if (WasKeyPressed(EKeys::C) && CaveVolume)
	{
		CaveVolume->SetEnabled(!CaveVolume->GetEnabled());
	}
	if (WasKeyPressed(EKeys::Two))
	{
		bAutoClap = !bAutoClap;
	}
	if (WasKeyPressed(EKeys::F))
	{
		Clap();
	}
	if (bAutoClap)
	{
		ClapTimer += DeltaSeconds;
		if (ClapTimer >= ClapInterval)
		{
			ClapTimer = 0.f;
			Clap();
		}
	}

	// 메아리 링 갱신
	for (int32 i = Echoes.Num() - 1; i >= 0; --i)
	{
		FEcho& Echo = Echoes[i];
		AActor* Ring = Echo.Ring.Get();
		if (!Ring)
		{
			Echoes.RemoveAt(i);
			continue;
		}
		if (Echo.Delay > 0.f)
		{
			Echo.Delay -= DeltaSeconds;
			Ring->SetActorHiddenInGame(Echo.Delay > 0.f);
			continue;
		}
		Echo.Age += DeltaSeconds;
		const float T = Echo.Age / FMath::Max(Echo.Lifetime, 0.01f);
		if (T >= 1.f)
		{
			Ring->Destroy();
			Echoes.RemoveAt(i);
			continue;
		}
		const float Radius = FMath::Lerp(20.f, Echo.MaxRadius, FMath::Sqrt(T));
		Ring->SetActorScale3D(FVector(Radius / 100.f, Radius / 100.f, 1.f - T * 0.8f));
	}

	// 현재 리스너가 어떤 Audio Volume 안에 있는지
	FVector ListenerLoc;
	FRotator ListenerRot;
	FReverbSettings Reverb;
	FInteriorSettings Interior;
	AAudioVolume* Current = GlossaryAudioKit::GetListener(this, ListenerLoc, ListenerRot) ? GetWorld()->GetAudioSettings(ListenerLoc, &Reverb, &Interior) : nullptr;
	const bool bInVolume = Current != nullptr && Current == CaveVolume;
	const bool bReverb = bInVolume && Current->GetEnabled() && Reverb.bApplyReverb && Reverb.ReverbEffect;
	const float Decay = bReverb ? Reverb.ReverbEffect->DecayTime : 0.f;
	if (Panel)
	{
		Panel->SetRow(0, bInVolume ? 1.f : 0.f, bInVolume ? TEXT("동굴 안") : TEXT("밖"));
		Panel->SetRow(1, Decay / 5.f, bReverb ? FString::Printf(TEXT("%.1f 초"), Decay) : TEXT("리버브 없음"));
		Panel->SetRow(2, bReverb ? Reverb.Volume : 0.f, bReverb ? FString::Printf(TEXT("%.1f"), Reverb.Volume) : TEXT("-"));
	}
	SetStatus(FString::Printf(TEXT("리스너: %s · Audio Volume %s · 손뼉 %d번 (자동 %s)\n%s"),
		bInVolume ? TEXT("동굴 Audio Volume 안") : TEXT("밖"),
		CaveVolume ? (CaveVolume->GetEnabled() ? TEXT("켜짐") : TEXT("꺼짐")) : TEXT("없음"),
		ClapCount, bAutoClap ? TEXT("ON") : TEXT("OFF"),
		bReverb ? *FString::Printf(TEXT("리버브 %s 적용 (Decay %.1f초) → 손뼉이 길게 울린다"), *Reverb.ReverbEffect->GetName(), Decay)
		        : TEXT("리버브 없음 → 손뼉이 짧게 끝난다")));
}
