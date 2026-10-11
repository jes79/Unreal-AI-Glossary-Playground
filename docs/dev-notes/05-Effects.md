# 05 이펙트 (GlossaryEffects) 작업 노트

Niagara System 은 전부 **에디터 C++ 빌더**(`Private/EffectsNiagaraSystems.cpp` + `EffectsNiagaraAuthoring.*`)가
엔진 템플릿 이미터(Fountain, OmnidirectionalBurst, DirectionalBurst, SimpleSpriteBurst, UpwardMeshBurst, DynamicBeam)에서 조립한다.
모듈 입력은 헤드리스 `FNiagaraSystemViewModel` 스택(`UNiagaraStackFunctionInput::SetLocalValue / SetLinkedParameterValue`)으로 바꾸고,
모듈 추가는 `FNiagaraStackGraphUtilities::AddScriptModuleToStack`, 이벤트 핸들러는 `FNiagaraEmitterViewModel::AddEventHandler`.
`tools/glossary-build.sh -filter=05` 한 번에 23개 NS_ 가 오류 0 으로 컴파일·저장된다(RHI 있는 커맨드렛, GPU 셰이더 대기 멈춤 없음).

## id 별 표

| id | 구현 방식 | 씬에서 보여 주는 것 | 검증 |
| --- | --- | --- | --- |
| 0501 | 런타임 데모 | 불꽃·불씨·연기 3개 이미터로 된 Niagara 모닥불(NS_Campfire) + 깜빡이는 Point Light, 1/2/3 으로 이미터별 ON/OFF | shot 확인 |
| 0502 | 런타임 데모 | Spawn Rate 50 연기(계속) vs Spawn Burst Instantaneous 100 파편(한꺼번에), 2.5초마다 재폭발 | shot 확인 |
| 0503 | 런타임 데모 | 10만 입자 이미터 GPU Compute Sim(Fixed Bounds 노란 상자) ↔ CPU Sim 전환, 프레임 시간 비교 | shot 확인(GPU·CPU 둘 다, 이 PC 에서는 둘 다 약 107 FPS — 차이는 사양에 따라) |
| 0504 | 런타임 데모 | User.Color · User.SpawnRate 를 노출한 분수, 숫자 키로 C++ SetVariableLinearColor/Float | shot 확인 |
| 0505 | 런타임 데모 | 휘두르는 칼의 칼끝에 Ribbon 이미터를 붙인 검기 트레일, ON/OFF | shot 확인 |
| 0506 | 런타임 데모 | 큐브 메시 입자 파편이 회전하며 떨어지는 Mesh Renderer vs 같은 움직임의 Sprite | shot 확인 |
| 0507 | 런타임 데모 | 불똥 분수: Collision 모듈(Restitution 0.5)로 바닥에 튕김 vs 충돌 없음(바닥 통과) | shot 확인 |
| 0508 | 런타임 데모 | Curl Noise Force 로 휘돌며 떠다니는 반딧불 vs 힘 없는 직선 이동 | shot 확인 |
| 0509 | 런타임 데모 | 불꽃놀이: Rocket 이 죽는 위치에서 Generate Death Event → 두 Burst 이미터의 Event Handler 가 스폰 | shot 확인 |
| 0510 | 런타임 데모 | 두 구슬을 잇는 빔(Dynamic Beam 2가닥), 끝점을 User.BeamStart/BeamEnd 로 매 프레임 갱신 | shot 확인 |
| 0511 | 런타임 데모 | 코드로 만든 8x8 폭발 플립북을 SubUV Animation 모듈로 재생, 원본 시트 나란히 | shot 확인 |
| 0512 | 런타임 데모 | 총구(Muzzle) 컴포넌트에 0.05초 화염 Niagara + Point Light 깜빡임, 자동 연사·슬로 모션(T)·화염 계속 보기(H) | shot 확인(H) |
| 0513 | 런타임 데모 | Line Trace 히트 지점에 노멀 방향 불꽃 SpawnSystemAtLocation, 노멀 화살표 표시 | shot 확인 |
| 0514 | 런타임 데모 | 섬광·파편·충격파 링·연기 4 이미터가 Spawn Time 시간차로 터지는 폭발 + 섬광 라이트 + 카메라 쉐이크 | shot 확인 |
| 0515 | 런타임 데모 | 캐릭터 발밑 회전 마법진(가산 머티리얼 판) + Vortex 로 감아 오르는 입자 오라, C 로 ON/OFF | shot 확인 |
| 0516 | 런타임 데모 | 달리는 마네킹의 발 착지 Anim Notify 에서 먼지 Niagara 스폰, 흙/눈 Physical Material 에 따라 색 변경 | shot 확인 |
| 0517 | 런타임 데모 | 카메라 XY 를 따라다니는 GPU 비 이미터(초당 6000) + 바닥에 튀는 물방울(GPU) | shot 확인 |
| 0518 | 런타임 데모 | Vortex Velocity + Point Attraction Force 로 입자가 돌며 중심으로 빨려 드는 포탈 | shot 확인 |
| 0519 | 런타임 데모 | 피격 시 DMI 의 FlashAmount 1→0 (0.15초)로 하얗게 번쩍이는 허수아비, ON/OFF | shot 확인 |
| 0520 | 런타임 데모 | Unbound 전역 Post Process Volume + 동굴 상자 볼륨(어둡고 푸른 톤, Blend Radius 4m) | shot 확인(밖·안) |
| 0521 | 런타임 데모 | 발광 세기 다른 구들에 Bloom 1.5·Threshold -1 표준 ↔ Convolution(FFT) ↔ 끔 | shot 확인 |
| 0522 | 런타임 데모 | 체력 30% 이하에서 Vignette Intensity 0.8, 피해/회복 키 | shot 확인 |
| 0523 | 런타임 데모 | 피격 순간 0.2초 Scene Fringe Intensity 5, C 로 고정 비교 | shot 확인 |
| 0524 | 런타임 데모 | Cinematic DOF Focal Distance = 대화 상대 거리, 배경 흐림, 초점 전환 | shot 확인 |
| 0525 | 런타임 데모 | 달리는 차를 따라가는 카메라, Motion Blur Amount 0.5 로 기둥이 흐름, 옵션처럼 ON/OFF | shot 확인 |
| 0526 | 런타임 데모 | 채도↓·색온도↑ 석양 Color Grading ↔ 같은 톤을 코드로 만든 LUT(T_LUT_Sunset) ↔ 원본 | shot 확인 |
| 0527 | 런타임 데모 | 어두운 동굴 ↔ 밝은 바깥, Auto Exposure(Histogram, Speed Up 0.6/Down 2, EV100 -2~14 고정) 눈 적응 | shot 확인(안·밖) |
| 0528 | 런타임 데모 | 깜빡이는 전구 하나뿐인 복도에 Film Grain Intensity 0.3, ON/OFF | shot 확인 |
| 0529 | 런타임 데모 | Post Process 도메인 머티리얼(흑백+스캔라인)을 Blendables 에 넣어 화면 전체, ON/OFF | shot 확인 |

