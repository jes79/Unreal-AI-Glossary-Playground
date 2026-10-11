// 05_Effects 의 Niagara System(NS_*) 을 템플릿 이미터 + 모듈 입력값 설정으로 조립한다.
// 실행: tools/glossary-build.sh -filter=05   (특정 시스템만: -NSOnly=NS_Campfire,NS_Beam)
#include "CoreMinimal.h"

#if WITH_EDITOR

#include "EffectsNiagaraAuthoring.h"
#include "Engine/StaticMesh.h"
#include "GlossaryKit.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "NiagaraEmitter.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraRibbonRendererProperties.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraSystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogGlossaryNS, Log, All);

namespace
{
	const FString Folder = TEXT("/Game/Glossary/05_Effects/");
	using FB = FGlossaryNiagaraBuilder;

	UMaterialInterface* Mat(const TCHAR* Name)
	{
		return LoadObject<UMaterialInterface>(nullptr, *(Folder + Name));
	}

	/** 이미터의 첫 Sprite 렌더러에 머티리얼 지정 */
	UNiagaraSpriteRendererProperties* SpriteMaterial(FB& B, const FGuid& E, const TCHAR* Material)
	{
		UNiagaraSpriteRendererProperties* Sprite = B.FirstRenderer<UNiagaraSpriteRendererProperties>(E);
		if (Sprite)
		{
			Sprite->Material = Mat(Material);
		}
		return Sprite;
	}

	// ------------------------------------------------------------ 0501 모닥불: 이미터 3개
	void BuildCampfire()
	{
		FB B(Folder + TEXT("NS_Campfire"));
		B.AddUserFloat(TEXT("FlameRate"), 60.f);
		B.AddUserFloat(TEXT("EmberRate"), 15.f);
		B.AddUserFloat(TEXT("SmokeRate"), 8.f);

		// Flames: 짧게 살며 위로 오르는 주황 가산 입자
		const FGuid Flames = B.AddEmitter(TEXT("Fountain"), TEXT("Flames"));
		B.LinkInput(Flames, TEXT("SpawnRate"), TEXT("SpawnRate"), FNiagaraTypeDefinition::GetFloatDef(), TEXT("User.FlameRate"));
		B.SetColor(Flames, TEXT("InitializeParticle"), TEXT("Color"), FLinearColor(6.f, 1.6f, 0.3f, 1.f));
		B.SetFloat(Flames, TEXT("InitializeParticle"), TEXT("Lifetime Min"), 0.5f);
		B.SetFloat(Flames, TEXT("InitializeParticle"), TEXT("Lifetime Max"), 0.9f);
		B.SetFloat(Flames, TEXT("InitializeParticle"), TEXT("Uniform Sprite Size Min"), 40.f);
		B.SetFloat(Flames, TEXT("InitializeParticle"), TEXT("Uniform Sprite Size Max"), 70.f);
		B.SetVec3(Flames, TEXT("AddVelocity"), TEXT("Velocity"), FVector3f(0.f, 0.f, 160.f));
		B.SetVec3(Flames, TEXT("GravityForce"), TEXT("Gravity"), FVector3f(0.f, 0.f, 60.f));
		SpriteMaterial(B, Flames, TEXT("M_FX_Additive"));

		const FGuid Embers = B.AddEmitter(TEXT("Fountain"), TEXT("Embers"));
		B.LinkInput(Embers, TEXT("SpawnRate"), TEXT("SpawnRate"), FNiagaraTypeDefinition::GetFloatDef(), TEXT("User.EmberRate"));
		B.SetColor(Embers, TEXT("InitializeParticle"), TEXT("Color"), FLinearColor(10.f, 3.f, 0.4f, 1.f));
		B.SetFloat(Embers, TEXT("InitializeParticle"), TEXT("Lifetime Min"), 1.5f);
		B.SetFloat(Embers, TEXT("InitializeParticle"), TEXT("Lifetime Max"), 2.5f);
		B.SetFloat(Embers, TEXT("InitializeParticle"), TEXT("Uniform Sprite Size Min"), 4.f);
		B.SetFloat(Embers, TEXT("InitializeParticle"), TEXT("Uniform Sprite Size Max"), 7.f);
		B.SetVec3(Embers, TEXT("AddVelocity"), TEXT("Velocity"), FVector3f(0.f, 0.f, 250.f));
		B.SetVec3(Embers, TEXT("GravityForce"), TEXT("Gravity"), FVector3f(0.f, 0.f, 20.f));
		SpriteMaterial(B, Embers, TEXT("M_FX_Additive"));

		const FGuid Smoke = B.AddEmitter(TEXT("Fountain"), TEXT("Smoke"));
		B.LinkInput(Smoke, TEXT("SpawnRate"), TEXT("SpawnRate"), FNiagaraTypeDefinition::GetFloatDef(), TEXT("User.SmokeRate"));
		B.SetColor(Smoke, TEXT("InitializeParticle"), TEXT("Color"), FLinearColor(0.08f, 0.08f, 0.08f, 0.6f));
		B.SetFloat(Smoke, TEXT("InitializeParticle"), TEXT("Lifetime Min"), 3.f);
		B.SetFloat(Smoke, TEXT("InitializeParticle"), TEXT("Lifetime Max"), 4.f);
		B.SetFloat(Smoke, TEXT("InitializeParticle"), TEXT("Uniform Sprite Size Min"), 80.f);
		B.SetFloat(Smoke, TEXT("InitializeParticle"), TEXT("Uniform Sprite Size Max"), 140.f);
		B.SetVec3(Smoke, TEXT("AddVelocity"), TEXT("Velocity"), FVector3f(0.f, 0.f, 90.f));
		B.SetVec3(Smoke, TEXT("GravityForce"), TEXT("Gravity"), FVector3f(0.f, 0.f, 10.f));
		SpriteMaterial(B, Smoke, TEXT("M_FX_Smoke"));
		B.Finish();
	}

	struct FSystemEntry
	{
		const TCHAR* Name;
		void (*Build)();
	};
}

void BuildNiagaraSystems()
{
	const FSystemEntry Systems[] = {
		{ TEXT("NS_Campfire"), &BuildCampfire },
	};
	FString Only;
	FParse::Value(FCommandLine::Get(), TEXT("NSOnly="), Only);
	for (const FSystemEntry& Entry : Systems)
	{
		if (!Only.IsEmpty() && !Only.Contains(Entry.Name))
		{
			continue;
		}
		UE_LOG(LogGlossaryNS, Display, TEXT("[Glossary] Niagara 빌드: %s"), Entry.Name);
		Entry.Build();
	}
}

#endif // WITH_EDITOR
