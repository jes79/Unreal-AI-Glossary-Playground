# 10 조명·렌더링·최적화 (GlossaryRendering) 작업 노트

모듈: `Source/GlossaryRendering` · 레벨: `/Game/Levels/10_Rendering/L_10xx_*` · 생성 에셋: `/Game/Glossary/10_Rendering/`
공통 도구: `RenderingDemoKit.h` — stat unit 숫자(Game/Draw/GPU/DrawCalls) 읽기, 콘솔 변수 바꾸고 EndPlay 에서 되돌리기(`FCVarScope`), 고정/비행 카메라, Post Process 덮어쓰기, BuildLevel 용 박스 볼륨.

## id 별 표

| id | 구현 방식 | 씬에서 보여 주는 것 | 검증 |
| --- | --- | --- | --- |
| 1001 | 런타임 데모 | 코넬 박스(빨강·초록 벽), 빨간 벽에 비춘 스포트라이트의 반사광이 바닥을 물들임, Lumen/None/SSGI 전환(Post Process GI Method) | shot 확인 (ON/OFF 2장) |
| 1002 | 런타임 데모 | Roughness 0.05 대리석 바닥, 화면 밖 노란 간판·잘린 기둥 반사로 Lumen 반사 vs SSR vs None 비교 | shot 확인 (Lumen/SSR) |
| 1003 | 런타임 데모 | 빌더가 만든 33만 삼각형 바위 20개(Nanite), r.Nanite.Visualize Triangles·Clusters, Nanite 끈 메시와 GPU 시간 비교 | shot 확인 (일반 화면 정상, Visualize 화면은 검게 나옴 — 남은 문제) |
| 1004 | 런타임 데모 | 2cm 철망·울타리·격자 지붕 그림자, r.Shadow.Virtual.Enable 1/0 으로 VSM vs CSM, CSM 해상도 조절 | shot 확인 |
| 1005 | 설명 + 대체 구현 | BuildLevel 이 Static 천장등 2·Stationary 스탠드·Movable 흔들리는 전등을 레벨에 저장, 실행 중 색 변경/이동 시도로 Mobility 차이 | shot 확인 |
| 1006 | 런타임 데모 | Directional(Atmosphere Sun)·Sky Atmosphere·Sky Light(Real Time Capture)·Volumetric Cloud, 해 고도 조절·자동 하루·구성 요소 끄기 | shot 확인 |
| 1007 | 런타임 데모 | 창문 있는 어두운 방, Exponential Height Fog + Volumetric Fog 로 창살 사이 God Ray, ON/OFF·밀도·산란 방향 | shot 확인 |
| 1008 | 런타임 데모 | 복도 Rect Light 형광등·무대 Spot(Inner 20°/Outer 35°)·Point 전구, 빌더가 만든 Light Function 머티리얼로 깜빡임 | shot 확인 |
| 1009 | 런타임 데모 | Lumen 끈 모바일 설정, 주황·파란 방마다 Sphere Reflection Capture(BuildLevel, Runtime Capture), 크롬 구 반사 ON/OFF·Lumen 비교 | shot 확인 |
| 1010 | 런타임 데모 | 빌더가 자동 생성한 LOD 3단계(50/25/10%, Screen Size 1/0.5/0.25/0.1) 바위 5개, 현재 LOD 색·삼각형 수 라벨, LOD 강제·와이어프레임 | shot 확인 |
| 1011 | 설명 + 대체 구현 | 건물 25채(75 컴포넌트) ↔ 빌더가 합친 HLOD 프록시 메시 1개, 80m 전환 거리·강제 전환·프록시 색 표시 | shot 확인 (개별/프록시) |
| 1012 | 런타임 데모 | 나무 5000그루를 HISM 1개 vs StaticMeshActor 5000개로 전환, DrawCalls·Game/Draw/GPU ms·생성 시간 비교 | shot 확인 (HISM/액터) |
| 1013 | 런타임 데모 | BuildLevel 의 Cull Distance Volume(50cm 이하 → 3000 밖 컬링) + 가로등 Max Draw Distance 1500, 그려진 소품 수(132/600) 표시 | shot 확인 |
| 1014 | 런타임 데모 | UGameUserSettings 옵션 메뉴(품질 낮음~시네마틱·해상도·프레임 제한) + ApplySettings, 숲 장면 품질·GPU 시간 비교, 나갈 때 원래 설정 복원 | shot 확인 |
| 1015 | 런타임 데모 | Game(12ms 계산)·Draw(액터 4000)·GPU(해상도 200%) 병목 스위치 + stat unit/unitgraph/gpu, Trace.File 로 Unreal Insights 트레이스 | shot 확인 |
| 1016 | 런타임 데모 | 가는 철사·먼 울타리·회전 날개·체크무늬, r.AntiAliasingMethod TSR/TAA/FXAA/없음 + r.ScreenPercentage 67 화질·GPU 비교 | shot 확인 |
| 1017 | 설명 + 대체 구현 | 768m 월드를 32m 셀 24×24 로 나누고 Loading Range 12800 안의 셀만 로드/언로드, 위에서 본 지도 + 노란 범위 원 | shot 확인 |
| 1018 | 런타임 데모 | 코드로 만든 PCG 그래프: 영역 Spline 면에 Surface Sampler 0.3/㎡ 나무, 길 Spline 주변 Difference 로 비움, 실행 중 재생성·길 옮기기 | shot 확인 |
| 1019 | 런타임 데모 | Volumetric Cloud Layer Bottom Altitude 5km, 구름 양을 머티리얼 파라미터(Cloud_GlobalCoverage) MID 로 실시간 조절, 해 고도 | shot 확인 |