## 생성 에셋 (/Game/Glossary/05_Effects)

- 머티리얼: M_FX_Additive, M_FX_Smoke, M_FX_Trail, M_FX_Ring(예비), M_FX_Mesh, M_FX_SubUV, M_FX_SheetPreview, M_FX_MagicCircle, M_HitFlash, M_PP_GrayScanline(MD_PostProcess)
- 텍스처: T_FX_Noise, T_FX_Explosion8x8(8x8 플립북), T_LUT_Sunset(256x16 LUT, ColorLookupTable 그룹)
- Niagara System(23): NS_Campfire, NS_SmokeRate, NS_DebrisBurst, NS_GPU100k, NS_CPU100k, NS_UserParam, NS_SwordTrail, NS_MeshDebris, NS_SpriteDebris,
  NS_SparksCollision, NS_SparksNoCollision, NS_FirefliesCurl, NS_FirefliesNoCurl, NS_Fireworks, NS_Beam, NS_Flipbook, NS_MuzzleFlash, NS_HitSpark,
  NS_Explosion, NS_AuraRise, NS_FootDust, NS_Rain, NS_Portal

## 작성 요령·주의

- 헤드리스 System ViewModel 은 `Options.MessageLogGuid` 를 넣지 않으면 `NiagaraMessageManager` assert 로 크래시 → 새 시스템은 임시 GUID.
- 정적 스위치(Velocity Mode, Shape Primitive, Lifetime Mode, Sprite Size Mode …)는 `SetEnumByName` 으로 먼저 바꾸고, 다음 입력을 찾을 때 스택을 RefreshChildren.
- 숨겨진 입력(EValueMode 10 = None)에 값을 넣어도 오류가 나지 않고 무시된다 → 모드 스위치를 먼저 맞출 것. `-NSDump` 로 템플릿 스택 덤프 가능.
- **힘·속도 모듈(Curl Noise, Vortex Velocity, Point Attraction)은 Solve Forces And Velocity 앞에 넣어야 한다** — 뒤에 append 하면 Physics.Force 가 다음 프레임에 지워져 효과가 없다. `AddModule(..., TargetIndex=1)`. (Collision·Generate Death Event 는 뒤에 둬도 됨)
- 빌더 유닛 빌드에서 익명 namespace 상수 이름이 겹치면 재정의 오류 → 파일별 접두사(D0520CaveCenter 등).
- 버스트 데모는 캡처 순간이 버스트 직후가 되도록 자동 타이머 위상을 맞췄다(shot.sh 키는 캡처 1.5초 전에 눌림).
- `Environment.ExposureEV100` 은 노출 보정 = EV100-10 이라 **값이 클수록 밝다**(이름과 반대). 밤 장면은 12 사용.

