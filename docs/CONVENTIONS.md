# 예제 레벨 작성 규칙

용어집 항목 하나 = 예제 레벨 하나 = 데모 액터 클래스 하나. 레벨은 손으로 만들지 않고 **C++ 빌더**로 생성한다
(`Tools > Glossary > Build All Levels` 또는 `tools/glossary-build.sh`). 이 문서는 데모를 추가하거나 고칠 때 지킬 규칙이다.

## 환경

- Unreal Engine 5.8, C++ (Visual Studio 2026 / MSVC 14.5x), Windows
- 렌더링: Lumen GI·반사, Virtual Shadow Maps, Nanite 켜짐. 정적 조명도 허용(`r.AllowStaticLighting=True`, 1005 데모용)
- 입력: **Enhanced Input**. 캐릭터 조작은 템플릿 에셋 `/Game/Input/IA_Move · IA_Look · IA_MouseLook · IA_Jump · IMC_Default · IMC_MouseLook`
  데모 토글 키(1~5, C 등)는 `AGlossaryDemoBase::WasKeyPressed(EKeys::One)` 로 바로 읽는다.
- 캐릭터: `AGlossaryCharacter` = Manny(`/Game/Characters/Mannequins`) + `ABP_Unarmed` + Spring Arm 카메라.
  애니메이션 재료: `/Game/Characters/Mannequins/Anims/Unarmed/{Attack,Jog,Jump,Walk}`, `Anims/Death`, `Rigs/CR_Mannequin_*`, `Rigs/PA_Mannequin`
- 켜진 플러그인: EnhancedInput, Niagara, StateTree·GameplayStateTree, SmartObjects, GameplayAbilities, PoseSearch, MotionWarping,
  ModelViewViewModel, CommonUI, ChaosVehicles, GeometryCollection·FieldSystem·Fracture, PCG, Metasound, Mass(Gameplay·AI·Crowd), ZoneGraph,
  PythonScriptPlugin, EditorScriptingUtilities
- 콜리전 채널: `ECC_GameTraceChannel1` = Projectile(오브젝트), `ECC_GameTraceChannel2` = Interact(트레이스)
- 게임플레이 태그(DefaultGame.ini): `State.Stunned/Burning/Poisoned/Invincible`, `Ability.Fireball`, `Ability.Cooldown.Fireball`, `Damage.Fire/Ice`
- 한글: UMG 기본 폰트(Roboto + DroidSansFallback)가 한글을 그린다. **TextRender 컴포넌트는 한글이 안 나오므로** 3D 라벨은 `GlossaryKit::SpawnLabel` 사용

## 폴더와 이름

| 대상 | 위치 · 이름 |
| --- | --- |
| 데모 클래스 | `Source/Glossary{분야}/Public/Demo{id}_{Name}.h` + `Private/Demo{id}_{Name}.cpp`, 클래스 `ADemo{id}_{Name}` |
| 보조 클래스 | 같은 모듈 `Public/{id}/` 또는 파일 이름 앞에 id (예: `D0703_BTTask_Attack.h`) |
| 레벨 | `/Game/Levels/{NN_분야}/L_{id}_{Name}` (빌더가 생성, 카탈로그가 경로 결정) |
| 생성 에셋 | `/Game/Glossary/{NN_분야}/` 접두사: M_ 머티리얼, MI_ 인스턴스, MF_ 함수, T_ 텍스처, S_ 사운드, NS_ Niagara, BT_/BB_ 등 |
| 에셋 빌더 | 분야 모듈 `Private/Glossary{분야}Module.cpp` 의 `Build...Assets()` 또는 별도 `Private/{분야}Assets.cpp` |

| 분야 | 모듈 | 폴더 |
| --- | --- | --- |
| 01 | GlossaryMovement | 01_Movement |
| 02 | GlossaryPhysics | 02_Physics |
| 03 | GlossaryAnimation | 03_Animation |
| 04 | GlossaryMaterial | 04_Material |
| 05 | GlossaryEffects | 05_Effects |
| 06 | GlossaryUI | 06_UI |
| 07 | GlossaryAI | 07_AI |
| 08 | GlossarySystems | 08_Systems |
| 09 | GlossaryAudio | 09_Audio |
| 10 | GlossaryRendering | 10_Rendering |
| 11 | GlossaryPatterns | 11_Patterns |

