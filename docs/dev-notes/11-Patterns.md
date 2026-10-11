# 11 코드 구조·블루프린트·에디터 (GlossaryPatterns)

모듈 `GlossaryPlayground/Source/GlossaryPatterns`, 레벨 `/Game/Levels/11_Patterns/L_11xx_*`, 생성 에셋 `/Game/Glossary/11_Patterns/`.
공통 보조: `D11_SidePanel.h` (오른쪽 보조 패널 위젯 + 최근 N줄 로그, 로그 카테고리 `LogGlossaryPatterns`).

## id 별 표

| id | 구현 방식 | 씬에서 보여 주는 것 | 검증 |
| --- | --- | --- | --- |
| 1101 | 런타임 데모 | Speed·Fire 를 노출한 대포, 실행 중 값 조절 + 리플렉션(TFieldIterator)으로 읽은 지정자 플래그·메타데이터 목록 | shot 확인 |
| 1102 | 런타임 데모 | UD1102_HealthComponent 하나를 플레이어(실행 중 추가)·적(생성자)·상자(실행 중 추가)에 붙여 재사용, 주인별 반응은 이벤트로 | shot 확인 |
| 1103 | 런타임 데모 | UINTERFACE Damageable 로 폭발통·허수아비·유리창을 캐스트 없이 Execute_TakeHit 한 줄로, 미구현 벽은 건너뜀 | shot 확인 |
| 1104 | 런타임 데모 | 적 OnDied(DYNAMIC_MULTICAST) 방송에 UI 배너·사운드·점수판이 각자 반응, AddDynamic/RemoveDynamic 비교 | shot 확인 |
| 1105 | 런타임 데모 | 기둥 개수·간격을 바꾸면 RerunConstructionScripts → OnConstruction 이 울타리를 다시 조립(호출 수·컴포넌트 수 표시), 레벨에 저장된 울타리는 에디터에서도 편집 | shot 확인 |
| 1106 | 런타임 데모 | Spawn/Destroy 포탑 vs 총알 풀 50개 포탑, SpawnActor·Destroy 누적 호출 수와 측정한 스폰 비용 비교 | shot 확인 |
| 1107 | 런타임 데모 | TSoftObjectPtr 보스 메시(Quinn)+애니를 RequestAsyncLoad 로 불러와 스폰, 메모리 상태·로드 시간·언로드(GC) 표시 | shot 확인 |
| 1108 | 런타임 데모 | 같은 거리 감지를 Tick·0.2초 Timer·Overlap 이벤트로 8개씩, 초당 검사 횟수 비교 + bCanEverTick=false 장식 | shot 확인 |
| 1109 | 런타임 데모 | TAutoConsoleVariable game.PlayerSpeed 로 이동 속도, Pawn 의 UFUNCTION(Exec) GodMode 무적, FAutoConsoleCommand game.Heal | shot 확인 |
| 1110 | 에디터 도구 | 선택 액터 격자 정렬·일괄 이름 변경 에디터 도구(Tools > Glossary > Patterns 메뉴 + Slate 창 + 콘솔 명령)와 같은 계산의 실행 중 미리보기 | shot 확인 + 에디터(-ExecutePythonScript)에서 메뉴 등록·Glossary.AlignSelected 로 12개 정렬·이름 변경·창 열기 확인 |
| 1111 | 에디터 도구 | Content/Python/glossary_texture_normalmap.py 로 노멀맵 압축 설정 일괄 변경, 레벨은 텍스처 현재 설정을 실행 중에 읽어 전/후 확인 | shot 확인 (실행 후 상태 shots/1111_after.png) + pythonscript 커맨드렛으로 3개 변환 확인 → 빌더로 원상 복구 |
| 1112 | 에디터 도구 | 함수 본문 값(GetLiveTuning)으로 도는 톱니바퀴, Live Coding 상태·패치 횟수, 재시작이 필요한 변경 표 | shot 확인 |
| 1113 | 런타임 데모 | ReplicatedUsing=OnRep_Health 표적 + Server RPC ServerFire + Multicast 섬광, NetMode·Role·RPC/OnRep 호출 수, L 키로 ?listen 전환 | shot 확인 |
| 1114 | 런타임 데모 | 경비병 시야 DrawDebugCone, 순찰 경로 DrawDebugLine, 시선 트레이스, V 키로 Visual Logger(.bvlog) 녹화 + GrabDebugSnapshot | shot 확인 |
| 1115 | 런타임 데모 | 실행 중 조립한 Behavior Tree + AI Perception 으로 순찰하는 적을 ' 키 Gameplay Debugger 로 확인, 커스텀 'Glossary' 카테고리 등록 | shot 확인 |

