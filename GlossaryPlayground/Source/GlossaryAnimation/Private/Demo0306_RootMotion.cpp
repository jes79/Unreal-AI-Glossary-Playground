#include "Demo0306_RootMotion.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "D03_Mannequin.h"
#include "GlossaryKit.h"

using namespace GlossaryKit;

ADemo0306_RootMotion::ADemo0306_RootMotion()
{
	DemoId = TEXT("0306");
	Controls = TEXT("E : 지금 대시 재생 (3.5초마다 자동 반복)\n")
		TEXT("관찰: 파란 원판 = 캡슐(실제 캐릭터 위치). 왼쪽은 애니메이션 이동량만큼 캡슐이 따라가고,\n")
		TEXT("오른쪽은 메시만 앞으로 갔다가 끝나면 캡슐 자리로 '툭' 돌아온다 (Root Motion 미사용)");
	Showcase = TEXT("같은 대시 몽타주를 Root Motion 사용/미사용 두 캐릭터로 동시에 재생, 캡슐 위치 원판으로 차이 표시");
	PlayerPawnClass = nullptr;
}

void ADemo0306_RootMotion::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	DashMontage = D03::LoadMontage(TEXT("AM_Dash"));

	StartA = FVector(-300.f, -220.f, 0.f);
	StartB = FVector(-300.f, 220.f, 0.f);
	WithRM = D03::SpawnMannequin(World, StartA, 0.f);
	WithoutRM = D03::SpawnMannequin(World, StartB, 0.f);

	for (AD03_Mannequin* M : { WithRM.Get(), WithoutRM.Get() })
	{
		if (!M) { continue; }
		// 캡슐 위치 표시: 캐릭터(캡슐)에 붙은 파란 원판
		AddShape(M, EShape::Cylinder, FVector(0, 0, -95.f), FVector(90.f, 90.f, 3.f), Colors::Player)->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (UAnimInstance* A = WithRM ? WithRM->GetMesh()->GetAnimInstance() : nullptr)
	{
		A->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);   // 몽타주의 루트 이동 → 캡슐 이동
	}
	if (UAnimInstance* B = WithoutRM ? WithoutRM->GetMesh()->GetAnimInstance() : nullptr)
	{
		B->SetRootMotionMode(ERootMotionMode::NoRootMotionExtraction);       // 루트 이동을 뽑지 않음 → 메시만 움직임
	}

	// 거리 눈금 (1m 간격)
	for (int32 i = 0; i <= 10; ++i)
	{
		for (float Y : { -220.f, 220.f })
		{
			SpawnShape(World, EShape::Cube, FVector(-300.f + i * 100.f, Y + 130.f, 1.f), FVector(4.f, 60.f, 2.f), FLinearColor::White);
		}
		SpawnLabel(World, FString::Printf(TEXT("%dm"), i), FVector(-300.f + i * 100.f, 420.f, 20.f), FLinearColor::White, 13.f);
	}
	SpawnLabel(World, TEXT("Root Motion ON\n(RootMotionFromMontagesOnly)"), StartA + FVector(0, 0, 260.f), Colors::Ally, 17.f);
	SpawnLabel(World, TEXT("Root Motion OFF\n(NoRootMotionExtraction)"), StartB + FVector(0, 0, 260.f), Colors::Enemy, 17.f);

	D03::SpawnViewCamera(World, GetPC(), FVector(180.f, -1500.f, 480.f), FVector(180.f, 0.f, 40.f), 72.f);
	StartDash();
}

void ADemo0306_RootMotion::StartDash()
{
	Timer = 0.f;
	MaxMeshOffsetB = 0.f;
	if (WithRM)
	{
		WithRM->SetActorLocationAndRotation(StartA + FVector(0, 0, 97.f), FRotator::ZeroRotator);
		WithRM->PlayAnimMontage(DashMontage);
	}
	if (WithoutRM)
	{
		WithoutRM->SetActorLocationAndRotation(StartB + FVector(0, 0, 97.f), FRotator::ZeroRotator);
		WithoutRM->PlayAnimMontage(DashMontage);
	}
}

void ADemo0306_RootMotion::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Timer += DeltaSeconds;
	if (WasKeyPressed(EKeys::E) || Timer >= DashInterval)
	{
		StartDash();
	}
	if (!WithRM || !WithoutRM)
	{
		return;
	}
	const float MovedA = FVector::Dist2D(WithRM->GetActorLocation(), StartA);
	const float MovedB = FVector::Dist2D(WithoutRM->GetActorLocation(), StartB);
	// 루트 뼈가 캡슐에서 얼마나 떨어져 있는지 (Root Motion 을 안 쓰면 메시만 앞서 나간다)
	const FVector RootB = WithoutRM->GetMesh()->GetSocketLocation(TEXT("pelvis"));
	MaxMeshOffsetB = FMath::Max(MaxMeshOffsetB, FVector::Dist2D(RootB, WithoutRM->GetActorLocation()));
	SetStatus(FString::Printf(TEXT("왼쪽 (Root Motion ON)  캡슐 이동 %.0f cm\n오른쪽 (OFF)           캡슐 이동 %.0f cm · 메시가 캡슐보다 최대 %.0f cm 앞섬"),
		MovedA, MovedB, MaxMeshOffsetB));
}
