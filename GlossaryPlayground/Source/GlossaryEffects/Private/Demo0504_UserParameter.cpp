#include "Demo0504_UserParameter.h"

#include "Components/StaticMeshComponent.h"
#include "EffectsDemoKit.h"
#include "GlossaryKit.h"
#include "NiagaraComponent.h"

using namespace GlossaryKit;

ADemo0504_UserParameter::ADemo0504_UserParameter()
{
	DemoId = TEXT("0504");
	Controls = TEXT("1 파랑 · 2 빨강 · 3 초록 · 4 노랑 : User.Color 바꾸기   [ / ] : User.SpawnRate -20 / +20\n")
		TEXT("관찰: 에셋을 고치지 않고 C++ 의 SetVariableLinearColor(\"Color\") / SetVariableFloat(\"SpawnRate\") 만으로\n")
		TEXT("실행 중인 이펙트의 색과 양이 바뀐다 (이름에는 \"User.\" 를 붙이지 않는다)");
	Showcase = TEXT("User.Color · User.SpawnRate 를 노출한 분수 이펙트, 숫자 키로 C++ 에서 색·양 변경");
	PlayerPawnClass = nullptr;
}

void ADemo0504_UserParameter::BeginPlay()
{
	Super::BeginPlay();
	SpawnBlock(GetWorld(), FVector::ZeroVector, FVector(200.f, 200.f, 40.f), Colors::Platform);
	Fountain = EffectsKit::AddNiagara(this, TEXT("NS_UserParam"), FVector(0.f, 0.f, 45.f));
	// 현재 색을 보여 주는 작은 견본 큐브
	Swatch = AddShape(this, EShape::Cube, FVector(0.f, 260.f, 60.f), FVector(60.f, 60.f, 60.f), Color);
	SpawnLabel(GetWorld(), TEXT("현재 User.Color"), FVector(0.f, 260.f, 150.f), FLinearColor::White, 16.f);
	EffectsKit::SetupFixedCamera(this, FVector(-900.f, 0.f, 300.f), FVector(0.f, 0.f, 220.f), 65.f);
	Apply();
}

void ADemo0504_UserParameter::Apply()
{
	if (Fountain)
	{
		// 핵심 두 줄: 시스템이 노출한 User 파라미터에 값 넣기
		Fountain->SetVariableLinearColor(TEXT("Color"), Color);
		Fountain->SetVariableFloat(TEXT("SpawnRate"), SpawnRate);
	}
	if (Swatch)
	{
		SetColor(Swatch, Color, 0.5f, 0.f, 2.f);
	}
}

void ADemo0504_UserParameter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	struct FPreset { FKey Key; FLinearColor Color; const TCHAR* Name; };
	const FPreset Presets[] = {
		{ EKeys::One, FLinearColor(0.1f, 0.6f, 1.f), TEXT("파랑") },
		{ EKeys::Two, FLinearColor(1.f, 0.1f, 0.05f), TEXT("빨강") },
		{ EKeys::Three, FLinearColor(0.1f, 1.f, 0.2f), TEXT("초록") },
		{ EKeys::Four, FLinearColor(1.f, 0.8f, 0.05f), TEXT("노랑") },
	};
	bool bChanged = false;
	for (const FPreset& Preset : Presets)
	{
		if (WasKeyPressed(Preset.Key)) { Color = Preset.Color; ColorName = Preset.Name; bChanged = true; }
	}
	if (WasKeyPressed(EKeys::LeftBracket))  { SpawnRate = FMath::Max(0.f, SpawnRate - 20.f); bChanged = true; }
	if (WasKeyPressed(EKeys::RightBracket)) { SpawnRate = FMath::Min(400.f, SpawnRate + 20.f); bChanged = true; }
	if (bChanged)
	{
		Apply();
	}
	SetStatus(FString::Printf(TEXT("User.Color = %s (%.2f, %.2f, %.2f) · User.SpawnRate = %.0f /초"),
		*ColorName, Color.R, Color.G, Color.B, SpawnRate));
}