## 대체 구현

- 0516: Anim Notify 자체는 진짜 UAnimNotify 서브클래스지만, 애니메이션 에셋에 미리 저장하지 않고 실행 중 Jog 시퀀스의 Notifies 배열에 넣고(EndPlay 에서 제거) 발 착지 시각은 발 뼈 높이 샘플링으로 계산한다.
  Physical Surface Type 은 Config 수정 금지라 쓰지 않고 Physical Material 오브젝트 자체로 색을 고른다. (Implementation 은 Runtime 유지 — 동작은 실제 Notify 경로)
- 0517: 바닥 물방울은 GPU 충돌 이벤트(이벤트는 CPU 전용)가 아니라 카메라 주변 바닥 높이에서 별도 Splash 이미터로 생성.
- 0518·0514 충격파: 링 스프라이트 크기 커브 대신 Ring 모양에서 바깥으로 퍼지는 입자 띠로 표현.
- 0514 카메라 쉐이크: 엔진 Wave/Perlin 패턴은 EngineCameras 플러그인 소속이라 UCameraShakePattern 을 직접 상속(UGlossaryDecayShakePattern).

## 남은 문제·공통 변경 요청

- (요청, GlossaryCore) `FGlossaryEnvironmentSettings::ExposureEV100` 은 `AutoExposureBias = EV100-10` 이라 **값이 클수록 밝아진다**. 이름(EV100)과 반대이므로 주석 수정 또는 부호 반전 검토.
- (요청, GlossaryCore) `EGlossarySky::Indoor` 레벨(0528)에 Sky Light 실시간 캡처 경고("…requires a SkyAtmosphere component… Otherwise it will be black")가 화면에 빨간 글씨로 뜬다.
- shot.sh 가 Shot_0520 첫 실행에서 TedsCore assert(Class.cpp:3778)로 시작 중 크래시 한 번 — 다른 모듈 빌드와 겹친 시점, 재실행은 정상.
- 0503: 이 PC 에서는 CPU 10만 개도 107 FPS 로 차이가 작게 보인다(멀티스레드 CPU 성능). 데모 의미는 그대로, 상태 줄에 실측 프레임 시간 표시.
- 커맨드렛 빌드 로그의 `Ready 0` (IsReadyToRun) 은 저장 시점 표시일 뿐이고 실행 시 모두 정상 재생됨.