## 생성 에셋

- `/Game/Glossary/11_Patterns/S_Died` — 1104 사운드 구독자 효과음 (MakeMelody)
- `/Game/Glossary/11_Patterns/Textures/T_Brick_D, T_Brick_N, T_Metal_D, T_Metal_N, T_Rock_Normal, T_Mask_R` — 1111 예제 텍스처. 빌더가 일부러 모두 TC_Default·sRGB 로 만든다(스크립트 실행 전 상태)
- `GlossaryPlayground/Content/Python/glossary_texture_normalmap.py` — 1111 스크립트(에셋 아님, 파일)
- 레벨 저장 액터: 1105 `AD1105_Fence`, 1110 흩어진 상자 12개(태그 `D1110_Messy`, 아웃라이너 폴더 `1110_MessyBoxes`)

## 메모

- 1105: `RerunConstructionScripts()` 는 WITH_EDITOR 전용이라 -game(에디터 바이너리)에서는 동작하고, 패키지 게임 경로는 `RebuildFence()` 를 직접 호출하도록 분기.
- 1109: Exec 함수는 PlayerController·Pawn·HUD·GameMode·CheatManager·GameInstance 에서만 콘솔이 찾는다. 공통 PlayerController 를 바꾸지 않기 위해 Pawn 서브클래스(AD1109_CheatCharacter)에 두었다. 정석은 UCheatManager 서브클래스 + CheatClass.
- 1110: Editor Utility Widget(WBP)은 C++ 로 만들 수 없어 같은 기능을 `D1110_AlignTool.cpp`(WITH_EDITOR)에서 ToolMenus(`LevelEditor.MainMenu.Tools.GlossaryMenu` 확장) + 노매드 탭 `GlossaryAlignTool` + `UEditorActorSubsystem` + `FScopedTransaction` 으로 구현. 콘솔 `Glossary.AlignSelected`, `Glossary.OpenAlignTool`.
- 1113: 단일 -game 은 Standalone(=서버 권한)이라 OnRep 은 수동 호출만 보인다. 2인 PIE(Play As Listen Server)에서 클라이언트 창 OnRep 자동 호출을 확인할 수 있다.
- 1115: BT 에셋 그래프는 코드로 만들 수 없어 같은 노드 구조를 실행 중에 조립(엔진 노드의 protected 설정값은 리플렉션으로 지정).

## 남은 문제 · 공통 변경 요청

- 3인칭 기본 카메라가 캐릭터를 화면 정중앙에 두어 정면의 데모 오브젝트를 가린다. PlayerStart 를 옆으로 비켜 두었지만 1113 표적은 발사 방향 때문에 일부 가려진다.
- 1115: Gameplay Debugger 오버레이(엔진 HUD, 왼쪽 위 고정)가 정보 패널과 겹친다. F1 로 패널을 접으면 된다.
- 1113 L(?listen 전환)은 shot 으로 검증하지 않음(Windows 방화벽 창 가능성). 2인 PIE 는 에디터에서만 확인 가능.
- 1112 실제 Live Coding 패치(Ctrl+Alt+F11)는 자동 검증 불가 — 상태 표시(모듈 로드됨·세션 꺼짐)만 확인.
- 이전 shot.sh 에서는 연속 키 입력 중 첫 키만 반영되는 경우가 있었다(메인 세션이 키 유지 3프레임으로 수정함). 데모 쪽은 1104 에 K(즉시 처치) 키를 추가해 단일 키로 확인.
