#include "Demo1112_LiveCoding.h"

#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GlossaryKit.h"
#include "GlossaryLabel.h"
#include "Modules/ModuleManager.h"

#if WITH_LIVE_CODING
#include "ILiveCodingModule.h"
#endif

using namespace GlossaryKit;

// =====================================================================================
//  ▼▼▼ Live Coding 실습: 아래 함수 본문의 값을 바꾸고 저장 → 에디터에서 Ctrl+Alt+F11 ▼▼▼
//  (PIE 실행 중이어도 된다. 다음 프레임부터 바뀐 값이 화면에 나온다)
//  예) RotationSpeed 를 90 → 360, Color 를 파랑 → 빨강, Spokes 를 6 → 12, Message 를 "버전 2" 로
// =====================================================================================
FD1112_Tuning ADemo1112_LiveCoding::GetLiveTuning() const
{
	FD1112_Tuning Tuning;
	Tuning.RotationSpeed = 90.f;
	Tuning.Color = FLinearColor(0.10f, 0.35f, 1.00f);
	Tuning.Spokes = 6;
	Tuning.Message = TEXT("버전 1 — 이 문구는 GetLiveTuning() 본문에서 나온다");
	return Tuning;
}
// =====================================================================================

ADemo1112_LiveCoding::ADemo1112_LiveCoding()
{
	DemoId = TEXT("1112");
	Controls = TEXT("에디터에서 이 레벨을 PIE 로 실행 → Demo1112_LiveCoding.cpp 의 GetLiveTuning() 숫자 수정·저장\n")
		TEXT("→ Ctrl+Alt+F11 (Live Coding 컴파일) → 톱니바퀴의 속도·색·바퀴살·문구가 재시작 없이 바뀐다\n")
		TEXT("관찰: 오른쪽 패널의 '패치 적용' 횟수와 'Live Coding 가능 / 재시작 필요' 표");
	Showcase = TEXT("함수 본문 값으로 도는 톱니바퀴, Live Coding 상태·패치 횟수와 재시작이 필요한 변경 표");
	Implementation = EGlossaryImplementation::EditorTool;
	AlternativeNote = TEXT("Live Coding 은 IDE·에디터에서 C++ 을 고치는 작업이라, 레벨은 고칠 함수와 Live Coding 상태를 보여 준다");
	PlayerPawnClass = nullptr;
}

void ADemo1112_LiveCoding::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	// 빈 액터 + 씬 루트에 허브·바퀴살을 붙여 톱니바퀴를 만든다 (카메라를 향하도록 YZ 평면)
	Gear = World->SpawnActor<AActor>(FVector(500.f, 0.f, 320.f), FRotator::ZeroRotator);
	USceneComponent* GearRoot = NewObject<USceneComponent>(Gear, TEXT("GearRoot"));
	GearRoot->SetMobility(EComponentMobility::Movable);
	Gear->SetRootComponent(GearRoot);
	GearRoot->RegisterComponent();
	AddShape(Gear, EShape::Cylinder, FVector::ZeroVector, FVector(120.f, 120.f, 60.f), Colors::Dark, nullptr, FRotator(90.f, 0.f, 0.f));

	MessageLabel = SpawnLabel(World, TEXT(""), FVector(500.f, 0.f, 620.f), FLinearColor::White, 22.f);

	if (APlayerController* PC = GetPC())
	{
		ACameraActor* Cam = World->SpawnActor<ACameraActor>(FVector(-600.f, 0.f, 330.f), FRotator::ZeroRotator);
		PC->SetViewTarget(Cam);
	}
	Panel = UD11_SidePanel::Create(GetPC(), TEXT("Live Coding"), FVector2D(1.f, 0.f), 13.f);

#if WITH_LIVE_CODING
	// 패치가 적용될 때마다 엔진이 알려 준다 → 화면 카운터
	if (ILiveCodingModule* LiveCoding = FModuleManager::GetModulePtr<ILiveCodingModule>(LIVE_CODING_MODULE_NAME))
	{
		PatchHandle = LiveCoding->GetOnPatchCompleteDelegate().AddWeakLambda(this, [this]() { ++PatchCount; });
	}
#endif
}

void ADemo1112_LiveCoding::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
#if WITH_LIVE_CODING
	if (ILiveCodingModule* LiveCoding = FModuleManager::GetModulePtr<ILiveCodingModule>(LIVE_CODING_MODULE_NAME))
	{
		LiveCoding->GetOnPatchCompleteDelegate().Remove(PatchHandle);
	}