전체 목록(id·이름·용어·쉽게 말하면·요청 예문)은 `Source/GlossaryCore/Private/GlossaryCatalogData.inl`.
클래스 이름은 반드시 `Demo{id}_{Name}` 이어야 빌더가 찾는다.

## 데모 클래스 형태

```cpp
UCLASS()
class GLOSSARYMOVEMENT_API ADemo0105_CoyoteTime : public AGlossaryDemoBase
{
	GENERATED_BODY()
public:
	ADemo0105_CoyoteTime();
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Coyote Time", meta = (ClampMin = 0))
	float CoyoteTime = 0.1f;   // 용어 이름이 코드에 그대로 보이게
};

ADemo0105_CoyoteTime::ADemo0105_CoyoteTime()
{
	DemoId = TEXT("0105");
	Controls = TEXT("WASD 이동 · Space 점프\nC : 코요테 타임 ON/OFF\n관찰: 발판 끝에서 떨어진 뒤 0.1초 동안 점프가 된다");
	Showcase = TEXT("발판 끝에서 떨어진 뒤 0.1초 점프 허용, ON/OFF 비교");
	// 필요하면: PlayerPawnClass, PlayerStartTransform, Environment.Sky, bNeedsNavMesh, bShowMouseCursor,
	//           Implementation = EGlossaryImplementation::Alternative; AlternativeNote = TEXT("...");
}
```

- `DemoId`, `Controls`(2~4줄: 조작 + **무엇을 관찰할지**), `Showcase`(위키 '씬에서 보여 주는 것' 한 줄)는 필수.
- 장면은 **BeginPlay** 에서 `GlossaryKit::SpawnShape / SpawnBlock / AddShape / SpawnLabel` 로 만든다(레벨 파일에 저장할 필요 없음).
  레벨에 저장돼야 하는 것(볼륨, 정적 라이트, 스트리밍 서브레벨 등)만 `#if WITH_EDITOR virtual void BuildLevel(UWorld*) override` 에서 놓는다.
- 실시간 값은 `SetStatus(TEXT("..."))` 로 정보 패널 '상태' 줄에 표시(매 Tick 갱신 가능).
- 화면 왼쪽 위 **620×440 영역은 정보 패널 자리**. 데모 UI 는 오른쪽이나 아래에 둔다.
- 비교 데모는 켜기/끄기 토글(대개 1/2 또는 C 키)로 차이를 보여 준다. Unity 판과 같은 방식.
- 기본 레벨 구성: 격자 바닥 60m×60m(z=0), 해·하늘·안개, PlayerStart(-600,0,100). 바꾸려면 `Environment`, `PlayerStartTransform`.
- 플레이어 폰이 필요 없으면 `PlayerPawnClass = nullptr`(고정 카메라) 또는 직접 만든 폰 클래스. 카메라는 `ACameraActor` 를 스폰해 `SetViewTarget`.
- 데모 간 공유: 다른 데모의 클래스를 재사용하는 것은 괜찮지만, 각 레벨은 독립적이어야 한다.

## 에셋을 코드로 만들기 (GlossaryAssetKit, 에디터 전용)

언리얼 에셋(머티리얼·텍스처·사운드·커브·데이터 에셋)은 실행 중에 만들 수 없는 것이 많다. Unity 판처럼 **빌더 코드가 에셋 파일을 생성**하고,
데모는 실행 중 `GlossaryKit::LoadAsset<UMaterialInterface>(TEXT("/Game/Glossary/04_Material/M_Dissolve"))` 로 불러 쓴다.

