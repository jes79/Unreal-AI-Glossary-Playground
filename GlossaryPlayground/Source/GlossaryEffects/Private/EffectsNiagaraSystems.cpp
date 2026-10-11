// 05_Effects 의 Niagara System(NS_*) 을 템플릿 이미터 + 모듈 입력값 설정으로 조립한다.
// 실행: tools/glossary-build.sh -filter=05   (특정 시스템만: -NSOnly=NS_Campfire,NS_Beam)
// 모듈·입력 이름은 -NSDump 로 템플릿을 덤프해 확인한 실제 스택 이름이다.
#include "CoreMinimal.h"

#if WITH_EDITOR

#include "EffectsNiagaraAuthoring.h"
#include "Engine/StaticMesh.h"
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
	const FString NSFolder = TEXT("/Game/Glossary/05_Effects/");
	using FB = FGlossaryNiagaraBuilder;
	const FNiagaraTypeDefinition& FloatDef() { return FNiagaraTypeDefinition::GetFloatDef(); }

	UMaterialInterface* Mat(const TCHAR* Name)
	{
		return LoadObject<UMaterialInterface>(nullptr, *(NSFolder + Name));
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

	/** Initialize Particle: 수명(무작위 범위) · 색 · 균일 스프라이트 크기(무작위 범위) */
	void InitParticle(FB& B, const FGuid& E, const FLinearColor& Color, float LifeMin, float LifeMax, float SizeMin, float SizeMax)
	{
		B.SetEnumByName(E, TEXT("InitializeParticle"), TEXT("Lifetime Mode"), TEXT("Random"));
		B.SetFloat(E, TEXT("InitializeParticle"), TEXT("Lifetime Min"), LifeMin);
		B.SetFloat(E, TEXT("InitializeParticle"), TEXT("Lifetime Max"), LifeMax);
		B.SetColor(E, TEXT("InitializeParticle"), TEXT("Color"), Color);
		B.SetEnumByName(E, TEXT("InitializeParticle"), TEXT("Sprite Size Mode"), TEXT("Random Uniform"));
		B.SetFloat(E, TEXT("InitializeParticle"), TEXT("Uniform Sprite Size Min"), SizeMin);
		B.SetFloat(E, TEXT("InitializeParticle"), TEXT("Uniform Sprite Size Max"), SizeMax);
	}

	/** Add Velocity: 원뿔 방향 (Axis 기준 Angle 도 안, 속도 Speed) */
	void ConeVelocity(FB& B, const FGuid& E, const FVector3f& Axis, float Angle, float Speed)
	{
		B.SetEnumByName(E, TEXT("AddVelocity"), TEXT("Velocity Mode"), TEXT("In Cone"));
		B.SetVec3(E, TEXT("AddVelocity"), TEXT("Cone Axis"), Axis);
		B.SetFloat(E, TEXT("AddVelocity"), TEXT("Cone Angle"), Angle);
		B.SetFloat(E, TEXT("AddVelocity"), TEXT("Velocity Speed"), Speed);
	}

	void LinearVelocity(FB& B, const FGuid& E, const FVector3f& Velocity)
	{
		B.SetEnumByName(E, TEXT("AddVelocity"), TEXT("Velocity Mode"), TEXT("Linear"));
		B.SetVec3(E, TEXT("AddVelocity"), TEXT("Velocity"), Velocity);
	}

	void PointVelocity(FB& B, const FGuid& E, float Speed)
	{
		B.SetEnumByName(E, TEXT("AddVelocity"), TEXT("Velocity Mode"), TEXT("From Point"));
		B.SetFloat(E, TEXT("AddVelocity"), TEXT("Velocity Speed"), Speed);
	}

	void SphereShape(FB& B, const FGuid& E, float Radius)
	{
		B.SetEnumByName(E, TEXT("ShapeLocation"), TEXT("Shape Primitive"), TEXT("Sphere"));
		B.SetFloat(E, TEXT("ShapeLocation"), TEXT("Sphere Radius"), Radius);
	}

	void Burst(FB& B, const FGuid& E, int32 Count, float SpawnTime = 0.f)
	{
		B.SetInt(E, TEXT("SpawnBurst_Instantaneous"), TEXT("Spawn Count"), Count);
		B.SetFloat(E, TEXT("SpawnBurst_Instantaneous"), TEXT("Spawn Time"), SpawnTime);
	}

	void SetGPU(FB& B, const FGuid& E, const FBox& Bounds)
	{
		if (FVersionedNiagaraEmitterData* Data = B.GetEmitterData(E))
		{
			Data->SimTarget = ENiagaraSimTarget::GPUComputeSim;         // GPU Compute Sim
			Data->CalculateBoundsMode = ENiagaraEmitterCalculateBoundMode::Fixed;   // GPU 입자는 CPU 가 위치를 모르므로 고정 경계
			Data->FixedBounds = Bounds;
		}
		B.InvalidateViewModel();
	}

	void SetLocalSpace(FB& B, const FGuid& E, bool bLocal)
	{
		if (FVersionedNiagaraEmitterData* Data = B.GetEmitterData(E))
		{
			Data->bLocalSpace = bLocal;
		}
		B.InvalidateViewModel();
	}

	/** 이미터 반복 방식: Once(한 번) / Infinite(무한) */
	void LoopOnce(FB& B, const FGuid& E)
	{
		B.SetEnumByName(E, TEXT("EmitterState"), TEXT("Loop Behavior"), TEXT("Once"));
	}

	// ============================================================ 0501 모닥불: 이미터 3개
	void BuildCampfire()
	{
		FB B(NSFolder + TEXT("NS_Campfire"));
		B.AddUserFloat(TEXT("FlameRate"), 60.f);
		B.AddUserFloat(TEXT("EmberRate"), 15.f);
		B.AddUserFloat(TEXT("SmokeRate"), 8.f);

		// Flames: 짧게 살며 위로 오르는 주황 가산 입자
		const FGuid Flames = B.AddEmitter(TEXT("Fountain"), TEXT("Flames"));
		B.LinkInput(Flames, TEXT("SpawnRate"), TEXT("SpawnRate"), FloatDef(), TEXT("User.FlameRate"));
		InitParticle(B, Flames, FLinearColor(6.f, 1.6f, 0.3f, 1.f), 0.5f, 0.9f, 40.f, 70.f);
		SphereShape(B, Flames, 30.f);
		ConeVelocity(B, Flames, FVector3f(0, 0, 1), 10.f, 120.f);
		B.SetVec3(Flames, TEXT("GravityForce"), TEXT("Gravity"), FVector3f(0.f, 0.f, 80.f));
		SpriteMaterial(B, Flames, TEXT("M_FX_Additive"));

		// Embers: 작고 밝은 불씨, 높이 튀어 오른다
		const FGuid Embers = B.AddEmitter(TEXT("Fountain"), TEXT("Embers"));
		B.LinkInput(Embers, TEXT("SpawnRate"), TEXT("SpawnRate"), FloatDef(), TEXT("User.EmberRate"));
		InitParticle(B, Embers, FLinearColor(12.f, 3.5f, 0.4f, 1.f), 1.5f, 2.5f, 3.f, 6.f);
		SphereShape(B, Embers, 30.f);
		ConeVelocity(B, Embers, FVector3f(0, 0, 1), 25.f, 220.f);
		B.SetVec3(Embers, TEXT("GravityForce"), TEXT("Gravity"), FVector3f(0.f, 0.f, 30.f));
		SpriteMaterial(B, Embers, TEXT("M_FX_Additive"));

		// Smoke: 크고 느리게 오르는 반투명 연기
		const FGuid Smoke = B.AddEmitter(TEXT("Fountain"), TEXT("Smoke"));
		B.LinkInput(Smoke, TEXT("SpawnRate"), TEXT("SpawnRate"), FloatDef(), TEXT("User.SmokeRate"));
		InitParticle(B, Smoke, FLinearColor(0.12f, 0.12f, 0.12f, 0.7f), 3.f, 4.f, 80.f, 140.f);
		SphereShape(B, Smoke, 30.f);
		ConeVelocity(B, Smoke, FVector3f(0, 0, 1), 15.f, 90.f);
		B.SetVec3(Smoke, TEXT("GravityForce"), TEXT("Gravity"), FVector3f(0.f, 0.f, 15.f));
		SpriteMaterial(B, Smoke, TEXT("M_FX_Smoke"));
		B.Finish();
	}

	// ============================================================ 0502 Spawn Rate / Spawn Burst
	void BuildSmokeRate()
	{
		FB B(NSFolder + TEXT("NS_SmokeRate"));
		B.AddUserFloat(TEXT("SmokeSpawnRate"), 50.f);
		const FGuid E = B.AddEmitter(TEXT("Fountain"), TEXT("Smoke"));
		B.LinkInput(E, TEXT("SpawnRate"), TEXT("SpawnRate"), FloatDef(), TEXT("User.SmokeSpawnRate"));   // Spawn Rate 50
		InitParticle(B, E, FLinearColor(0.35f, 0.35f, 0.38f, 0.6f), 3.f, 4.f, 50.f, 100.f);
		SphereShape(B, E, 30.f);
		ConeVelocity(B, E, FVector3f(0, 0, 1), 15.f, 110.f);
		B.SetVec3(E, TEXT("GravityForce"), TEXT("Gravity"), FVector3f(0.f, 0.f, 10.f));
		SpriteMaterial(B, E, TEXT("M_FX_Smoke"));
		B.Finish();
	}

	void BuildDebrisBurst()
	{
		FB B(NSFolder + TEXT("NS_DebrisBurst"));
		const FGuid E = B.AddEmitter(TEXT("OmnidirectionalBurst"), TEXT("Debris"));
		LoopOnce(B, E);
		Burst(B, E, 100);   // Spawn Burst Instantaneous 100
		InitParticle(B, E, FLinearColor(8.f, 2.5f, 0.3f, 1.f), 1.2f, 2.f, 8.f, 14.f);
		SphereShape(B, E, 10.f);
		PointVelocity(B, E, 700.f);
		B.SetVec3(E, TEXT("GravityForce"), TEXT("Gravity"), FVector3f(0.f, 0.f, -980.f));
		B.SetFloat(E, TEXT("Drag"), TEXT("Drag"), 1.f);
		SpriteMaterial(B, E, TEXT("M_FX_Additive"));
		B.Finish();
	}

	// ============================================================ 0503 GPU 10만 vs CPU 10만
	void BuildHundredK(const TCHAR* Name, bool bGPU)
	{
		FB B(NSFolder + Name);
		const FGuid E = B.AddEmitter(TEXT("Fountain"), TEXT("Swarm"));
		B.SetFloat(E, TEXT("SpawnRate"), TEXT("SpawnRate"), 50000.f);   // 50000/초 × 수명 2초 = 살아 있는 입자 10만 개
		InitParticle(B, E, bGPU ? FLinearColor(0.2f, 1.5f, 3.f, 1.f) : FLinearColor(3.f, 1.f, 0.2f, 1.f), 2.f, 2.f, 3.f, 5.f);
		SphereShape(B, E, 380.f);
		B.SetEnumByName(E, TEXT("ShapeLocation"), TEXT("Sphere Distribution"), TEXT("Random"));
		ConeVelocity(B, E, FVector3f(0, 0, 1), 180.f, 40.f);
		B.SetVec3(E, TEXT("GravityForce"), TEXT("Gravity"), FVector3f(0.f, 0.f, 0.f));
		B.AddModule(E, ENiagaraScriptUsage::ParticleUpdateScript, TEXT("/Niagara/Modules/Update/Forces/V2/CurlNoiseForce.CurlNoiseForce"));
		B.SetFloat(E, TEXT("CurlNoiseForce"), TEXT("Noise Strength"), 300.f);
		SpriteMaterial(B, E, TEXT("M_FX_Additive"));
		if (bGPU)
		{
			SetGPU(B, E, FBox(FVector(-450.f), FVector(450.f)));
		}
		B.Finish();
	}
	void BuildGPU100k() { BuildHundredK(TEXT("NS_GPU100k"), true); }
	void BuildCPU100k() { BuildHundredK(TEXT("NS_CPU100k"), false); }

	// ============================================================ 0504 User Parameter
	void BuildUserParam()
	{
		FB B(NSFolder + TEXT("NS_UserParam"));
		B.AddUserColor(TEXT("Color"), FLinearColor(0.1f, 0.6f, 1.f, 1.f));
		B.AddUserFloat(TEXT("SpawnRate"), 80.f);
		const FGuid E = B.AddEmitter(TEXT("Fountain"), TEXT("Fountain"));
		InitParticle(B, E, FLinearColor::White, 1.5f, 2.f, 10.f, 18.f);
		// 핵심: 모듈 입력을 User 파라미터에 연결 → C++ SetVariable* 로 바꿀 수 있다
		B.LinkInput(E, TEXT("InitializeParticle"), TEXT("Color"), FNiagaraTypeDefinition::GetColorDef(), TEXT("User.Color"));
		B.LinkInput(E, TEXT("SpawnRate"), TEXT("SpawnRate"), FloatDef(), TEXT("User.SpawnRate"));
		ConeVelocity(B, E, FVector3f(0, 0, 1), 15.f, 600.f);
		SpriteMaterial(B, E, TEXT("M_FX_Additive"));
		B.Finish();
	}

	// ============================================================ 0505 Ribbon 트레일
	void BuildSwordTrail()
	{
		FB B(NSFolder + TEXT("NS_SwordTrail"));
		const FGuid E = B.AddEmitter(TEXT("Fountain"), TEXT("Trail"));
		B.SetFloat(E, TEXT("SpawnRate"), TEXT("SpawnRate"), 200.f);   // 칼끝이 지나간 자리마다 입자를 촘촘히 남긴다
		InitParticle(B, E, FLinearColor(0.6f, 1.6f, 4.f, 1.f), 0.3f, 0.3f, 10.f, 10.f);
		B.SetEnumByName(E, TEXT("InitializeParticle"), TEXT("Ribbon Width Mode"), TEXT("Direct Set"));
		B.SetFloat(E, TEXT("InitializeParticle"), TEXT("Ribbon Width"), 45.f);
		SphereShape(B, E, 0.f);
		LinearVelocity(B, E, FVector3f::ZeroVector);
		B.SetVec3(E, TEXT("GravityForce"), TEXT("Gravity"), FVector3f::ZeroVector);
		B.RemoveAllRenderers(E);
		UNiagaraRibbonRendererProperties* Ribbon = NewObject<UNiagaraRibbonRendererProperties>(B.GetSystem());
		Ribbon->Material = Mat(TEXT("M_FX_Trail"));
		B.AddRenderer(E, Ribbon);   // Ribbon Renderer: 입자들을 나이 순서로 이어 띠를 만든다
		B.Finish();
	}

	// ============================================================ 0506 Mesh Renderer
	void BuildDebris(const TCHAR* Name, bool bMesh)
	{
		FB B(NSFolder + Name);
		const FGuid E = B.AddEmitter(TEXT("UpwardMeshBurst"), TEXT("Debris"));
		LoopOnce(B, E);
		Burst(B, E, 40);
		B.SetColor(E, TEXT("InitializeParticle"), TEXT("Color"), FLinearColor(0.8f, 0.45f, 0.2f, 1.f));
		B.SetEnumByName(E, TEXT("InitializeParticle"), TEXT("Lifetime Mode"), TEXT("Random"));
		B.SetFloat(E, TEXT("InitializeParticle"), TEXT("Lifetime Min"), 2.f);
		B.SetFloat(E, TEXT("InitializeParticle"), TEXT("Lifetime Max"), 2.4f);
		if (bMesh)
		{
			B.SetEnumByName(E, TEXT("InitializeParticle"), TEXT("Mesh Scale Mode"), TEXT("Random Uniform"));
			B.SetFloat(E, TEXT("InitializeParticle"), TEXT("Mesh Uniform Scale Min"), 0.12f);
			B.SetFloat(E, TEXT("InitializeParticle"), TEXT("Mesh Uniform Scale Max"), 0.25f);
			if (UNiagaraMeshRendererProperties* MeshRenderer = B.FirstRenderer<UNiagaraMeshRendererProperties>(E))
			{
				// 입자 하나 = 큐브 메시 하나 (회전은 Update Mesh Orientation 모듈이 매 프레임)
				if (MeshRenderer->Meshes.Num() == 0) MeshRenderer->Meshes.AddDefaulted();
				MeshRenderer->Meshes[0].Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
				MeshRenderer->bOverrideMaterials = true;
				MeshRenderer->OverrideMaterials.Reset();
				FNiagaraMeshMaterialOverride Override;
				Override.ExplicitMat = Mat(TEXT("M_FX_Mesh"));
				MeshRenderer->OverrideMaterials.Add(Override);
			}
		}
		else
		{
			B.SetEnumByName(E, TEXT("InitializeParticle"), TEXT("Sprite Size Mode"), TEXT("Random Uniform"));
			B.SetFloat(E, TEXT("InitializeParticle"), TEXT("Uniform Sprite Size Min"), 12.f);
			B.SetFloat(E, TEXT("InitializeParticle"), TEXT("Uniform Sprite Size Max"), 25.f);
			B.RemoveAllRenderers(E);
			UNiagaraSpriteRendererProperties* Sprite = NewObject<UNiagaraSpriteRendererProperties>(B.GetSystem());
			Sprite->Material = Mat(TEXT("M_FX_Mesh"));
			B.AddRenderer(E, Sprite);
		}
		B.Finish();
	}
	void BuildMeshDebris() { BuildDebris(TEXT("NS_MeshDebris"), true); }
	void BuildSpriteDebris() { BuildDebris(TEXT("NS_SpriteDebris"), false); }

	// ============================================================ 0507 Collision
	void BuildSparks(const TCHAR* Name, bool bCollision)
	{
		FB B(NSFolder + Name);
		const FGuid E = B.AddEmitter(TEXT("Fountain"), TEXT("Sparks"));
		B.SetFloat(E, TEXT("SpawnRate"), TEXT("SpawnRate"), 80.f);
		InitParticle(B, E, FLinearColor(10.f, 3.f, 0.3f, 1.f), 3.f, 3.5f, 6.f, 9.f);
		SphereShape(B, E, 5.f);
		ConeVelocity(B, E, FVector3f(0, 0, 1), 35.f, 500.f);
		B.SetVec3(E, TEXT("GravityForce"), TEXT("Gravity"), FVector3f(0.f, 0.f, -980.f));
		B.SetFloat(E, TEXT("Drag"), TEXT("Drag"), 0.f);
		if (bCollision)
		{
			// Collision 모듈: 바닥에 닿으면 Restitution 0.5 로 튕긴다 (CPU 이미터 → 물리 레이 트레이스)
			B.AddModule(E, ENiagaraScriptUsage::ParticleUpdateScript, TEXT("/Niagara/Modules/Collision/Collision.Collision"));
			B.SetFloat(E, TEXT("Collision"), TEXT("Restitution"), 0.5f);
			B.SetFloat(E, TEXT("Collision"), TEXT("Friction"), 0.2f);
		}
		SpriteMaterial(B, E, TEXT("M_FX_Additive"));
		B.Finish();
	}
	void BuildSparksCollision() { BuildSparks(TEXT("NS_SparksCollision"), true); }
	void BuildSparksNoCollision() { BuildSparks(TEXT("NS_SparksNoCollision"), false); }

	// ============================================================ 0508 Curl Noise 반딧불
	void BuildFireflies(const TCHAR* Name, bool bCurl)
	{
		FB B(NSFolder + Name);
		const FGuid E = B.AddEmitter(TEXT("Fountain"), TEXT("Fireflies"));
		B.SetFloat(E, TEXT("SpawnRate"), TEXT("SpawnRate"), 25.f);
		InitParticle(B, E, FLinearColor(4.f, 6.f, 0.6f, 1.f), 5.f, 7.f, 8.f, 12.f);
		SphereShape(B, E, 220.f);
		ConeVelocity(B, E, FVector3f(0, 0, 1), 180.f, 40.f);
		B.SetVec3(E, TEXT("GravityForce"), TEXT("Gravity"), FVector3f::ZeroVector);
		B.SetFloat(E, TEXT("Drag"), TEXT("Drag"), bCurl ? 1.f : 0.f);
		if (bCurl)
		{
			// Curl Noise Force: 3D 노이즈의 curl → 소용돌이 흐름을 따라 떠다닌다
			B.AddModule(E, ENiagaraScriptUsage::ParticleUpdateScript, TEXT("/Niagara/Modules/Update/Forces/V2/CurlNoiseForce.CurlNoiseForce"));
			B.SetFloat(E, TEXT("CurlNoiseForce"), TEXT("Noise Strength"), 250.f);
			B.SetFloat(E, TEXT("CurlNoiseForce"), TEXT("Noise Frequency"), 0.004f);
		}
		SpriteMaterial(B, E, TEXT("M_FX_Additive"));
		B.Finish();
	}
	void BuildFirefliesCurl() { BuildFireflies(TEXT("NS_FirefliesCurl"), true); }
	void BuildFirefliesNoCurl() { BuildFireflies(TEXT("NS_FirefliesNoCurl"), false); }

	// ============================================================ 0509 Death Event 불꽃놀이
	void BuildFireworks()
	{
		FB B(NSFolder + TEXT("NS_Fireworks"));
		// Rocket: 위로 쏘아 올려지고, 죽을 때 Generate Death Event
		const FGuid Rocket = B.AddEmitter(TEXT("Fountain"), TEXT("Rocket"));
		B.SetFloat(Rocket, TEXT("SpawnRate"), TEXT("SpawnRate"), 1.5f);
		InitParticle(B, Rocket, FLinearColor(10.f, 8.f, 4.f, 1.f), 1.2f, 1.6f, 14.f, 14.f);
		SphereShape(B, Rocket, 10.f);
		ConeVelocity(B, Rocket, FVector3f(0, 0, 1), 15.f, 1600.f);
		B.SetVec3(Rocket, TEXT("GravityForce"), TEXT("Gravity"), FVector3f(0.f, 0.f, -400.f));
		B.SetFloat(Rocket, TEXT("Drag"), TEXT("Drag"), 0.f);
		B.AddModule(Rocket, ENiagaraScriptUsage::ParticleUpdateScript, TEXT("/Niagara/Modules/Events/GenerateDeathEvent.GenerateDeathEvent"));
		SpriteMaterial(B, Rocket, TEXT("M_FX_Additive"));
		if (FVersionedNiagaraEmitterData* Data = B.GetEmitterData(Rocket)) { Data->bRequiresPersistentIDs = true; }

		// Burst 이미터 두 개(색 다름): 스스로는 스폰하지 않고 Death Event 를 받을 때만 80개씩
		const FLinearColor BurstColors[2] = { FLinearColor(12.f, 3.f, 6.f, 1.f), FLinearColor(2.f, 8.f, 12.f, 1.f) };
		for (int32 i = 0; i < 2; ++i)
		{
			const FGuid Shell = B.AddEmitter(TEXT("Fountain"), i == 0 ? TEXT("BurstPink") : TEXT("BurstCyan"));
			B.SetModuleEnabled(Shell, TEXT("SpawnRate"), false);
			InitParticle(B, Shell, BurstColors[i], 1.4f, 2.f, 8.f, 12.f);
			SphereShape(B, Shell, 5.f);
			ConeVelocity(B, Shell, FVector3f(0, 0, 1), 180.f, i == 0 ? 600.f : 420.f);
			B.SetVec3(Shell, TEXT("GravityForce"), TEXT("Gravity"), FVector3f(0.f, 0.f, -250.f));
			B.SetFloat(Shell, TEXT("Drag"), TEXT("Drag"), 1.2f);
			SpriteMaterial(B, Shell, TEXT("M_FX_Additive"));
			// Event Handler: Rocket 의 DeathEvent → 이벤트마다 40개 스폰, Receive Death Event 로 위치를 이벤트 위치로
			const FGuid EventId = B.AddEventHandler(Shell, Rocket, TEXT("DeathEvent"), 40);
			B.AddModule(Shell, ENiagaraScriptUsage::ParticleEventScript, TEXT("/Niagara/Modules/Events/ReceiveDeathEvent.ReceiveDeathEvent"), EventId);
		}
		B.Finish();
	}

	// ============================================================ 0510 Beam
	void BuildBeam()
	{
		FB B(NSFolder + TEXT("NS_Beam"));
		B.AddUserVec3(TEXT("BeamStart"), FVector3f(0.f, -400.f, 200.f));
		B.AddUserVec3(TEXT("BeamEnd"), FVector3f(0.f, 400.f, 200.f));
		for (int32 i = 0; i < 2; ++i)
		{
			// 같은 끝점을 쓰는 빔 두 가닥: 굵은 코어 + 가는 번개. Dynamic Beam 은 매 짧은 수명마다 탄젠트가 바뀌어 지글거린다
			const FGuid E = B.AddEmitter(TEXT("DynamicBeam"), i == 0 ? TEXT("Core") : TEXT("Lightning"));
			B.LinkInput(E, TEXT("BeamEmitterSetup"), TEXT("Beam Start"), FNiagaraTypeDefinition::GetVec3Def(), TEXT("User.BeamStart"));
			B.LinkInput(E, TEXT("BeamEmitterSetup"), TEXT("Beam End"), FNiagaraTypeDefinition::GetVec3Def(), TEXT("User.BeamEnd"));
			B.SetBool(E, TEXT("BeamEmitterSetup"), TEXT("Absolute Beam Start"), true);
			B.SetBool(E, TEXT("BeamEmitterSetup"), TEXT("Absolute Beam End"), true);
			B.SetColor(E, TEXT("Color"), TEXT("Color"), i == 0 ? FLinearColor(1.f, 3.f, 8.f, 1.f) : FLinearColor(6.f, 4.f, 12.f, 1.f));
			B.SetFloat(E, TEXT("BeamWidth"), TEXT("Beam Width"), i == 0 ? 18.f : 6.f);
			if (UNiagaraRibbonRendererProperties* Ribbon = B.FirstRenderer<UNiagaraRibbonRendererProperties>(E))
			{
				Ribbon->Material = Mat(TEXT("M_FX_Trail"));
			}
		}
		B.Finish();
	}

	// ============================================================ 0511 SubUV 플립북
	void BuildFlipbook()
	{
		FB B(NSFolder + TEXT("NS_Flipbook"));
		const FGuid E = B.AddEmitter(TEXT("SimpleSpriteBurst"), TEXT("Explosion"));
		LoopOnce(B, E);
		Burst(B, E, 1);
		B.SetEnumByName(E, TEXT("InitializeParticle"), TEXT("Lifetime Mode"), TEXT("Direct Set"));
		B.SetFloat(E, TEXT("InitializeParticle"), TEXT("Lifetime"), 1.2f);
		B.SetColor(E, TEXT("InitializeParticle"), TEXT("Color"), FLinearColor(1.f, 1.f, 1.f, 1.f));
		B.SetEnumByName(E, TEXT("InitializeParticle"), TEXT("Sprite Size Mode"), TEXT("Uniform"));
		B.SetFloat(E, TEXT("InitializeParticle"), TEXT("Uniform Sprite Size"), 380.f);
		B.SetModuleEnabled(E, TEXT("ScaleColor"), false);
		// SubUV Animation 모듈: 수명 동안 SubImageIndex 0 → 63
		B.AddModule(E, ENiagaraScriptUsage::ParticleUpdateScript, TEXT("/Niagara/Modules/Update/SubUV/V2/SubUVAnimation.SubUVAnimation"));
		if (UNiagaraSpriteRendererProperties* Sprite = SpriteMaterial(B, E, TEXT("M_FX_SubUV")))
		{
			Sprite->SubImageSize = FVector2D(8.0, 8.0);   // 8x8 시트
		}
		B.DumpStack(E);
		B.Finish();
	}

	// ============================================================ 0512 Muzzle Flash
	void BuildMuzzleFlash()
	{
		FB B(NSFolder + TEXT("NS_MuzzleFlash"));
		// 앞으로 길게 뻗는 화염 조각 (로컬 +X = 총구 방향)
		const FGuid Flame = B.AddEmitter(TEXT("DirectionalBurst"), TEXT("Flame"));
		LoopOnce(B, Flame);
		SetLocalSpace(B, Flame, true);
		Burst(B, Flame, 10);
		InitParticle(B, Flame, FLinearColor(12.f, 5.f, 1.f, 1.f), 0.05f, 0.05f, 20.f, 45.f);
		ConeVelocity(B, Flame, FVector3f(1, 0, 0), 12.f, 1800.f);
		B.SetVec3(Flame, TEXT("GravityForce"), TEXT("Gravity"), FVector3f::ZeroVector);
		SpriteMaterial(B, Flame, TEXT("M_FX_Additive"));
		// 총구의 둥근 섬광 한 장
		const FGuid Core = B.AddEmitter(TEXT("SimpleSpriteBurst"), TEXT("Core"));
		LoopOnce(B, Core);
		SetLocalSpace(B, Core, true);
		Burst(B, Core, 1);
		B.SetEnumByName(Core, TEXT("InitializeParticle"), TEXT("Lifetime Mode"), TEXT("Direct Set"));
		B.SetFloat(Core, TEXT("InitializeParticle"), TEXT("Lifetime"), 0.05f);
		B.SetColor(Core, TEXT("InitializeParticle"), TEXT("Color"), FLinearColor(16.f, 9.f, 3.f, 1.f));
		B.SetEnumByName(Core, TEXT("InitializeParticle"), TEXT("Sprite Size Mode"), TEXT("Uniform"));
		B.SetFloat(Core, TEXT("InitializeParticle"), TEXT("Uniform Sprite Size"), 70.f);
		SpriteMaterial(B, Core, TEXT("M_FX_Additive"));
		B.Finish();
	}

	// ============================================================ 0513 Hit Spark
	void BuildHitSpark()
	{
		FB B(NSFolder + TEXT("NS_HitSpark"));
		const FGuid E = B.AddEmitter(TEXT("DirectionalBurst"), TEXT("Sparks"));
		LoopOnce(B, E);
		Burst(B, E, 30);
		InitParticle(B, E, FLinearColor(12.f, 5.f, 1.f, 1.f), 0.35f, 0.7f, 3.f, 5.f);
		// 시스템 로컬 +X(= SpawnSystemAtLocation 의 Rotation = 표면 노멀) 방향 원뿔. 좌표계를 Local 로 해서 회전을 따르게
		ConeVelocity(B, E, FVector3f(1, 0, 0), 40.f, 900.f);
		B.SetEnumByName(E, TEXT("AddVelocity"), TEXT("Rotation Coordinate Space"), TEXT("Local"));
		B.SetVec3(E, TEXT("GravityForce"), TEXT("Gravity"), FVector3f(0.f, 0.f, -980.f));
		if (UNiagaraSpriteRendererProperties* Sprite = SpriteMaterial(B, E, TEXT("M_FX_Additive")))
		{
			Sprite->Alignment = ENiagaraSpriteAlignment::VelocityAligned;   // 속도 방향으로 늘어진 불똥
		}
		// 맞은 지점의 짧은 섬광
		const FGuid Flash = B.AddEmitter(TEXT("SimpleSpriteBurst"), TEXT("Flash"));
		LoopOnce(B, Flash);
		Burst(B, Flash, 1);
		B.SetEnumByName(Flash, TEXT("InitializeParticle"), TEXT("Lifetime Mode"), TEXT("Direct Set"));
		B.SetFloat(Flash, TEXT("InitializeParticle"), TEXT("Lifetime"), 0.08f);
		B.SetColor(Flash, TEXT("InitializeParticle"), TEXT("Color"), FLinearColor(10.f, 6.f, 2.f, 1.f));
		B.SetEnumByName(Flash, TEXT("InitializeParticle"), TEXT("Sprite Size Mode"), TEXT("Uniform"));
		B.SetFloat(Flash, TEXT("InitializeParticle"), TEXT("Uniform Sprite Size"), 60.f);
		SpriteMaterial(B, Flash, TEXT("M_FX_Additive"));
		B.DumpStack(E);
		B.Finish();
	}

	// ============================================================ 0514 Explosion (4 레이어, 시간차)
	void BuildExplosion()
	{
		FB B(NSFolder + TEXT("NS_Explosion"));
		// 0초: 섬광
		const FGuid Flash = B.AddEmitter(TEXT("SimpleSpriteBurst"), TEXT("Flash"));
		LoopOnce(B, Flash);
		Burst(B, Flash, 1, 0.f);
		B.SetEnumByName(Flash, TEXT("InitializeParticle"), TEXT("Lifetime Mode"), TEXT("Direct Set"));
		B.SetFloat(Flash, TEXT("InitializeParticle"), TEXT("Lifetime"), 0.2f);
		B.SetColor(Flash, TEXT("InitializeParticle"), TEXT("Color"), FLinearColor(30.f, 14.f, 4.f, 1.f));
		B.SetEnumByName(Flash, TEXT("InitializeParticle"), TEXT("Sprite Size Mode"), TEXT("Uniform"));
		B.SetFloat(Flash, TEXT("InitializeParticle"), TEXT("Uniform Sprite Size"), 700.f);
		SpriteMaterial(B, Flash, TEXT("M_FX_Additive"));
		// 0초: 파편
		const FGuid Debris = B.AddEmitter(TEXT("OmnidirectionalBurst"), TEXT("Debris"));
		LoopOnce(B, Debris);
		Burst(B, Debris, 80, 0.f);
		InitParticle(B, Debris, FLinearColor(10.f, 3.f, 0.4f, 1.f), 1.f, 2.f, 6.f, 12.f);
		SphereShape(B, Debris, 20.f);
		ConeVelocity(B, Debris, FVector3f(0, 0, 1), 80.f, 1300.f);
		B.SetVec3(Debris, TEXT("GravityForce"), TEXT("Gravity"), FVector3f(0.f, 0.f, -980.f));
		B.SetFloat(Debris, TEXT("Drag"), TEXT("Drag"), 0.5f);
		if (UNiagaraSpriteRendererProperties* Sprite = SpriteMaterial(B, Debris, TEXT("M_FX_Additive")))
		{
			Sprite->Alignment = ENiagaraSpriteAlignment::VelocityAligned;
		}
		// 0.05초: 충격파 링 — 바닥 높이 고리에서 바깥으로 퍼지는 입자 띠
		const FGuid Ring = B.AddEmitter(TEXT("OmnidirectionalBurst"), TEXT("Shockwave"));
		LoopOnce(B, Ring);
		Burst(B, Ring, 120, 0.05f);
		InitParticle(B, Ring, FLinearColor(6.f, 4.f, 2.5f, 0.8f), 0.5f, 0.6f, 40.f, 60.f);
		B.SetEnumByName(Ring, TEXT("ShapeLocation"), TEXT("Shape Primitive"), TEXT("Ring / Disc"));
		B.SetFloat(Ring, TEXT("ShapeLocation"), TEXT("Ring Radius"), 40.f);
		B.SetFloat(Ring, TEXT("ShapeLocation"), TEXT("Disc Coverage"), 0.f);
		PointVelocity(B, Ring, 1800.f);
		B.SetVec3(Ring, TEXT("GravityForce"), TEXT("Gravity"), FVector3f::ZeroVector);
		B.SetFloat(Ring, TEXT("Drag"), TEXT("Drag"), 2.f);
		SpriteMaterial(B, Ring, TEXT("M_FX_Additive"));
		// 0.1초: 연기
		const FGuid Smoke = B.AddEmitter(TEXT("OmnidirectionalBurst"), TEXT("Smoke"));
		LoopOnce(B, Smoke);
		Burst(B, Smoke, 30, 0.1f);
		InitParticle(B, Smoke, FLinearColor(0.08f, 0.07f, 0.07f, 0.8f), 2.5f, 3.5f, 180.f, 300.f);
		SphereShape(B, Smoke, 80.f);
		ConeVelocity(B, Smoke, FVector3f(0, 0, 1), 70.f, 350.f);
		B.SetVec3(Smoke, TEXT("GravityForce"), TEXT("Gravity"), FVector3f(0.f, 0.f, 60.f));
		B.SetFloat(Smoke, TEXT("Drag"), TEXT("Drag"), 2.f);
		SpriteMaterial(B, Smoke, TEXT("M_FX_Smoke"));
		B.Finish();
	}

	// ============================================================ 0515 Aura
	void BuildAuraRise()
	{
		FB B(NSFolder + TEXT("NS_AuraRise"));
		const FGuid E = B.AddEmitter(TEXT("Fountain"), TEXT("Rise"));
		SetLocalSpace(B, E, true);   // 캐릭터를 따라 움직이는 오라
		B.SetFloat(E, TEXT("SpawnRate"), TEXT("SpawnRate"), 60.f);
		InitParticle(B, E, FLinearColor(0.4f, 2.f, 6.f, 1.f), 1.4f, 2.f, 6.f, 12.f);
		B.SetEnumByName(E, TEXT("ShapeLocation"), TEXT("Shape Primitive"), TEXT("Ring / Disc"));
		B.SetFloat(E, TEXT("ShapeLocation"), TEXT("Ring Radius"), 70.f);
		B.SetFloat(E, TEXT("ShapeLocation"), TEXT("Disc Coverage"), 0.f);
		LinearVelocity(B, E, FVector3f(0.f, 0.f, 110.f));
		B.SetVec3(E, TEXT("GravityForce"), TEXT("Gravity"), FVector3f(0.f, 0.f, 20.f));
		// 몸 둘레를 돌며 오르도록 소용돌이
		B.AddModule(E, ENiagaraScriptUsage::ParticleUpdateScript, TEXT("/Niagara/Modules/Update/Velocity/VortexVelocity.VortexVelocity"));
		SpriteMaterial(B, E, TEXT("M_FX_Additive"));
		B.Finish();
	}

	// ============================================================ 0516 발자국 먼지
	void BuildFootDust()
	{
		FB B(NSFolder + TEXT("NS_FootDust"));
		B.AddUserColor(TEXT("Color"), FLinearColor(0.45f, 0.28f, 0.12f, 1.f));
		const FGuid E = B.AddEmitter(TEXT("OmnidirectionalBurst"), TEXT("Dust"));
		LoopOnce(B, E);
		Burst(B, E, 12);
		InitParticle(B, E, FLinearColor::White, 0.7f, 1.1f, 25.f, 50.f);
		B.LinkInput(E, TEXT("InitializeParticle"), TEXT("Color"), FNiagaraTypeDefinition::GetColorDef(), TEXT("User.Color"));   // 바닥 재질 색
		SphereShape(B, E, 10.f);
		ConeVelocity(B, E, FVector3f(0, 0, 1), 70.f, 160.f);
		B.SetVec3(E, TEXT("GravityForce"), TEXT("Gravity"), FVector3f(0.f, 0.f, 10.f));
		B.SetFloat(E, TEXT("Drag"), TEXT("Drag"), 3.f);
		SpriteMaterial(B, E, TEXT("M_FX_Smoke"));
		B.Finish();
	}

	// ============================================================ 0517 비 (GPU) + 물방울
	void BuildRain()
	{
		FB B(NSFolder + TEXT("NS_Rain"));
		B.AddUserFloat(TEXT("RainRate"), 6000.f);
		const FGuid Rain = B.AddEmitter(TEXT("Fountain"), TEXT("Rain"));
		B.LinkInput(Rain, TEXT("SpawnRate"), TEXT("SpawnRate"), FloatDef(), TEXT("User.RainRate"));
		InitParticle(B, Rain, FLinearColor(0.5f, 0.6f, 0.8f, 0.5f), 0.75f, 0.75f, 3.f, 3.f);
		B.SetEnumByName(Rain, TEXT("InitializeParticle"), TEXT("Sprite Size Mode"), TEXT("Non-Uniform"));
		B.SetVec2(Rain, TEXT("InitializeParticle"), TEXT("Sprite Size"), FVector2f(2.5f, 70.f));
		// 카메라 주변 40m × 40m, 높이 15m 판에서 생성 → 0.75초 뒤 바닥(z=0)에 닿을 즈음 사라진다
		B.SetEnumByName(Rain, TEXT("ShapeLocation"), TEXT("Shape Primitive"), TEXT("Box / Plane"));
		B.SetVec3(Rain, TEXT("ShapeLocation"), TEXT("Box Size"), FVector3f(4000.f, 4000.f, 10.f));
		B.SetVec3(Rain, TEXT("ShapeLocation"), TEXT("Offset"), FVector3f(0.f, 0.f, 1500.f));
		LinearVelocity(B, Rain, FVector3f(0.f, 0.f, -2000.f));
		B.SetVec3(Rain, TEXT("GravityForce"), TEXT("Gravity"), FVector3f::ZeroVector);
		B.SetFloat(Rain, TEXT("Drag"), TEXT("Drag"), 0.f);
		if (UNiagaraSpriteRendererProperties* Sprite = SpriteMaterial(B, Rain, TEXT("M_FX_Additive")))
		{
			Sprite->Alignment = ENiagaraSpriteAlignment::VelocityAligned;   // 떨어지는 방향으로 긴 빗줄기
		}
		SetGPU(B, Rain, FBox(FVector(-2500.f, -2500.f, -100.f), FVector(2500.f, 2500.f, 1700.f)));

		// 바닥에 튀는 물방울: 같은 영역 바닥 높이에서 짧게 튀어 오르는 작은 입자
		const FGuid Splash = B.AddEmitter(TEXT("Fountain"), TEXT("Splash"));
		B.SetFloat(Splash, TEXT("SpawnRate"), TEXT("SpawnRate"), 1500.f);
		InitParticle(B, Splash, FLinearColor(0.8f, 0.9f, 1.2f, 0.8f), 0.2f, 0.3f, 3.f, 5.f);
		B.SetEnumByName(Splash, TEXT("ShapeLocation"), TEXT("Shape Primitive"), TEXT("Box / Plane"));
		B.SetVec3(Splash, TEXT("ShapeLocation"), TEXT("Box Size"), FVector3f(3000.f, 3000.f, 1.f));
		B.SetVec3(Splash, TEXT("ShapeLocation"), TEXT("Offset"), FVector3f(0.f, 0.f, 3.f));
		ConeVelocity(B, Splash, FVector3f(0, 0, 1), 35.f, 180.f);
		B.SetVec3(Splash, TEXT("GravityForce"), TEXT("Gravity"), FVector3f(0.f, 0.f, -980.f));
		SpriteMaterial(B, Splash, TEXT("M_FX_Additive"));
		SetGPU(B, Splash, FBox(FVector(-2000.f, -2000.f, -50.f), FVector(2000.f, 2000.f, 200.f)));
		B.Finish();
	}

	// ============================================================ 0518 Portal
	void BuildPortal()
	{
		FB B(NSFolder + TEXT("NS_Portal"));
		const FLinearColor Colors[2] = { FLinearColor(3.f, 0.8f, 8.f, 1.f), FLinearColor(0.5f, 3.f, 8.f, 1.f) };
		for (int32 i = 0; i < 2; ++i)
		{
			const FGuid E = B.AddEmitter(TEXT("Fountain"), i == 0 ? TEXT("SwirlPurple") : TEXT("SwirlCyan"));
			SetLocalSpace(B, E, true);
			B.SetFloat(E, TEXT("SpawnRate"), TEXT("SpawnRate"), 300.f);
			InitParticle(B, E, Colors[i], 1.6f, 2.4f, 5.f, 11.f);
			// 로컬 XY 평면의 고리에서 태어난다 (컴포넌트를 세워 고리가 카메라를 향하게)
			B.SetEnumByName(E, TEXT("ShapeLocation"), TEXT("Shape Primitive"), TEXT("Ring / Disc"));
			B.SetFloat(E, TEXT("ShapeLocation"), TEXT("Ring Radius"), i == 0 ? 220.f : 180.f);
			B.SetFloat(E, TEXT("ShapeLocation"), TEXT("Disc Coverage"), 0.f);
			LinearVelocity(B, E, FVector3f::ZeroVector);
			B.SetVec3(E, TEXT("GravityForce"), TEXT("Gravity"), FVector3f::ZeroVector);
			B.SetFloat(E, TEXT("Drag"), TEXT("Drag"), 0.5f);
			// Vortex Velocity: 축(로컬 Z) 둘레를 돈다 + Point Attraction: 중심으로 빨려 든다
			B.AddModule(E, ENiagaraScriptUsage::ParticleUpdateScript, TEXT("/Niagara/Modules/Update/Velocity/VortexVelocity.VortexVelocity"));
			B.AddModule(E, ENiagaraScriptUsage::ParticleUpdateScript, TEXT("/Niagara/Modules/Update/Forces/V2/PointAttractionForce.PointAttractionForce"));
			if (i == 0)
			{
				B.DumpStack(E);
			}
			SpriteMaterial(B, E, TEXT("M_FX_Additive"));
		}
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
		{ TEXT("NS_SmokeRate"), &BuildSmokeRate },
		{ TEXT("NS_DebrisBurst"), &BuildDebrisBurst },
		{ TEXT("NS_GPU100k"), &BuildGPU100k },
		{ TEXT("NS_CPU100k"), &BuildCPU100k },
		{ TEXT("NS_UserParam"), &BuildUserParam },
		{ TEXT("NS_SwordTrail"), &BuildSwordTrail },
		{ TEXT("NS_MeshDebris"), &BuildMeshDebris },
		{ TEXT("NS_SpriteDebris"), &BuildSpriteDebris },
		{ TEXT("NS_SparksCollision"), &BuildSparksCollision },
		{ TEXT("NS_SparksNoCollision"), &BuildSparksNoCollision },
		{ TEXT("NS_FirefliesCurl"), &BuildFirefliesCurl },
		{ TEXT("NS_FirefliesNoCurl"), &BuildFirefliesNoCurl },
		{ TEXT("NS_Fireworks"), &BuildFireworks },
		{ TEXT("NS_Beam"), &BuildBeam },
		{ TEXT("NS_Flipbook"), &BuildFlipbook },
		{ TEXT("NS_MuzzleFlash"), &BuildMuzzleFlash },
		{ TEXT("NS_HitSpark"), &BuildHitSpark },
		{ TEXT("NS_Explosion"), &BuildExplosion },
		{ TEXT("NS_AuraRise"), &BuildAuraRise },
		{ TEXT("NS_FootDust"), &BuildFootDust },
		{ TEXT("NS_Rain"), &BuildRain },
		{ TEXT("NS_Portal"), &BuildPortal },
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