#endif
	Super::EndPlay(EndPlayReason);
}

void ADemo1112_LiveCoding::RebuildSpokes(int32 Count)
{
	for (UStaticMeshComponent* Spoke : SpokeMeshes)
	{
		Spoke->DestroyComponent();
	}
	SpokeMeshes.Reset();
	for (int32 i = 0; i < Count; ++i)
	{
		const float Angle = 360.f / Count * i;
		// 바퀴살: 반지름 방향으로 뻗은 막대 (X 축 회전 = Roll)
		const FRotator Rot(0.f, 0.f, Angle);
		const FVector Offset = Rot.RotateVector(FVector(0.f, 0.f, 150.f));
		SpokeMeshes.Add(AddShape(Gear, EShape::Cube, Offset, FVector(30.f, 40.f, 200.f), FLinearColor::White, nullptr, Rot));
	}
	LastColor = FLinearColor::Black; // 색을 다시 칠하게
}

FString ADemo1112_LiveCoding::DescribeLiveCoding() const
{
#if WITH_LIVE_CODING
	if (ILiveCodingModule* LiveCoding = FModuleManager::GetModulePtr<ILiveCodingModule>(LIVE_CODING_MODULE_NAME))
	{
		return FString::Printf(TEXT("모듈 로드됨 · 이번 세션 %s · 기본 설정 %s%s"),
			LiveCoding->IsEnabledForSession() ? TEXT("켜짐") : TEXT("꺼짐"),
			LiveCoding->IsEnabledByDefault() ? TEXT("켜짐") : TEXT("꺼짐"),
			LiveCoding->IsCompiling() ? TEXT(" · 컴파일 중…") : TEXT(""));
	}
	return TEXT("이 실행에는 LiveCoding 모듈이 로드되지 않음 (에디터 PIE 에서 확인)");
#else
	return TEXT("이 빌드는 Live Coding 미지원 (Shipping 등)");
#endif
}

void ADemo1112_LiveCoding::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// 매 프레임 함수를 다시 부른다 → 본문이 패치되면 바로 다음 프레임부터 새 값
	const FD1112_Tuning Tuning = GetLiveTuning();

	if (SpokeMeshes.Num() != Tuning.Spokes)
	{
		RebuildSpokes(FMath::Clamp(Tuning.Spokes, 1, 36));
	}
	if (!Tuning.Color.Equals(LastColor))
	{
		for (UStaticMeshComponent* Spoke : SpokeMeshes)
		{
			SetColor(Spoke, Tuning.Color, 0.4f, 0.f, 0.5f);
		}
		LastColor = Tuning.Color;
	}
	Gear->AddActorLocalRotation(FRotator(0.f, 0.f, Tuning.RotationSpeed * DeltaSeconds));
	if (LastMessage != Tuning.Message)
	{
		LastMessage = Tuning.Message;
		MessageLabel->SetText(LastMessage);
	}

	if (Panel)
	{
		Panel->SetBody(FString::Printf(
			TEXT("상태: %s\n패치 적용 %d회 (이 레벨을 연 뒤)\n\n")
			TEXT("GetLiveTuning() 현재 값\n  RotationSpeed %.0f°/s · Spokes %d\n  Color (%.2f, %.2f, %.2f)\n\n")
			TEXT("Live Coding 으로 되는 것 (Ctrl+Alt+F11)\n")
			TEXT("  · .cpp 함수 본문의 값·로직 수정\n  · 새 지역 변수·새 static 함수(.cpp 안)\n\n")
			TEXT("에디터 재시작이 필요한 것\n")
			TEXT("  · 헤더의 멤버 변수 추가·삭제 (객체 크기·배치 변경)\n")
			TEXT("  · UPROPERTY·UFUNCTION·USTRUCT 추가·변경\n    (리플렉션 정보는 UHT 가 시작 때 등록)\n")
			TEXT("  · 생성자 기본값 (이미 만들어진 CDO 는 그대로)\n  · 새 클래스·모듈, Build.cs 변경\n\n")
			TEXT("설정: Editor Preferences > Live Coding\n  (Enable Reinstancing 을 켜면 일부 헤더 변경도 시도)"),
			*DescribeLiveCoding(), PatchCount, Tuning.RotationSpeed, Tuning.Spokes, Tuning.Color.R, Tuning.Color.G, Tuning.Color.B));
	}
	SetStatus(FString::Printf(TEXT("GetLiveTuning(): %.0f°/s · 바퀴살 %d개 · 패치 %d회"), Tuning.RotationSpeed, Tuning.Spokes, PatchCount));
}
