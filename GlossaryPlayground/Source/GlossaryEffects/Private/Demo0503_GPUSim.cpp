#include "Demo0503_GPUSim.h"

#include "DrawDebugHelpers.h"
#include "EffectsDemoKit.h"
#include "GlossaryKit.h"
#include "NiagaraComponent.h"

using namespace GlossaryKit;

ADemo0503_GPUSim::ADemo0503_GPUSim()
{
	DemoId = TEXT("0503");
	Controls = TEXT("C : GPU Compute Sim ↔ CPU Sim 전환 (같은 10만 개 이미터)   B : Fixed Bounds 상자 표시\n")
		TEXT("관찰: GPU Sim 은 10만 개도 프레임 시간이 거의 그대로, CPU Sim 은 입자 계산 때문에 느려진다.\n")
		TEXT("GPU 이미터는 CPU 가 입자 위치를 모르므로 Fixed Bounds(노란 상자)로 컬링 범위를 정해 준다");
	Showcase = TEXT("10만 입자 이미터 GPU Compute Sim(Fixed Bounds) ↔ CPU Sim 전환, 프레임 시간 비교");
	PlayerPawnClass = nullptr;
	Environment.Sky = EGlossarySky::Night;
	Environment.ExposureEV100 = 12.f;   // 환경 노출 보정 = EV100-10 이라 값이 클수록 밝다 (밤 장면을 조금 밝게)
}

void ADemo0503_GPUSim::BeginPlay()
{
	Super::BeginPlay();
	// 같은 설정의 이미터 두 개: SimTarget 만 GPUComputeSim / CPUSim 으로 다르다
	GPUSystem = EffectsKit::AddNiagara(this, TEXT("NS_GPU100k"), FVector(0.f, 0.f, 500.f));
	CPUSystem = EffectsKit::AddNiagara(this, TEXT("NS_CPU100k"), FVector(0.f, 0.f, 500.f), nullptr, false);
	SpawnLabel(GetWorld(), TEXT("입자 100,000개"), FVector(0.f, 0.f, 1050.f), Colors::Item, 22.f);
	EffectsKit::SetupFixedCamera(this, FVector(-2300.f, -300.f, 700.f), FVector(0.f, -100.f, 480.f), 60.f);
	ApplyMode();
}

void ADemo0503_GPUSim::ApplyMode()
{
	if (GPUSystem && CPUSystem)
	{
		if (bUseGPU) { CPUSystem->Deactivate(); CPUSystem->SetVisibility(false); GPUSystem->SetVisibility(true); GPUSystem->Activate(true); }
		else         { GPUSystem->Deactivate(); GPUSystem->SetVisibility(false); CPUSystem->SetVisibility(true); CPUSystem->Activate(true); }
	}
	SmoothedMs = 0.f;
}

void ADemo0503_GPUSim::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasKeyPressed(EKeys::C)) { bUseGPU = !bUseGPU; ApplyMode(); }
	if (WasKeyPressed(EKeys::B)) { bShowBounds = !bShowBounds; }

	const float Ms = DeltaSeconds * 1000.f;
	SmoothedMs = SmoothedMs <= 0.f ? Ms : FMath::Lerp(SmoothedMs, Ms, 0.05f);

	if (bShowBounds && bUseGPU)
	{
		// 빌더에서 GPU 이미터에 준 Fixed Bounds 와 같은 상자
		DrawDebugBox(GetWorld(), FVector(0.f, 0.f, 500.f), FixedBoundsExtent, FColor::Yellow, false, -1.f, 0, 3.f);
	}
	SetStatus(FString::Printf(TEXT("%s · 입자 %d 개 · 프레임 %.1f ms (%.0f FPS)%s"),
		bUseGPU ? TEXT("GPU Compute Sim") : TEXT("CPU Sim"), ParticleCount, SmoothedMs, SmoothedMs > 0.f ? 1000.f / SmoothedMs : 0.f,
		bUseGPU ? TEXT(" · Fixed Bounds 900cm 상자") : TEXT(" · 경계는 CPU 가 매 프레임 계산")));
}
