# Niagara 시스템을 C++ 에디터 코드로 만들기 (UE 5.8 조사 메모)

결론: 에디터 전용 코드로 템플릿 이미터에서 Niagara System 을 만들고, 모듈 입력값·렌더러·모듈 추가·User 파라미터까지 설정해 저장할 수 있다.

## 만들기
- `UNiagaraSystemFactoryNew` 는 export 안 됨 → `NewObject<UNiagaraSystem>(Pkg, Name, RF_Public|RF_Standalone|RF_Transactional)` + `UNiagaraSystemFactoryNew::InitializeSystem(Sys, true)` (이 static 함수는 export 됨)
- `FNiagaraEditorUtilities::AddEmitterToSystem(*Sys, *Emitter, Emitter->GetExposedVersion().VersionGuid)` → 핸들 Id
- `Sys->SetEffectType(GetDefault<UNiagaraSettings>()->GetDefaultEffectType())`
- `NiagaraEditor/Internal/AssetBuildUtilities/NiagaraAssetBuilder.h` 는 프로젝트 모듈에서 include 불가 → 로직을 복사
- 템플릿: `/Niagara/DefaultAssets/Templates/Emitters/{Fountain, Minimal, SimpleSpriteBurst, OmnidirectionalBurst, DirectionalBurst, DynamicBeam, StaticBeam, LocationBasedRibbon, UpwardMeshBurst, HangingParticulates, BlowingParticles, ConfettiBurst, RecycleParticlesInView, SingleLoopingParticle}.X`
  시스템: `/Niagara/DefaultAssets/Templates/Systems/{SimpleExplosion, RadialBurst, DirectionalBurst, AttributeReaderTrails}` (*Lightweight 은 Stateless 라 모듈 스택 없음)
  BehaviorExamples: `/Niagara/DefaultAssets/Templates/BehaviorExamples/SubUVAnimation` 등
- 머티리얼: `/Niagara/DefaultAssets/DefaultSpriteMaterial`, `/Niagara/DefaultAssets/DefaultRIbbonMaterial`(대문자 I 가 실제 이름)

## 모듈 입력값 설정
- `SystemAdapter` API 는 읽기 전용(사실상 스텁) → 쓰지 말 것
- **스택 뷰모델** 사용(export 됨):
  ```cpp
  FNiagaraSystemViewModelOptions O; O.bCanAutoCompile=false; O.bCanSimulate=false; O.bCanModifyEmittersFromTimeline=false;
  O.bCompileForEdit=false; O.bIsForDataProcessingOnly=true; O.EditMode=ENiagaraSystemViewModelEditMode::SystemAsset;
  TSharedRef<FNiagaraSystemViewModel> VM = MakeShared<FNiagaraSystemViewModel>(); VM->Initialize(*Sys, O);
  UNiagaraStackViewModel* Stack = VM->GetEmitterHandleViewModelById(Id)->GetEmitterStackViewModel();
  // Stack->GetRootEntry()->GetUnfilteredChildrenOfType<UNiagaraStackModuleItem>(Modules, true)
  // Module->GetModuleNode().GetFunctionName() == "SpawnRate" / "InitializeParticle" / "AddVelocity" ...
  // Module->GetOutputNode()->GetUsage() 로 단계 구분 (UNiagaraScript::IsEquivalentUsage)
  // UNiagaraStackFunctionInput: GetInputParameterHandle().GetName() == "SpawnRate" / "Color" / "Lifetime Min"
  //   (최상위 입력만: &In->GetInputFunctionCallNode() == &Module->GetModuleNode())
  //   SetLocalValue(TSharedRef<FStructOnScope>(InputType.GetStruct()) + memcpy) — 타입: FNiagaraFloat, FLinearColor, FVector3f, FNiagaraBool, FNiagaraInt32, enum
  //   SetLinkedParameterValue(FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(), "User.Color"))
  VM.Reset(); Sys->RequestCompile(true); Sys->WaitForCompilationComplete(true, false); // 저장
  ```
