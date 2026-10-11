#include "EffectsNiagaraAuthoring.h"

#if WITH_EDITOR

#include "GlossaryAssetKit.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "NiagaraEditorUtilities.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraScript.h"
#include "NiagaraSettings.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemFactoryNew.h"
#include "UObject/StructOnScope.h"
#include "ViewModels/NiagaraEmitterHandleViewModel.h"
#include "ViewModels/NiagaraEmitterViewModel.h"
#include "ViewModels/NiagaraSystemViewModel.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"
#include "ViewModels/Stack/NiagaraStackFunctionInput.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "ViewModels/Stack/NiagaraStackModuleItem.h"
#include "ViewModels/Stack/NiagaraStackViewModel.h"

DEFINE_LOG_CATEGORY_STATIC(LogGlossaryNiagara, Log, All);

namespace
{
	/** 이름 비교: 공백 제거 + 대소문자 무시 ("Lifetime Min" == "LifetimeMin") */
	FString Normalize(const FString& In)
	{
		return In.Replace(TEXT(" "), TEXT("")).ToLower();
	}

	/** "SpawnRate@Update" → 이름 + 단계(없으면 아무 단계) */
	void SplitModuleName(const FString& Module, FString& OutName, FString& OutStage)
	{
		if (!Module.Split(TEXT("@"), &OutName, &OutStage))
		{
			OutName = Module;
			OutStage.Reset();
		}
	}

	bool StageMatches(const FString& Stage, ENiagaraScriptUsage Usage)
	{
		if (Stage.IsEmpty()) return true;
		if (Stage == TEXT("Spawn"))         return Usage == ENiagaraScriptUsage::ParticleSpawnScript || Usage == ENiagaraScriptUsage::ParticleSpawnScriptInterpolated;
		if (Stage == TEXT("Update"))        return Usage == ENiagaraScriptUsage::ParticleUpdateScript;
		if (Stage == TEXT("EmitterSpawn"))  return Usage == ENiagaraScriptUsage::EmitterSpawnScript;
		if (Stage == TEXT("EmitterUpdate")) return Usage == ENiagaraScriptUsage::EmitterUpdateScript;
		if (Stage == TEXT("Event"))         return Usage == ENiagaraScriptUsage::ParticleEventScript;
		return false;
	}

	const TCHAR* UsageName(ENiagaraScriptUsage Usage)
	{
		switch (Usage)
		{
		case ENiagaraScriptUsage::ParticleSpawnScript:
		case ENiagaraScriptUsage::ParticleSpawnScriptInterpolated: return TEXT("Spawn");
		case ENiagaraScriptUsage::ParticleUpdateScript: return TEXT("Update");
		case ENiagaraScriptUsage::EmitterSpawnScript: return TEXT("EmitterSpawn");
		case ENiagaraScriptUsage::EmitterUpdateScript: return TEXT("EmitterUpdate");
		case ENiagaraScriptUsage::ParticleEventScript: return TEXT("Event");
		default: return TEXT("Other");
		}
	}
}

