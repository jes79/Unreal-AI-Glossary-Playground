# 06 UI·UX (UMG)

UMG 위젯 트리를 C++ 에서 만듭니다. Widget Blueprint 가 필요한 용어는 같은 구조를 코드로 보여 줍니다.

레벨 18개 · 폴더 `/Game/Levels/06_UI` · 모듈 `Source/GlossaryUI`

| 번호 | 용어 | 쉽게 말하면 | 레벨에서 보여 주는 것 | 구현 방식 |
| --- | --- | --- | --- | --- |
| 0601 | [Widget Blueprint (UMG)](#0601) | 화면 UI를 만드는 에셋 | 체력바·점수 HUD 를 CreateWidget + AddToViewport, BindWidget 베이스 클래스(UDemo0601_HudBase) 패턴 | 설명 + 대체 구현 |
| 0602 | [Anchor / DPI Scaling](#0602) | 해상도가 바뀌어도 제자리 유지 | 네 모서리·가운데·가로 늘이기 Anchor 와 절대 좌표 나쁜 예, 해상도 전환 + DPI 커브 ON/OFF 와 GetViewportScale 표시 | 런타임 데모 |
| 0603 | [이벤트 갱신 vs Property Binding](#0603) | 매 프레임 검사 vs 바뀔 때만 갱신 | 같은 체력바를 Property Binding(TextDelegate/PercentDelegate, 매 프레임) vs Delegate(변경 시)로 그리고 호출 횟수 비교 | 런타임 데모 |
| 0604 | [체력바 (지연 감소)](#0604) | 닳은 만큼 뒤쪽 바가 천천히 줄어듦 | 빨간 바 즉시 감소 + 흰 고스트 바 0.5초 뒤 FInterpTo 추적, 지연 ON/OFF | 런타임 데모 |
| 0605 | [데미지 숫자 (Floating Text)](#0605) | 맞은 자리에 떠오르는 숫자 | 적 머리 위 데미지 숫자 팝업·상승·페이드, 치명타는 노란색 큰 글자 | 런타임 데모 |
| 0606 | [Widget Component (월드 UI)](#0606) | 3D 공간에 붙는 UI | 적 머리 위 WidgetComponent(Screen) 체력바, 15m 밖이면 숨김 ON/OFF | 런타임 데모 |
| 0607 | [Widget Animation](#0607) | UI 움직임 애니메이션 | 실행 중 조립한 UWidgetAnimation 으로 창 슬라이드-인(이동+투명도)과 버튼 호버 1.1배 | 설명 + 대체 구현 |
| 0608 | [일시정지 메뉴 + Input Mode](#0608) | 멈추고 마우스로 메뉴 조작 | ESC 로 SetGamePaused + Input Mode UI Only + 커서, 다시 누르면 Game Only, 월드 시간 정지 | 런타임 데모 |
| 0609 | [ListView (목록 가상화)](#0609) | 긴 목록을 보이는 만큼만 만듦 | 아이템 1000개 ListView, 생성된 엔트리 위젯 수·재사용 횟수 표시 | 런타임 데모 |
| 0610 | [드래그 앤 드롭](#0610) | 슬롯 아이템 끌어 옮기기 | 4×4 인벤토리 DetectDrag → OnDragDetected(DragDropOperation) → OnDrop 옮기기·교환 | 런타임 데모 |
| 0611 | [툴팁](#0611) | 마우스를 올리면 뜨는 설명 | 슬롯 호버 시 SetToolTip 커스텀 툴팁(이름·등급 색·설명·능력치) | 런타임 데모 |
| 0612 | [대화창 타이핑 효과](#0612) | 글자가 하나씩 찍힘 | 글자당 0.03초 타이핑 대화창, 클릭 시 전체 표시 → 다음 대사 | 런타임 데모 |
| 0613 | [페이드 / 로딩 화면](#0613) | 장면 전환 시 검게 가림 | StartCameraFade 0.5초 → 로딩 위젯 → OpenLevel 로 같은 레벨 재시작 → 페이드 인 | 런타임 데모 |
| 0614 | [미니맵 (SceneCapture2D)](#0614) | 위에서 내려다본 작은 지도 | 플레이어 위 SceneCaptureComponent2D(Ortho) → Render Target → 오른쪽 위 Image 미니맵, 확대·회전 | 런타임 데모 |
| 0615 | [화면 밖 마커 (Waypoint)](#0615) | 목표 방향을 화면 가장자리에 표시 | ProjectWorldLocationToScreen 목표 마커, 화면 밖이면 가장자리 방향 화살표 + 거리(m) | 런타임 데모 |
| 0616 | [Common UI](#0616) | 게임패드 포커스·메뉴 스택 표준 | UCommonActivatableWidgetStack 메인→옵션→그래픽 스택, Esc/패드 B 로 pop·포커스 복원 | 설명 + 대체 구현 |
| 0617 | [MVVM (Viewmodel)](#0617) | 데이터와 UI 사이 중간 객체로 자동 바인딩 | Health·MaxHealth FieldNotify 뷰모델 하나를 체력바·배지 두 View 가 구독 | 설명 + 대체 구현 |
| 0618 | [텍스트 현지화 (FText·String Table)](#0618) | 한/영 언어 전환 | 코드로 만든 String Table 키만 쓰는 메뉴, SetCurrentCulture 로 한/영 전환(문구·Format 어순·날짜) | 설명 + 대체 구현 |

## 항목별 상세

### 0601

**Widget Blueprint (UMG)** — 화면 UI를 만드는 에셋

> 요청 예문: "체력바와 점수 텍스트가 있는 WBP_HUD를 만들고 CreateWidget + AddToViewport로 띄워줘."

- 레벨: `/Game/Levels/06_UI/L_0601_WidgetBlueprint`
- 데모 클래스: `ADemo0601_WidgetBlueprint` (`Source/GlossaryUI/Public/Demo0601_WidgetBlueprint.h`)
- 구현 방식: 설명 + 대체 구현
- 레벨에서 보여 주는 것: 체력바·점수 HUD 를 CreateWidget + AddToViewport, BindWidget 베이스 클래스(UDemo0601_HudBase) 패턴

### 0602

**Anchor / DPI Scaling** — 해상도가 바뀌어도 제자리 유지

> 요청 예문: "체력바는 왼쪽 위, 미니맵은 오른쪽 위 Anchor로 고정하고 DPI Scaling 커브를 1080p 기준으로 맞춰줘."

- 레벨: `/Game/Levels/06_UI/L_0602_AnchorDPI`
- 데모 클래스: `ADemo0602_AnchorDPI` (`Source/GlossaryUI/Public/Demo0602_AnchorDPI.h`)
- 구현 방식: 런타임 데모
- 레벨에서 보여 주는 것: 네 모서리·가운데·가로 늘이기 Anchor 와 절대 좌표 나쁜 예, 해상도 전환 + DPI 커브 ON/OFF 와 GetViewportScale 표시

### 0603

**이벤트 갱신 vs Property Binding** — 매 프레임 검사 vs 바뀔 때만 갱신

> 요청 예문: "Bind 함수 대신 체력이 바뀔 때만 Delegate로 ProgressBar를 갱신하게 바꿔줘."

- 레벨: `/Game/Levels/06_UI/L_0603_EventDrivenUI`
- 데모 클래스: `ADemo0603_EventDrivenUI` (`Source/GlossaryUI/Public/Demo0603_EventDrivenUI.h`)
- 구현 방식: 런타임 데모
- 레벨에서 보여 주는 것: 같은 체력바를 Property Binding(TextDelegate/PercentDelegate, 매 프레임) vs Delegate(변경 시)로 그리고 호출 횟수 비교

### 0604

**체력바 (지연 감소)** — 닳은 만큼 뒤쪽 바가 천천히 줄어듦

> 요청 예문: "피격 시 빨간 바는 즉시, 뒤쪽 흰 바는 0.5초 뒤 FInterpTo로 따라 줄어들게 해줘."

- 레벨: `/Game/Levels/06_UI/L_0604_HealthBar`
- 데모 클래스: `ADemo0604_HealthBar` (`Source/GlossaryUI/Public/Demo0604_HealthBar.h`)
- 구현 방식: 런타임 데모
- 레벨에서 보여 주는 것: 빨간 바 즉시 감소 + 흰 고스트 바 0.5초 뒤 FInterpTo 추적, 지연 ON/OFF

### 0605

**데미지 숫자 (Floating Text)** — 맞은 자리에 떠오르는 숫자

> 요청 예문: "적이 맞으면 머리 위에 데미지 숫자가 떠올랐다 사라지게 하고, 치명타는 노란색으로 크게 해줘."

- 레벨: `/Game/Levels/06_UI/L_0605_DamageNumbers`
- 데모 클래스: `ADemo0605_DamageNumbers` (`Source/GlossaryUI/Public/Demo0605_DamageNumbers.h`)
- 구현 방식: 런타임 데모
- 레벨에서 보여 주는 것: 적 머리 위 데미지 숫자 팝업·상승·페이드, 치명타는 노란색 큰 글자

### 0606

**Widget Component (월드 UI)** — 3D 공간에 붙는 UI

> 요청 예문: "적 머리 위에 WidgetComponent(Space: Screen)로 체력바를 붙이고, 멀어지면 숨겨줘."

- 레벨: `/Game/Levels/06_UI/L_0606_WidgetComponent`
- 데모 클래스: `ADemo0606_WidgetComponent` (`Source/GlossaryUI/Public/Demo0606_WidgetComponent.h`)
- 구현 방식: 런타임 데모
- 레벨에서 보여 주는 것: 적 머리 위 WidgetComponent(Screen) 체력바, 15m 밖이면 숨김 ON/OFF

### 0607

**Widget Animation** — UI 움직임 애니메이션

> 요청 예문: "버튼에 마우스를 올리면 Widget Animation으로 1.1배 커지고, 창은 아래에서 슬라이드되며 열리게 해줘."

- 레벨: `/Game/Levels/06_UI/L_0607_WidgetAnimation`
- 데모 클래스: `ADemo0607_WidgetAnimation` (`Source/GlossaryUI/Public/Demo0607_WidgetAnimation.h`)
- 구현 방식: 설명 + 대체 구현
- 레벨에서 보여 주는 것: 실행 중 조립한 UWidgetAnimation 으로 창 슬라이드-인(이동+투명도)과 버튼 호버 1.1배

### 0608

**일시정지 메뉴 + Input Mode** — 멈추고 마우스로 메뉴 조작

> 요청 예문: "ESC로 SetGamePaused와 Input Mode UI Only, 마우스 커서 표시를 하고, 다시 누르면 Game Only로 돌아가게 해줘."

- 레벨: `/Game/Levels/06_UI/L_0608_PauseMenu`
- 데모 클래스: `ADemo0608_PauseMenu` (`Source/GlossaryUI/Public/Demo0608_PauseMenu.h`)
- 구현 방식: 런타임 데모
- 레벨에서 보여 주는 것: ESC 로 SetGamePaused + Input Mode UI Only + 커서, 다시 누르면 Game Only, 월드 시간 정지

### 0609

**ListView (목록 가상화)** — 긴 목록을 보이는 만큼만 만듦

> 요청 예문: "ListView로 아이템 1000개 목록을 만들고 Entry Widget을 재사용하게(IUserObjectListEntry) 해줘."

- 레벨: `/Game/Levels/06_UI/L_0609_ListView`
- 데모 클래스: `ADemo0609_ListView` (`Source/GlossaryUI/Public/Demo0609_ListView.h`)
- 구현 방식: 런타임 데모
- 레벨에서 보여 주는 것: 아이템 1000개 ListView, 생성된 엔트리 위젯 수·재사용 횟수 표시

### 0610

**드래그 앤 드롭** — 슬롯 아이템 끌어 옮기기

> 요청 예문: "인벤토리 슬롯에 OnDragDetected / OnDrop을 구현해서 아이템을 옮기고 교환하게 해줘."

- 레벨: `/Game/Levels/06_UI/L_0610_DragDrop`
- 데모 클래스: `ADemo0610_DragDrop` (`Source/GlossaryUI/Public/Demo0610_DragDrop.h`)
- 구현 방식: 런타임 데모
- 레벨에서 보여 주는 것: 4×4 인벤토리 DetectDrag → OnDragDetected(DragDropOperation) → OnDrop 옮기기·교환

### 0611

**툴팁** — 마우스를 올리면 뜨는 설명

> 요청 예문: "슬롯에 마우스를 올리면 이름·등급·설명이 있는 커스텀 툴팁 위젯을 띄워줘."

- 레벨: `/Game/Levels/06_UI/L_0611_Tooltip`
- 데모 클래스: `ADemo0611_Tooltip` (`Source/GlossaryUI/Public/Demo0611_Tooltip.h`)
- 구현 방식: 런타임 데모
- 레벨에서 보여 주는 것: 슬롯 호버 시 SetToolTip 커스텀 툴팁(이름·등급 색·설명·능력치)

### 0612

**대화창 타이핑 효과** — 글자가 하나씩 찍힘

> 요청 예문: "대사를 글자당 0.03초로 타이핑하고, 클릭하면 전체 표시 후 다음 대사로 넘어가게 해줘."

- 레벨: `/Game/Levels/06_UI/L_0612_Typewriter`
- 데모 클래스: `ADemo0612_Typewriter` (`Source/GlossaryUI/Public/Demo0612_Typewriter.h`)
- 구현 방식: 런타임 데모
- 레벨에서 보여 주는 것: 글자당 0.03초 타이핑 대화창, 클릭 시 전체 표시 → 다음 대사

### 0613

**페이드 / 로딩 화면** — 장면 전환 시 검게 가림

> 요청 예문: "PlayerCameraManager StartCameraFade로 0.5초 페이드 아웃 후 로딩 위젯을 띄우고 OpenLevel 해줘."

- 레벨: `/Game/Levels/06_UI/L_0613_FadeLoading`
- 데모 클래스: `ADemo0613_FadeLoading` (`Source/GlossaryUI/Public/Demo0613_FadeLoading.h`)
- 구현 방식: 런타임 데모
- 레벨에서 보여 주는 것: StartCameraFade 0.5초 → 로딩 위젯 → OpenLevel 로 같은 레벨 재시작 → 페이드 인

### 0614

**미니맵 (SceneCapture2D)** — 위에서 내려다본 작은 지도

> 요청 예문: "플레이어 위 SceneCaptureComponent2D(Orthographic)를 Render Target에 그려 오른쪽 위 미니맵으로 보여줘."

- 레벨: `/Game/Levels/06_UI/L_0614_Minimap`
- 데모 클래스: `ADemo0614_Minimap` (`Source/GlossaryUI/Public/Demo0614_Minimap.h`)
- 구현 방식: 런타임 데모
- 레벨에서 보여 주는 것: 플레이어 위 SceneCaptureComponent2D(Ortho) → Render Target → 오른쪽 위 Image 미니맵, 확대·회전

### 0615

**화면 밖 마커 (Waypoint)** — 목표 방향을 화면 가장자리에 표시

> 요청 예문: "ProjectWorldToScreen으로 목표 위치를 표시하고, 화면 밖이면 가장자리에 방향 화살표와 거리(m)를 띄워줘."

- 레벨: `/Game/Levels/06_UI/L_0615_Waypoint`
- 데모 클래스: `ADemo0615_Waypoint` (`Source/GlossaryUI/Public/Demo0615_Waypoint.h`)
- 구현 방식: 런타임 데모
- 레벨에서 보여 주는 것: ProjectWorldLocationToScreen 목표 마커, 화면 밖이면 가장자리 방향 화살표 + 거리(m)

### 0616

**Common UI** — 게임패드 포커스·메뉴 스택 표준

> 요청 예문: "Common UI의 Activatable Widget Stack으로 메인→옵션→그래픽 메뉴 스택을 만들고 패드 B버튼으로 뒤로 가게 해줘."

- 레벨: `/Game/Levels/06_UI/L_0616_CommonUI`
- 데모 클래스: `ADemo0616_CommonUI` (`Source/GlossaryUI/Public/Demo0616_CommonUI.h`)
- 구현 방식: 설명 + 대체 구현
- 레벨에서 보여 주는 것: UCommonActivatableWidgetStack 메인→옵션→그래픽 스택, Esc/패드 B 로 pop·포커스 복원

### 0617

**MVVM (Viewmodel)** — 데이터와 UI 사이 중간 객체로 자동 바인딩

> 요청 예문: "UMG Viewmodel 플러그인으로 Health·MaxHealth 뷰모델을 만들고 체력바에 바인딩해줘."

- 레벨: `/Game/Levels/06_UI/L_0617_MVVM`
- 데모 클래스: `ADemo0617_MVVM` (`Source/GlossaryUI/Public/Demo0617_MVVM.h`)
- 구현 방식: 설명 + 대체 구현
- 레벨에서 보여 주는 것: Health·MaxHealth FieldNotify 뷰모델 하나를 체력바·배지 두 View 가 구독

### 0618

**텍스트 현지화 (FText·String Table)** — 한/영 언어 전환

> 요청 예문: "UI 문자열을 FText와 String Table로 바꾸고 Localization Dashboard로 한국어·영어를 만들어 설정에서 전환하게 해줘."

- 레벨: `/Game/Levels/06_UI/L_0618_Localization`
- 데모 클래스: `ADemo0618_Localization` (`Source/GlossaryUI/Public/Demo0618_Localization.h`)
- 구현 방식: 설명 + 대체 구현
- 레벨에서 보여 주는 것: 코드로 만든 String Table 키만 쓰는 메뉴, SetCurrentCulture 로 한/영 전환(문구·Format 어순·날짜)
