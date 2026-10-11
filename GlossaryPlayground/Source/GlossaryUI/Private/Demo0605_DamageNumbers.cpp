#include "Demo0605_DamageNumbers.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

// ---------------------------------------------------------------- 레이어 위젯

void UDemo0605_DamageLayer::BuildTree()
{
	Canvas = MakeRootCanvas();
}

void UDemo0605_DamageLayer::SpawnNumber(const FVector& WorldLocation, int32 Damage, bool bCritical)
{
	if (!Canvas)
	{
		return;
	}
	// 치명타: 노란색 + 큰 글자 + 느낌표. 일반: 흰색
	UTextBlock* Text = MakeText(bCritical ? FString::Printf(TEXT("%d!"), Damage) : FString::FromInt(Damage),
		bCritical ? 44.f : 26.f, bCritical ? FLinearColor(1.f, 0.85f, 0.05f) : FLinearColor::White, true);
	Text->SetShadowOffset(FVector2D(2.f, 2.f));
	Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f));
	UCanvasPanelSlot* CSlot = Canvas->AddChildToCanvas(Text);
	CSlot->SetAutoSize(true);
	CSlot->SetAlignment(FVector2D(0.5f, 1.f)); // 숫자의 아래 가운데가 맞은 지점
	Text->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

	FDemo0605FloatingNumber& N = Numbers.AddDefaulted_GetRef();
	N.Text = Text;
	// 같은 자리에 겹치지 않게 좌우로 조금 흩뿌린다
	N.WorldLocation = WorldLocation + FVector(0.f, FMath::FRandRange(-40.f, 40.f), 0.f);
	N.bCritical = bCritical;
}

void UDemo0605_DamageLayer::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	APlayerController* PC = GetOwningPlayer();
	for (int32 i = Numbers.Num() - 1; i >= 0; --i)
	{
		FDemo0605FloatingNumber& N = Numbers[i];
		N.Age += InDeltaTime;
		const float T = N.Age / Lifetime;
		if (T >= 1.f || !N.Text)
		{
			if (N.Text)
			{
				N.Text->RemoveFromParent();
			}
			Numbers.RemoveAtSwap(i);
			continue;
		}
		// 1) 위로 떠오르기 (월드 공간에서 올린 뒤 화면으로 투영)
		N.WorldLocation.Z += RiseSpeed * InDeltaTime;
		FVector2D ScreenPos;
		const bool bOnScreen = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, N.WorldLocation, ScreenPos, false);
		N.Text->SetVisibility(bOnScreen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (UCanvasPanelSlot* CSlot = Cast<UCanvasPanelSlot>(N.Text->Slot))
		{
			CSlot->SetPosition(ScreenPos);
		}
		// 2) 튀어나오는 크기: 처음 0.15초 동안 1.6배 → 1배 (치명타는 더 크게)
		const float Pop = N.bCritical ? 2.0f : 1.6f;
		const float Scale = T < 0.15f ? FMath::Lerp(Pop, 1.f, T / 0.15f) : 1.f;
		N.Text->SetRenderScale(FVector2D(Scale, Scale));
		// 3) 마지막 40% 동안 서서히 사라짐
		N.Text->SetRenderOpacity(T > 0.6f ? 1.f - (T - 0.6f) / 0.4f : 1.f);
	}
}

// ---------------------------------------------------------------- 데모

ADemo0605_DamageNumbers::ADemo0605_DamageNumbers()
{
	DemoId = TEXT("0605");
	Controls = TEXT("적 클릭 : 공격   K : 치명타로 공격   A : 자동 공격 ON/OFF\n")
		TEXT("관찰: 맞은 적 머리 위에 숫자가 튀어나와 떠오르며 사라진다. 치명타(25%)는 노란색으로 크게");
	Showcase = TEXT("적 머리 위 데미지 숫자 팝업·상승·페이드, 치명타는 노란색 큰 글자");
	PlayerPawnClass = nullptr;
	bShowMouseCursor = true;
	PlayerStartTransform = FTransform(FRotator(-8.f, 0.f, 0.f), FVector(-700.f, 0.f, 250.f));
}

void ADemo0605_DamageNumbers::BeginPlay()
{
	Super::BeginPlay();
	for (int32 i = 0; i < 3; ++i)
	{
		const FVector Loc(200.f, -350.f + 350.f * i, 90.f);
		Enemies.Add(SpawnShape(GetWorld(), EShape::Cylinder, Loc, FVector(100.f, 100.f, 180.f), Colors::Enemy));
	}
	Layer = CreateWidget<UDemo0605_DamageLayer>(GetPC(), UDemo0605_DamageLayer::StaticClass());
	Layer->Lifetime = NumberLifetime;
	Layer->SetVisibility(ESlateVisibility::HitTestInvisible); // 클릭이 월드로 통과하도록
	Layer->AddToViewport(0);
}

void ADemo0605_DamageNumbers::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Layer)
	{
		Layer->RemoveFromParent();
	}
	Super::EndPlay(EndPlayReason);
}

void ADemo0605_DamageNumbers::HitEnemy(AStaticMeshActor* Enemy, bool bForceCrit)
{
	const bool bCrit = bForceCrit || FMath::FRand() < CriticalChance;
	const int32 Base = FMath::RandRange(12, 30);
	const int32 Damage = bCrit ? FMath::RoundToInt(Base * CriticalMultiplier) : Base;
	// 머리 위(높이 180cm 원기둥 꼭대기 + 30cm)에서 시작
	Layer->SpawnNumber(Enemy->GetActorLocation() + FVector(0.f, 0.f, 120.f), Damage, bCrit);
	++TotalHits;
	TotalCrits += bCrit ? 1 : 0;
}

void ADemo0605_DamageNumbers::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Layer || Enemies.IsEmpty())
	{
		return;
	}
	if (WasKeyPressed(EKeys::A))
	{
		bAutoAttack = !bAutoAttack;
	}
	if (WasKeyPressed(EKeys::K))
	{
		HitEnemy(Enemies[FMath::RandRange(0, Enemies.Num() - 1)], true);
	}
	if (WasKeyPressed(EKeys::LeftMouseButton))
	{
		FHitResult Hit;
		if (GetMouseWorldHit(Hit))
		{
			if (AStaticMeshActor* Enemy = Cast<AStaticMeshActor>(Hit.GetActor()); Enemy && Enemies.Contains(Enemy))
			{
				HitEnemy(Enemy);
			}
		}
	}
	AttackTimer += DeltaSeconds;
	if (bAutoAttack && AttackTimer >= AutoAttackInterval)
	{
		AttackTimer = 0.f;
		HitEnemy(Enemies[FMath::RandRange(0, Enemies.Num() - 1)]);
	}
	SetStatus(FString::Printf(TEXT("자동 공격 %s · 화면의 숫자 %d개 · 누적 %d타 (치명타 %d)\n치명타 확률 %.0f%% · 배율 x%.1f · 수명 %.1fs"),
		bAutoAttack ? TEXT("ON") : TEXT("OFF"), Layer->GetAliveCount(), TotalHits, TotalCrits,
		CriticalChance * 100.f, CriticalMultiplier, NumberLifetime));
}