- Static switch(Color Mode, Lifetime Mode 등)는 먼저 SetLocalValue 로 설정한 뒤 RefreshChildren 해야 종속 입력이 생김
- 저수준 대안: `Script->RapidIterationParameters.SetParameterValue(v, FNiagaraUtilities::ConvertVariableToRapidIterationConstantName(FNiagaraVariable(Float, "SpawnRate.SpawnRate"), *Handle.GetUniqueInstanceName(), Usage), true)` 를 시스템 Spawn/Update + 해당 usage 의 모든 이미터 스크립트(GPU 포함)에 기록. 로컬 값만 가능
- Fountain 의 실제 RI 이름: `SpawnRate.SpawnRate`, `InitializeParticle.Color`, `InitializeParticle.Lifetime Min/Max`, `InitializeParticle.Uniform Sprite Size Min/Max`, `AddVelocity.Velocity`, `ScaleColor.Scale RGBA`, `GravityForce.Gravity`, `Drag.Drag`, `ShapeLocation.Sphere Radius`, `EmitterState.Loop Duration`

## 이미터 속성·렌더러·모듈 추가
- `FVersionedNiagaraEmitterData* ED = Handle.GetEmitterData()`: `SimTarget = ENiagaraSimTarget::GPUComputeSim`, `CalculateBoundsMode = Fixed`, `FixedBounds`, `bLocalSpace`, `GetRenderers()`, `GetScripts()`. StaticStruct 는 링크 안 됨 → 바꾼 뒤 시스템 강제 컴파일
- 렌더러: `NewObject<UNiagaraSpriteRendererProperties>(Emitter)` → `Emitter->AddRenderer(R, VersionGuid)`. Sprite: Material, SubImageSize. Ribbon: Material. Mesh: Meshes[i].Mesh
- 모듈 추가: `FNiagaraStackGraphUtilities::AddScriptModuleToStack(LoadObject<UNiagaraScript>(…), *FNiagaraEditorUtilities::GetScriptOutputNode(*ED->UpdateScriptProps.Script))` 후 RefreshChildren
- 모듈 경로: `/Niagara/Modules/Emitter/{SpawnRate,SpawnBurst_Instantaneous,SpawnPerUnit,EmitterState}`, `/Niagara/Modules/Spawn/Initialization/V2/InitializeParticle`, `/Niagara/Modules/Spawn/Location/V2/ShapeLocation`, `/Niagara/Modules/Spawn/Velocity/{AddVelocity,AddVelocityInCone,AddVelocityFromPoint}`, `/Niagara/Modules/Update/Forces/V2/{CurlNoiseForce,PointAttractionForce}`, `/Niagara/Modules/Update/Forces/{GravityForce,Drag,VortexForce}`, `/Niagara/Modules/Update/Velocity/VortexVelocity`, `/Niagara/Modules/Collision/Collision`, `/Niagara/Modules/Update/SubUV/V2/SubUVAnimation`, `/Niagara/Modules/Update/Color/ScaleColor`, `/Niagara/Modules/Update/Size/ScaleSpriteSize`, `/Niagara/Modules/Events/{GenerateDeathEvent,ReceiveDeathEvent,GenerateLocationEvent,ReceiveLocationEvent}`, `/Niagara/Modules/Beams/{BeamEmitterSetup,SpawnBeam,UpdateBeam}`, `/Niagara/Modules/Ribbons/RibbonWidth`
- 이벤트 핸들러: `FNiagaraEmitterViewModel::AddEventHandler(Props, true)` (SourceEventName="DeathEvent", ExecutionMode=SpawnedParticles). 이벤트는 CPU 전용

## User 파라미터
- `Sys->GetExposedParameters().AddParameter(V); SetParameterValue(Value, V);` (V 이름은 "User.Color")
- 런타임: `NiagaraComponent->SetVariableLinearColor(FName("Color"), …)` — "User." 접두사 없이

## Build.cs (에디터 전용)
`UnrealEd, Niagara, NiagaraCore, NiagaraEditor, NiagaraShader, AssetRegistry, AssetTools, DeveloperSettings, MovieScene, ContentBrowser`
(런타임 모듈에서는 `if (Target.bBuildEditor)` 안에 넣는다)

## 위험
- 커맨드렛 -nullrhi 에서는 GPU 셰이더 컴파일이 멈추거나 건너뛸 수 있음 → RHI 있는 커맨드렛 또는 에디터 메뉴에서 실행
- 스택 뷰모델은 에디터 UI API 라 버전마다 바뀔 수 있음
- 동적 입력(커브, Random Range)이 걸린 입력은 RI 경로로는 무시됨 → SetLocalValue 사용
- 대안(최후): 마스터 시스템 몇 개에 User 파라미터를 노출하고 StaticDuplicateObject 로 복제 + User 기본값만 변경
