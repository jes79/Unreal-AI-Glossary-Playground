#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR

#include "NiagaraTypes.h"
#include "NiagaraCommon.h"

class UNiagaraSystem;
class UNiagaraScript;
class UNiagaraRendererProperties;
class UNiagaraStackModuleItem;
class UNiagaraStackFunctionInput;
class FNiagaraSystemViewModel;
struct FVersionedNiagaraEmitterData;
struct FNiagaraEventScriptProperties;

/**
 * 에디터 전용: Niagara System 에셋을 C++ 로 조립한다 (docs/dev-notes/niagara-authoring.md 레시피).
 *  1) 새 시스템 + 템플릿 이미터(Fountain 등) 추가
 *  2) 헤드리스 System ViewModel 의 스택에서 모듈 입력값을 바꾸거나 User 파라미터에 연결
 *  3) 모듈·렌더러·이벤트 핸들러 추가, 이미터 속성(GPU Sim·Fixed Bounds) 변경
 *  4) 컴파일 완료까지 기다린 뒤 저장
 * 모듈 이름은 스택에 보이는 함수 이름(예: "SpawnRate", "InitializeParticle"), 입력 이름은 공백·대소문자 무시("Lifetime Min").
 */
class FGlossaryNiagaraBuilder
{
public:
	/** PackagePath = "/Game/Glossary/05_Effects/NS_Campfire". 같은 이름이 있으면 새로 만든다(덮어쓰기). */
	explicit FGlossaryNiagaraBuilder(const FString& PackagePath);
	~FGlossaryNiagaraBuilder();

	UNiagaraSystem* GetSystem() const { return System; }

	/** 템플릿 이미터 추가. Template 은 "Fountain" 처럼 이름만 주면 /Niagara/DefaultAssets/Templates/Emitters/ 에서 찾는다 */
	FGuid AddEmitter(const FString& Template, FName Name);
	FVersionedNiagaraEmitterData* GetEmitterData(const FGuid& Emitter) const;

	// ---------- 모듈 입력값 (로컬 값) ----------
	bool SetFloat(const FGuid& Emitter, const FString& Module, const FString& Input, float Value);
	bool SetInt(const FGuid& Emitter, const FString& Module, const FString& Input, int32 Value);
	bool SetBool(const FGuid& Emitter, const FString& Module, const FString& Input, bool bValue);
	bool SetVec2(const FGuid& Emitter, const FString& Module, const FString& Input, const FVector2f& Value);
	bool SetVec3(const FGuid& Emitter, const FString& Module, const FString& Input, const FVector3f& Value);
	bool SetColor(const FGuid& Emitter, const FString& Module, const FString& Input, const FLinearColor& Value);
	/** 정적 스위치·열거형 입력(Lifetime Mode, Shape Primitive …) — 값은 열거형 정수 */
	bool SetEnum(const FGuid& Emitter, const FString& Module, const FString& Input, int32 Value);
	/** 열거형 입력을 표시 이름으로 지정 ("Sphere", "Random") */
	bool SetEnumByName(const FGuid& Emitter, const FString& Module, const FString& Input, const FString& EntryName);
	/** 오브젝트 입력 (메시 등) */
	bool SetObject(const FGuid& Emitter, const FString& Module, const FString& Input, UObject* Value);
	/** 모듈 입력을 파라미터에 연결 ("User.Color", "Emitter.xxx" 등) */
	bool LinkInput(const FGuid& Emitter, const FString& Module, const FString& Input, const FNiagaraTypeDefinition& Type, FName Parameter);
	/** 입력을 기본값으로 되돌린다 (동적 입력 → 로컬 값 등) */
	bool ResetInput(const FGuid& Emitter, const FString& Module, const FString& Input);

	/** 모듈 켜기/끄기 */
	bool SetModuleEnabled(const FGuid& Emitter, const FString& Module, bool bEnabled);
	bool HasModule(const FGuid& Emitter, const FString& Module);

	/** 모듈 추가: Usage = ParticleSpawnScript / ParticleUpdateScript / EmitterUpdateScript …, 이벤트 스택은 EventUsageId 지정 */
	bool AddModule(const FGuid& Emitter, ENiagaraScriptUsage Usage, const FString& ScriptPath, const FGuid& EventUsageId = FGuid(), int32 TargetIndex = INDEX_NONE);

	/** 렌더러 추가 (NewObject 해서 넘긴다) */
	void AddRenderer(const FGuid& Emitter, UNiagaraRendererProperties* Renderer);
	template <typename T> T* FirstRenderer(const FGuid& Emitter) const;
	UNiagaraRendererProperties* FirstRendererOfClass(const FGuid& Emitter, UClass* Class) const;
	void RemoveAllRenderers(const FGuid& Emitter);

	/** 이벤트 핸들러 추가 (반환: 이벤트 스크립트 UsageId) */
	FGuid AddEventHandler(const FGuid& Emitter, const FGuid& SourceEmitter, FName EventName, int32 SpawnNumber);

	// ---------- User 파라미터 ----------
	void AddUserFloat(FName Name, float Default);
	void AddUserInt(FName Name, int32 Default);
	void AddUserColor(FName Name, const FLinearColor& Default);
	void AddUserVec3(FName Name, const FVector3f& Default);
	void AddUserPosition(FName Name, const FVector3f& Default);

	/** 디버그: 이미터 스택의 모듈과 입력 이름·타입을 로그로 출력 */
	void DumpStack(const FGuid& Emitter);

	/** 컴파일 대기 + 저장. 실패하면 false */
	bool Finish();

	/** 스택 뷰모델을 버린다(구조 변경 뒤 다시 만들도록) */
	void InvalidateViewModel();

	int32 GetErrorCount() const { return Errors; }

private:
	FNiagaraSystemViewModel& GetViewModel();
	UNiagaraStackModuleItem* FindModule(const FGuid& Emitter, const FString& Module);
	UNiagaraStackFunctionInput* FindInput(const FGuid& Emitter, const FString& Module, const FString& Input);
	bool SetRaw(const FGuid& Emitter, const FString& Module, const FString& Input, const void* Data, int32 Size);
	void Fail(const FString& Message);

	FString Path;
	UNiagaraSystem* System = nullptr;
	TSharedPtr<FNiagaraSystemViewModel> ViewModel;
	int32 Errors = 0;
};

template <typename T>
T* FGlossaryNiagaraBuilder::FirstRenderer(const FGuid& Emitter) const
{
	return Cast<T>(FirstRendererOfClass(Emitter, T::StaticClass()));
}

#endif // WITH_EDITOR