## 생성 에셋 (`/Game/Glossary/10_Rendering/`, 빌더 `RenderingAssets.cpp`)

- `SM_NaniteRock` (327,680 삼각형, Nanite 켬) / `SM_NaniteRock_NoNanite` (같은 원본, Nanite 끔) — 이십면체 7분할 + Perlin 변위, MeshDescription 으로 생성
- `SM_LODRock` (20,480 삼각형, LOD1~3 = Reduction 50/25/10%, Screen Size 1.0/0.5/0.25/0.1)
- `SM_Tree` (줄기·잎 2 슬롯), `M_Foliage`(Used with Instanced Static Meshes) + `MI_TreeTrunk` · `MI_TreeLeaves`
- `SM_HLODProxy_Town` (건물 25채를 상자 하나씩 합친 프록시) + `MI_HLODProxy`, `MI_Rock`
- `M_LightFunction_Flicker` (MD_LightFunction, Custom HLSL 깜빡임, FlickerSpeed 파라미터)
- `PCG_ForestScatter` (Get Spline Data ×2 → Create Surface From Spline → Surface Sampler 0.3 → Difference ← Bounds Modifier ← Spline Sampler → Transform Points → Static Mesh Spawner)

## 남은 문제 · 공통 변경 요청

- 1003: `r.Nanite.Visualize Triangles/Clusters` 를 켜면 -game(쿡 안 한 에디터 실행)에서 화면 전체가 검게 나오고 범례("Triangles")만 표시된다. 자동 노출로 바꾸고 25초 기다려도 같음(셰이더 대기 문제 아님). 일반 화면·Nanite 끈 메시 비교·삼각형 수 표시는 정상. 에디터 뷰포트 Lit > Nanite Visualization 에서는 볼 수 있으니 정보 패널 안내로 보완 필요(미해결).
- 공통 `M_Color` 에 `bUsedWithInstancedStaticMeshes` 가 꺼져 있어 HISM/ISM·PCG 인스턴스에 쓰면 기본(회색) 머티리얼로 그려진다. 이 분야는 `M_Foliage` 를 따로 만들어 해결. 요청: GlossaryCore 의 M_Color 빌더에서 이 플래그를 켜 주면 다른 분야도 인스턴싱에 쓸 수 있다.
- Indoor 하늘(Sky Light Real Time Capture + 하늘 없음)에서 엔진 화면 경고("A sky light with real time capture ... requires SkyAtmosphere ...")가 빨간 글씨로 뜬다(1005·1008). 요청: Environment Indoor 에서 SkyLight 의 Real Time Capture 를 끄거나 캡처 큐브맵을 지정.
- 1005: "LIGHTING NEEDS TO BE REBUILT" 경고는 의도된 것(커맨드렛에서 Lightmass 를 돌리지 않음).
- 스폰 회전이 라이트 액터(ASpotLight/ARectLight)에 반영되지 않는 경우가 있어 컴포넌트에 SetWorldRotation 을 명시했다.

## 대체 구현과 이유

- 1005 Light Mobility: Build Lighting(Lightmass/Swarm) 을 레벨 생성 커맨드렛에서 돌리지 않아(BuiltData 패키지가 저장 경로 확정 전에 생겨야 함) Static 라이트는 미리보기 상태. Mobility 제약(색 변경·이동 가능 여부)은 실제로 동작.
- 1011 HLOD: HLOD Layer·Build HLODs 는 World Partition 맵 + 에디터 빌드가 필요. 빌더가 합친 프록시 메시를 만들고 거리로 전환하는 것으로 개념을 보여 줌.
- 1017 World Partition: 공통 레벨 빌더가 일반 맵(NewBlankMap)을 만들고, WP 변환·OFPA 저장은 공통 빌더 수정이 필요. 같은 규칙(Cell Size·Loading Range 12800·스트리밍 소스)을 C++ 로 직접 구현.