FGlossaryNiagaraBuilder::FGlossaryNiagaraBuilder(const FString& PackagePath)
	: Path(PackagePath)
{
	UPackage* Package = GlossaryAssetKit::PreparePackage(PackagePath);
	const FString Name = FPackageName::GetShortName(PackagePath);

	// 이전 실행의 시스템은 치워 두고 새로 만든다 (빌더는 매번 같은 결과)
	if (UObject* Existing = StaticFindObject(UObject::StaticClass(), Package, *Name))
	{
		Existing->Rename(*MakeUniqueObjectName(GetTransientPackage(), Existing->GetClass(), *(Name + TEXT("_Old"))).ToString(),
			GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
		Existing->ClearFlags(RF_Public | RF_Standalone);
	}

	System = NewObject<UNiagaraSystem>(Package, *Name, RF_Public | RF_Standalone | RF_Transactional);
	UNiagaraSystemFactoryNew::InitializeSystem(System, true);   // System Spawn/Update 기본 노드
	if (UNiagaraEffectType* EffectType = GetDefault<UNiagaraSettings>()->GetDefaultEffectType())
	{
		System->SetEffectType(EffectType);
	}
}

FGlossaryNiagaraBuilder::~FGlossaryNiagaraBuilder()
{
	InvalidateViewModel();
}

void FGlossaryNiagaraBuilder::Fail(const FString& Message)
{
	++Errors;
	UE_LOG(LogGlossaryNiagara, Error, TEXT("[Glossary] %s: %s"), *FPackageName::GetShortName(Path), *Message);
}

void FGlossaryNiagaraBuilder::InvalidateViewModel()
{
	if (ViewModel.IsValid())
	{
		ViewModel.Reset();   // 소멸자가 Cleanup 을 부른다
	}
}

FNiagaraSystemViewModel& FGlossaryNiagaraBuilder::GetViewModel()
{
	if (!ViewModel.IsValid())
	{
		// UI 없이 데이터 편집만 하는 헤드리스 뷰모델 (자동 컴파일·시뮬레이션 끔)
		FNiagaraSystemViewModelOptions Options;
		Options.bCanAutoCompile = false;
		Options.bCanSimulate = false;
		Options.bCanModifyEmittersFromTimeline = false;
		Options.bCompileForEdit = false;
		Options.bIsForDataProcessingOnly = true;
		Options.EditMode = ENiagaraSystemViewModelEditMode::SystemAsset;
		// 메시지 매니저 구독 키가 비어 있으면 assert → 새 시스템은 에셋 GUID 가 없을 수 있어 임시 GUID 사용
		Options.MessageLogGuid = System->GetAssetGuid().IsValid() ? System->GetAssetGuid() : FGuid::NewGuid();
		ViewModel = MakeShared<FNiagaraSystemViewModel>();
		ViewModel->Initialize(*System, Options);
	}
	return *ViewModel;
}

FGuid FGlossaryNiagaraBuilder::AddEmitter(const FString& Template, FName Name)
{
	InvalidateViewModel();
	const FString TemplatePath = Template.StartsWith(TEXT("/")) ? Template : TEXT("/Niagara/DefaultAssets/Templates/Emitters/") + Template;
	UNiagaraEmitter* Source = LoadObject<UNiagaraEmitter>(nullptr, *TemplatePath);
	if (!Source)
	{
		Fail(FString::Printf(TEXT("템플릿 이미터 없음 %s"), *TemplatePath));
		return FGuid();
	}
	const FGuid Id = FNiagaraEditorUtilities::AddEmitterToSystem(*System, *Source, Source->GetExposedVersion().VersionGuid);
	for (FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
	{
		if (Handle.GetId() == Id)
		{
			Handle.SetName(Name, *System);
		}
	}
	return Id;
}

FVersionedNiagaraEmitterData* FGlossaryNiagaraBuilder::GetEmitterData(const FGuid& Emitter) const
{
	for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
	{
		if (Handle.GetId() == Emitter)
		{
			return Handle.GetEmitterData();
		}
	}
	return nullptr;
}

UNiagaraStackModuleItem* FGlossaryNiagaraBuilder::FindModule(const FGuid& Emitter, const FString& Module)
{
	FString Name, Stage;
	SplitModuleName(Module, Name, Stage);
	TSharedPtr<FNiagaraEmitterHandleViewModel> HandleVM = GetViewModel().GetEmitterHandleViewModelById(Emitter);
	if (!HandleVM.IsValid())
	{
		Fail(TEXT("이미터 핸들 없음"));
		return nullptr;
	}
	UNiagaraStackEntry* Root = HandleVM->GetEmitterStackViewModel()->GetRootEntry();
	Root->RefreshChildren();
	TArray<UNiagaraStackModuleItem*> Modules;
	Root->GetUnfilteredChildrenOfType<UNiagaraStackModuleItem>(Modules, true);
	const FString Wanted = Normalize(Name);
	for (UNiagaraStackModuleItem* Item : Modules)
	{
		const UNiagaraNodeOutput* Output = Item->GetOutputNode();
		if (!Output || !StageMatches(Stage, Output->GetUsage()))
		{
			continue;
		}
		if (Normalize(Item->GetModuleNode().GetFunctionName()) == Wanted || Normalize(Item->GetDisplayName().ToString()) == Wanted)
		{
			return Item;
		}
	}
	return nullptr;
}

bool FGlossaryNiagaraBuilder::HasModule(const FGuid& Emitter, const FString& Module)
{
	return FindModule(Emitter, Module) != nullptr;
}

UNiagaraStackFunctionInput* FGlossaryNiagaraBuilder::FindInput(const FGuid& Emitter, const FString& Module, const FString& Input)
{
	UNiagaraStackModuleItem* Item = FindModule(Emitter, Module);
	if (!Item)
	{
		Fail(FString::Printf(TEXT("모듈 없음 %s"), *Module));
		return nullptr;
	}
	TArray<UNiagaraStackFunctionInput*> Inputs;
	Item->GetUnfilteredChildrenOfType<UNiagaraStackFunctionInput>(Inputs, true);
	const FString Wanted = Normalize(Input);
	for (UNiagaraStackFunctionInput* In : Inputs)
	{
		// 모듈의 최상위 입력만 (동적 입력 안쪽 입력 제외)
		if (&In->GetInputFunctionCallNode() != &Item->GetModuleNode())
		{
			continue;
		}
		if (Normalize(In->GetInputParameterHandle().GetName().ToString()) == Wanted || Normalize(In->GetDisplayName().ToString()) == Wanted)
		{
			return In;
		}
	}
	Fail(FString::Printf(TEXT("입력 없음 %s.%s"), *Module, *Input));
	return nullptr;
}

bool FGlossaryNiagaraBuilder::SetRaw(const FGuid& Emitter, const FString& Module, const FString& Input, const void* Data, int32 Size)
{
	UNiagaraStackFunctionInput* In = FindInput(Emitter, Module, Input);
	if (!In)
	{
		return false;
	}
	const FNiagaraTypeDefinition& Type = In->GetInputType();
	const UScriptStruct* Struct = Type.GetScriptStruct();
	if (!Struct || Type.GetSize() != Size)
	{
		Fail(FString::Printf(TEXT("타입 불일치 %s.%s (%s, %d != %d)"), *Module, *Input, *Type.GetName(), Type.GetSize(), Size));
		return false;
	}
	TSharedRef<FStructOnScope> Value = MakeShared<FStructOnScope>(Struct);
	FMemory::Memcpy(Value->GetStructMemory(), Data, Size);
	In->SetLocalValue(Value);
	return true;
}

bool FGlossaryNiagaraBuilder::SetFloat(const FGuid& E, const FString& M, const FString& I, float V)              { return SetRaw(E, M, I, &V, sizeof(V)); }
bool FGlossaryNiagaraBuilder::SetInt(const FGuid& E, const FString& M, const FString& I, int32 V)                { return SetRaw(E, M, I, &V, sizeof(V)); }
bool FGlossaryNiagaraBuilder::SetVec2(const FGuid& E, const FString& M, const FString& I, const FVector2f& V)    { return SetRaw(E, M, I, &V, sizeof(V)); }
bool FGlossaryNiagaraBuilder::SetVec3(const FGuid& E, const FString& M, const FString& I, const FVector3f& V)    { return SetRaw(E, M, I, &V, sizeof(V)); }
bool FGlossaryNiagaraBuilder::SetColor(const FGuid& E, const FString& M, const FString& I, const FLinearColor& V) { return SetRaw(E, M, I, &V, sizeof(V)); }
bool FGlossaryNiagaraBuilder::SetEnum(const FGuid& E, const FString& M, const FString& I, int32 V)               { return SetRaw(E, M, I, &V, sizeof(V)); }

bool FGlossaryNiagaraBuilder::SetBool(const FGuid& E, const FString& M, const FString& I, bool bValue)
{
	FNiagaraBool Value;
	Value.SetValue(bValue);
	return SetRaw(E, M, I, &Value, sizeof(Value));
}

bool FGlossaryNiagaraBuilder::SetEnumByName(const FGuid& Emitter, const FString& Module, const FString& Input, const FString& EntryName)
{
	UNiagaraStackFunctionInput* In = FindInput(Emitter, Module, Input);
	if (!In)
	{
		return false;
	}
	const UEnum* Enum = In->GetInputType().GetEnum();
	if (!Enum)
	{
		Fail(FString::Printf(TEXT("열거형 아님 %s.%s"), *Module, *Input));
		return false;
	}
	const FString Wanted = Normalize(EntryName);
	for (int32 i = 0; i < Enum->NumEnums(); ++i)
	{
		if (Normalize(Enum->GetDisplayNameTextByIndex(i).ToString()) == Wanted || Normalize(Enum->GetNameStringByIndex(i)) == Wanted)
		{
			const int32 Value = (int32)Enum->GetValueByIndex(i);
			return SetEnum(Emitter, Module, Input, Value);
		}
	}
	Fail(FString::Printf(TEXT("열거 항목 없음 %s.%s = %s"), *Module, *Input, *EntryName));
	return false;
}

bool FGlossaryNiagaraBuilder::SetObject(const FGuid& Emitter, const FString& Module, const FString& Input, UObject* Value)
{
	UNiagaraStackFunctionInput* In = FindInput(Emitter, Module, Input);
	if (!In)
	{
		return false;
	}
	In->SetObjectAssetValue(Value);
	return true;
}

bool FGlossaryNiagaraBuilder::LinkInput(const FGuid& Emitter, const FString& Module, const FString& Input, const FNiagaraTypeDefinition& Type, FName Parameter)
{
	UNiagaraStackFunctionInput* In = FindInput(Emitter, Module, Input);
	if (!In)
	{
		return false;
	}
	In->SetLinkedParameterValue(FNiagaraVariableBase(Type, Parameter));
	return true;
}

bool FGlossaryNiagaraBuilder::ResetInput(const FGuid& Emitter, const FString& Module, const FString& Input)
{
	UNiagaraStackFunctionInput* In = FindInput(Emitter, Module, Input);
	if (!In)
	{
		return false;
	}
	if (In->CanReset())
	{
		In->Reset();
	}
	return true;
}

bool FGlossaryNiagaraBuilder::SetModuleEnabled(const FGuid& Emitter, const FString& Module, bool bEnabled)
{
	UNiagaraStackModuleItem* Item = FindModule(Emitter, Module);
	if (!Item)
	{
		Fail(FString::Printf(TEXT("모듈 없음 %s"), *Module));
		return false;
	}
	Item->SetEnabled(bEnabled);
	return true;
}

bool FGlossaryNiagaraBuilder::AddModule(const FGuid& Emitter, ENiagaraScriptUsage Usage, const FString& ScriptPath, const FGuid& EventUsageId)
{
	FVersionedNiagaraEmitterData* Data = GetEmitterData(Emitter);
	UNiagaraScript* Module = LoadObject<UNiagaraScript>(nullptr, *ScriptPath);
	if (!Data || !Module)
	{
		Fail(FString::Printf(TEXT("모듈 추가 실패 %s"), *ScriptPath));
		return false;
	}
	UNiagaraScript* Target = nullptr;
	switch (Usage)
	{
	case ENiagaraScriptUsage::ParticleSpawnScript:  Target = Data->SpawnScriptProps.Script; break;
	case ENiagaraScriptUsage::ParticleUpdateScript: Target = Data->UpdateScriptProps.Script; break;
	case ENiagaraScriptUsage::EmitterSpawnScript:   Target = Data->EmitterSpawnScriptProps.Script; break;
	case ENiagaraScriptUsage::EmitterUpdateScript:  Target = Data->EmitterUpdateScriptProps.Script; break;
	case ENiagaraScriptUsage::ParticleEventScript:
		if (FNiagaraEventScriptProperties* Event = Data->GetEventHandlerByIdUnsafe(EventUsageId))
		{
			Target = Event->Script;
		}
		break;
	default: break;
	}
	UNiagaraNodeOutput* OutputNode = Target ? FNiagaraEditorUtilities::GetScriptOutputNode(*Target) : nullptr;
	if (!OutputNode)
	{
		Fail(FString::Printf(TEXT("출력 노드 없음 %s"), *ScriptPath));
		return false;
	}
	InvalidateViewModel();
	return FNiagaraStackGraphUtilities::AddScriptModuleToStack(Module, *OutputNode) != nullptr;
}

void FGlossaryNiagaraBuilder::AddRenderer(const FGuid& Emitter, UNiagaraRendererProperties* Renderer)
{
	for (FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
	{
		if (Handle.GetId() == Emitter)
		{
			const FVersionedNiagaraEmitter Instance = Handle.GetInstance();
			Instance.Emitter->AddRenderer(Renderer, Instance.Version);
		}
	}
	InvalidateViewModel();
}

UNiagaraRendererProperties* FGlossaryNiagaraBuilder::FirstRendererOfClass(const FGuid& Emitter, UClass* Class) const
{
	if (FVersionedNiagaraEmitterData* Data = GetEmitterData(Emitter))
	{
		for (UNiagaraRendererProperties* Renderer : Data->GetRenderers())
		{
			if (Renderer && Renderer->IsA(Class))
			{
				return Renderer;
			}
		}
	}
	return nullptr;
}

void FGlossaryNiagaraBuilder::RemoveAllRenderers(const FGuid& Emitter)
{
	for (FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
	{
		if (Handle.GetId() == Emitter)
		{
			const FVersionedNiagaraEmitter Instance = Handle.GetInstance();
			TArray<UNiagaraRendererProperties*> Renderers = Instance.GetEmitterData()->GetRenderers();
			for (UNiagaraRendererProperties* Renderer : Renderers)
			{
				Instance.Emitter->RemoveRenderer(Renderer, Instance.Version);
			}
		}
	}
	InvalidateViewModel();
}

FGuid FGlossaryNiagaraBuilder::AddEventHandler(const FGuid& Emitter, const FGuid& SourceEmitter, FName EventName, int32 SpawnNumber)
{
	TSharedPtr<FNiagaraEmitterHandleViewModel> HandleVM = GetViewModel().GetEmitterHandleViewModelById(Emitter);
	if (!HandleVM.IsValid())
	{
		Fail(TEXT("이벤트 핸들러: 이미터 없음"));
		return FGuid();
	}
	// 이벤트 핸들러 = 다른 이미터가 보낸 이벤트(DeathEvent 등)를 받아 실행되는 스크립트 스택
	FNiagaraEventScriptProperties Props;
	Props.SourceEmitterID = SourceEmitter;
	Props.SourceEventName = EventName;
	Props.ExecutionMode = EScriptExecutionMode::SpawnedParticles;   // 이벤트마다 SpawnNumber 개를 새로 스폰
	Props.SpawnNumber = SpawnNumber;
	Props.MaxEventsPerFrame = 0;
	HandleVM->GetEmitterViewModel()->AddEventHandler(Props, true);
	const FGuid UsageId = Props.Script ? Props.Script->GetUsageId() : FGuid();
	InvalidateViewModel();
	return UsageId;
}

void FGlossaryNiagaraBuilder::AddUserFloat(FName Name, float Default)
{
	const FNiagaraVariable Var(FNiagaraTypeDefinition::GetFloatDef(), FName(TEXT("User.") + Name.ToString()));
	System->GetExposedParameters().AddParameter(Var, true);
	System->GetExposedParameters().SetParameterValue(Default, Var);
}

void FGlossaryNiagaraBuilder::AddUserInt(FName Name, int32 Default)
{
	const FNiagaraVariable Var(FNiagaraTypeDefinition::GetIntDef(), FName(TEXT("User.") + Name.ToString()));
	System->GetExposedParameters().AddParameter(Var, true);
	System->GetExposedParameters().SetParameterValue(Default, Var);
}

void FGlossaryNiagaraBuilder::AddUserColor(FName Name, const FLinearColor& Default)
{
	const FNiagaraVariable Var(FNiagaraTypeDefinition::GetColorDef(), FName(TEXT("User.") + Name.ToString()));
	System->GetExposedParameters().AddParameter(Var, true);
	System->GetExposedParameters().SetParameterValue(Default, Var);
}

void FGlossaryNiagaraBuilder::AddUserVec3(FName Name, const FVector3f& Default)
{
	const FNiagaraVariable Var(FNiagaraTypeDefinition::GetVec3Def(), FName(TEXT("User.") + Name.ToString()));
	System->GetExposedParameters().AddParameter(Var, true);
	System->GetExposedParameters().SetParameterValue(Default, Var);
}

void FGlossaryNiagaraBuilder::AddUserPosition(FName Name, const FVector3f& Default)
{
	const FNiagaraVariable Var(FNiagaraTypeDefinition::GetPositionDef(), FName(TEXT("User.") + Name.ToString()));
	System->GetExposedParameters().AddParameter(Var, true);
	System->GetExposedParameters().SetParameterValue(FNiagaraPosition(Default), Var);
}

void FGlossaryNiagaraBuilder::DumpStack(const FGuid& Emitter)
{
	TSharedPtr<FNiagaraEmitterHandleViewModel> HandleVM = GetViewModel().GetEmitterHandleViewModelById(Emitter);
	if (!HandleVM.IsValid())
	{
		return;
	}
	UNiagaraStackEntry* Root = HandleVM->GetEmitterStackViewModel()->GetRootEntry();
	Root->RefreshChildren();
	TArray<UNiagaraStackModuleItem*> Modules;
	Root->GetUnfilteredChildrenOfType<UNiagaraStackModuleItem>(Modules, true);
	for (UNiagaraStackModuleItem* Item : Modules)
	{
		const UNiagaraNodeOutput* Output = Item->GetOutputNode();
		UE_LOG(LogGlossaryNiagara, Display, TEXT("[NSDump] %s @%s  '%s'"), *HandleVM->GetName().ToString(),
			Output ? UsageName(Output->GetUsage()) : TEXT("?"), *Item->GetModuleNode().GetFunctionName());
		TArray<UNiagaraStackFunctionInput*> Inputs;
		Item->GetUnfilteredChildrenOfType<UNiagaraStackFunctionInput>(Inputs, true);
		for (UNiagaraStackFunctionInput* In : Inputs)
		{
			if (&In->GetInputFunctionCallNode() != &Item->GetModuleNode())
			{
				continue;
			}
			UE_LOG(LogGlossaryNiagara, Display, TEXT("[NSDump]      . %s : %s (mode %d)"),
				*In->GetInputParameterHandle().GetName().ToString(), *In->GetInputType().GetName(), (int32)In->GetValueMode());
		}
	}
	if (FVersionedNiagaraEmitterData* Data = GetEmitterData(Emitter))
	{
		for (UNiagaraRendererProperties* Renderer : Data->GetRenderers())
		{
			UE_LOG(LogGlossaryNiagara, Display, TEXT("[NSDump]   renderer %s"), Renderer ? *Renderer->GetClass()->GetName() : TEXT("null"));
		}
	}
}

bool FGlossaryNiagaraBuilder::Finish()
{
	InvalidateViewModel();
	// 이미터 속성(SimTarget 등)을 바꿨으므로 강제 컴파일 후 끝날 때까지 기다린다(GPU 셰이더 포함)
	System->RequestCompile(true);
	System->WaitForCompilationComplete(true, false);

	TArray<UNiagaraScript*> Scripts;
	Scripts.Add(System->GetSystemSpawnScript());
	Scripts.Add(System->GetSystemUpdateScript());
	for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
	{
		if (FVersionedNiagaraEmitterData* Data = Handle.GetEmitterData())
		{
			Data->GetScripts(Scripts, true);
		}
	}
	for (UNiagaraScript* Script : Scripts)
	{
		if (!Script)
		{
			continue;
		}
		for (const FNiagaraCompileEvent& Event : Script->GetVMExecutableData().LastCompileEvents)
		{
			if (Event.Severity == FNiagaraCompileEventSeverity::Error)
			{
				Fail(FString::Printf(TEXT("컴파일 오류 (%s): %s"), *Script->GetName(), *Event.Message));
			}
		}
	}
	const bool bSaved = GlossaryAssetKit::SaveAsset(System);
	UE_LOG(LogGlossaryNiagara, Display, TEXT("[Glossary] Niagara 시스템 저장 %s (오류 %d, 이미터 %d, Ready %d)"),
		*Path, Errors, System->GetEmitterHandles().Num(), System->IsReadyToRun() ? 1 : 0);
	return bSaved && Errors == 0;
}

#endif // WITH_EDITOR