```cpp
#if WITH_EDITOR
#include "GlossaryAssetKit.h"
static void BuildGlossaryMaterialAssets()
{
	using namespace GlossaryAssetKit;
	UMaterial* M = CreateMaterial(TEXT("/Game/Glossary/04_Material/M_Dissolve"));
	M->BlendMode = BLEND_Masked;
	auto* Amount = AddScalarParam(M, TEXT("DissolveAmount"), 0.f);
	auto* Noise  = AddTextureParam(M, TEXT("Noise"), CreateNoiseTexture(TEXT("/Game/Glossary/04_Material/T_Noise")));
	auto* Code   = AddCustom(M, TEXT("return Noise - Amount;"), {TEXT("Noise"), TEXT("Amount")}, CMOT_Float1);
	Connect(Noise, TEXT("R"), Code, TEXT("Noise"));
	Connect(Amount, TEXT(""), Code, TEXT("Amount"));
	ConnectToPin(Code, MP_OpacityMask);
	FinishMaterial(M);   // 컴파일 + 저장, 오류는 로그
}
#endif
// StartupModule: GlossaryAssetKit::RegisterBuilder(TEXT("04_Material"), &BuildGlossaryMaterialAssets);
```

- 빌더 이름은 분야 폴더 이름(`04_Material`). 여러 개면 `04_Material_Decal` 처럼 접두사를 유지(필터 빌드가 접두사로 찾음).
- 머티리얼 노드는 `AddExpression<UMaterialExpressionXxx>`, 복잡한 수식은 **Custom 노드(HLSL)** 로 짧게.
- 사운드는 `MakeTone / MakeNoise / MakeMelody` → `CreateSoundWave`. 텍스처는 `CreateTexture(픽셀 함수)` / `CreateNoiseTexture`.
- 커브·데이터 에셋 등은 `CreateOrLoad<UCurveFloat>(Path)` 로 만들고 값을 채운 뒤 `SaveAsset`.
- 빌더는 여러 번 실행해도 같은 결과(덮어쓰기)여야 한다.
- 실행 중 만들 수 있는 것(Dynamic Material Instance, 런타임 Behavior Tree 노드, UInputAction NewObject 등)은 굳이 에셋으로 만들지 않아도 된다.

## 위젯(UMG)

Widget Blueprint 는 코드로 만들 수 없으므로 `UGlossaryWidgetBase` 를 상속해 `BuildTree()` 에서 `WidgetTree->ConstructWidget` 으로 만든다.
헬퍼: `MakeRootCanvas / AddToCanvas / MakeText / MakeBorder / MakeButton`. 패널 데모는 정보 패널과 겹치지 않게 오른쪽/아래 앵커를 쓴다.

## 대체 구현

코드로 만들 수 없는 에셋(Animation Blueprint 그래프, Niagara 모듈 스택, Behavior Tree 에디터 그래프, Control Rig, Sequencer 키 등)은

1. 실행 중 동등한 구조를 C++ 로 만들거나(예: Behavior Tree 노드를 런타임에 조립, UAnimInstance C++ 서브클래스),
2. 같은 개념을 직접 구현하고 정보 패널에 "실제 프로젝트에서는 에디터에서 이렇게 만든다"를 적는다.

이 경우 `Implementation = EGlossaryImplementation::Alternative; AlternativeNote = TEXT("무엇을 무엇으로 대체");`

## 코드 스타일

- 주석은 한국어, "왜/무엇을 보여 주는지" 위주로 간결하게. 학습용이므로 핵심 줄에는 설명 주석.
- 용어 이름이 코드에 그대로 보이게 짓는다(`CoyoteTime`, `JumpBufferTime`, `WalkableFloorAngle`).
- 튜닝 값은 `UPROPERTY(EditAnywhere, Category = "...")` 로 노출(디테일 패널에서 조절 가능).
- UE 5.8 API: `TObjectPtr<>` 멤버, `GetWorld()->SpawnActor`, Enhanced Input. 구 입력(`BindAxis`, `InputComponent->BindKey`) 금지.
- 엔진 헤더는 필요한 것만 include (IWYU).

## 빌드와 검증

```bash
tools/build.sh                     # 에디터 타깃 C++ 빌드
tools/build.sh GlossaryEffects     # 모듈 하나만
tools/glossary-build.sh -filter=05 # 05 분야 에셋 + 레벨 생성 (에디터를 닫고 실행)
```
