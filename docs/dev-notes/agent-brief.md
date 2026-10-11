# 분야 구현 작업 지침 (에이전트 공통)

프로젝트: `C:\Dev\Unreal-AI-Glossary-Playground` — UE 5.8 C++. 언리얼 AI 요청 용어집 187개 용어를 각각 동작하는 예제 레벨로 만든다.
Unity 판(`C:\Dev\Unity-AI-Glossary-Playground`)과 같은 수준·형식이 목표: 용어마다 눈으로 차이가 보이는 데모, 켜기/끄기 비교, 정보 패널.

## 먼저 읽을 것
1. `docs/CONVENTIONS.md` — 규칙 전부 (반드시)
2. `GlossaryPlayground/Source/GlossaryCore/Public/*.h` — 쓸 수 있는 헬퍼 (GlossaryDemoBase, GlossaryKit, GlossaryAssetKit, GlossaryWidgets, GlossaryCharacter, GlossaryEnvironment)
3. 참고 구현: `Source/GlossaryMovement/Public|Private/Demo0101_CharacterMovement.*`
4. 내 분야 항목: `Source/GlossaryCore/Private/GlossaryCatalogData.inl` 에서 내 분야 id 의 용어·쉽게 말하면·요청 예문
5. Unity 판 같은 분야의 위키 `C:\Dev\Unity-AI-Glossary-Playground\docs\wiki\NN-*.md` (데모 아이디어 참고, 같은 용어가 있으면 비슷한 연출)

## 해야 할 일
- 내 분야의 **모든 id** 에 대해 `ADemo{id}_{Name}` 클래스를 내 모듈에 만든다 (이름은 카탈로그와 정확히 일치).
- 데모는 요청 예문을 실제로 구현한 결과물이어야 한다. 예문의 수치(0.1초, 반경 500 등)를 그대로 쓰고 UPROPERTY 로 노출.
- 가능하면 언리얼의 진짜 기능(컴포넌트·서브시스템·에셋)을 쓴다. 코드로 만들 수 없는 에디터 전용 에셋은 (1) GlossaryAssetKit 빌더로 생성하거나 (2) 런타임에 같은 구조를 조립하거나 (3) 대체 구현 + `Implementation = Alternative; AlternativeNote = ...` 로 정직하게 표시.
- 에셋이 필요하면 내 모듈의 에셋 빌더(`Build...Assets`, 이름 `NN_Folder` 접두사)에서 생성한다.

## 작업 범위 (중요 — 다른 에이전트와 동시에 작업 중)
- **수정 가능**: `GlossaryPlayground/Source/Glossary{내 분야}/` 아래 전부(Build.cs 포함), `docs/dev-notes/{NN}-{분야}.md`(내 노트), `shots/{내 id}*.png`
- **수정 금지**: GlossaryCore, GlossaryEditor, 다른 분야 모듈, Config/*.ini, .uproject, tools/. 필요하면 내 모듈 안에서 해결하고, 꼭 필요한 공통 변경은 최종 보고에 "요청"으로 적는다.
- 플러그인이 꺼져 있어 필요한 경우도 보고만 한다(현재 켜진 목록은 CONVENTIONS.md).
- git commit 하지 않는다(메인 세션이 한다).

## 빌드·검증 (반드시 tools/locked.sh 로 감싼다 — 언리얼 프로세스는 한 번에 하나만)
```bash
cd /c/Dev/Unreal-AI-Glossary-Playground
tools/locked.sh tools/build.sh Glossary{분야}          # 내 모듈만 컴파일 (몇 분 걸릴 수 있음, timeout 600000)
tools/locked.sh tools/glossary-build.sh -filter={NN}   # 내 분야 에셋 + 레벨 생성 (에디터 커맨드렛)
tools/locked.sh tools/shot.sh {id} [초] [키,키]         # 게임 실행 → shots/{id}.png 자동 캡처, 로그 오류 출력
```
- `tools/glossary-build.sh` 출력에서 `[Glossary]` 줄과 `Error` 를 확인. 머티리얼 컴파일 오류도 여기 나온다.
- `tools/shot.sh` 의 키 이름은 FKey 이름: `One, Two, SpaceBar, C, E, LeftMouseButton, Tab, RightBracket` 등.
- **모든 레벨을 최소 한 번 shot 으로 찍어 Read 도구로 이미지를 직접 보고** 확인한다: 데모 오브젝트가 보이는지, 정보 패널과 겹치지 않는지, 상태 줄이 갱신되는지, 로그 Error 가 없는지. 토글 데모는 키를 넣은 shot 도 찍는다.
- 결과가 이상하면 고치고 다시 찍는다. 컴파일 오류는 내 모듈 것만 고친다(다른 모듈 오류로 실패하면 잠시 후 재시도).
- 빌드 중 "conflicting instance" 나 DLL 잠금 오류는 locked.sh 를 안 쓴 경우다. 항상 locked.sh.

## 최종 보고 (이 형식으로)
1. id 별 표: id | 구현 방식(런타임 데모/대체 구현/에디터 도구) | 씬에서 보여 주는 것(한 줄, 위키용) | 검증(shot 확인 여부)
2. 생성 에셋 목록
3. 남은 문제·공통 변경 요청
4. 대체 구현 항목과 이유
노트 파일 `docs/dev-notes/{NN}-{분야}.md` 에도 같은 표를 저장한다.
