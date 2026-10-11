# 02 물리·충돌 (Chaos) — 구현 노트

모듈 `GlossaryPhysics` · 레벨 `/Game/Levels/02_Physics/L_02xx_*` · 생성 에셋 `/Game/Glossary/02_Physics/`

## 1. id 별 표

| id | 구현 방식 | 씬에서 보여 주는 것 | 검증 |
| --- | --- | --- | --- |
| 0201 | 런타임 데모 | Projectile 오브젝트 채널 총알이 Pawn=Overlap·WorldDynamic=Block·Projectile=Ignore 로 다르게 반응, BlockAll 프리셋(1키)과 비교 | shot 확인 |
| 0202 | 런타임 데모 | 벽 OnComponentHit(막힘) vs 코인 OnComponentBeginOverlap(통과+점수 10), Generate Overlap Events 끄기(1키) 비교 | shot 확인 (자동 공이 코인 획득·벽 Hit) |
| 0203 | 런타임 데모 | 카메라 정면 LineTraceSingleByChannel 1000cm, 맞은 액터 이름·거리 표시 + DrawDebugLine(초록/빨강) | shot 확인 |
| 0204 | 런타임 데모 | SphereTraceMulti 반지름 50 근접 판정으로 범위 안 적 3명 동시 타격, Line Trace(1키)와 비교 | shot 확인 (E) |
| 0205 | 런타임 데모 | Simulate Physics 켠 상자 더미를 클릭/E 지점 AddImpulseAtLocation 으로 밀기, 꺼진 더미와 비교 | shot 확인 (E) |
| 0206 | 런타임 데모 | PM_Ice(Friction 0.05, Min) 상자 미끄러짐 vs 기본, PM_Rubber(Restitution 0.9, Max) 공 튀기 vs 기본 | shot 확인 (얼음 770cm·고무 328cm) |
| 0207 | 런타임 데모 | Physics Constraint 문 경첩(Swing1 ±90° 제한, 1키로 Free 비교) + 고리 8개 볼 조인트 체인 | shot 확인 |
| 0208 | 런타임 데모 | 클릭/E 위치 RadialForceComponent 반경 500 FireImpulse, 원 안 상자만 사방으로 날아감 | shot 확인 (E) |
| 0209 | 런타임 데모 | Manny 4명: E = 캡슐 끄고 Mesh Ragdoll 프로필+SetSimulatePhysics(true), Q = 죽음 애니메이션만(비교) | shot 확인 (E) |
| 0210 | 런타임 데모 | 25000cm/s 물리 총알이 4cm 벽을 터널링(CCD OFF) vs bUseCCD 로 막힘(CCD ON), 통과/막힘 횟수 | shot 확인 (OFF 통과 4 · ON 막힘 4) |
| 0211 | 런타임 데모 (에셋은 빌더 생성) | 코드로 Voronoi 20조각 낸 Geometry Collection 벽, 총알이 맞은 곳에 Field System(External Strain + Radial Velocity)으로 파괴 | shot 확인 (E → 20조각 무너짐) |
| 0212 | 런타임 데모 | ProjectileMovementComponent 초속 3000 총알 직선 궤적 vs Should Bounce 수류탄(Bounciness 0.5) 튀는 포물선, 3초 뒤 폭발 | shot 확인 |
| 0213 | 런타임 데모 | PhysicsHandleComponent 로 화면 가운데 물리 물체를 잡아 들고(좌클릭 유지/E), 놓으면 시선 방향으로 던지기 | shot 확인 (E 로 잡은 상태) |
| 0214 | 런타임 데모 | AWheeledVehiclePawn + ChaosWheeledVehicleMovementComponent 4륜 스포츠카를 WASD 로 운전(1키 자동 운전), 속도·기어·RPM | shot 확인 (자동 운전 40km/h) |

## 2. 생성 에셋 (빌더 `02_Physics`, `Private/GlossaryPhysicsModule.cpp`)

- `/Game/Glossary/02_Physics/PM_Ice` — Friction 0.05, Friction Combine = Min
- `/Game/Glossary/02_Physics/PM_Rubber` — Restitution 0.9, Restitution Combine = Max
- `/Game/Glossary/02_Physics/PM_Default` — 비교용 (Friction 0.7, Restitution 0.1)
- `/Game/Glossary/02_Physics/M_DestructibleWall` — GC 용 머티리얼 (`bUsedWithGeometryCollections`)
- `/Game/Glossary/02_Physics/GC_Wall` — 큐브(400×40×300)를 `FGeometryCollectionEngineConversion::AppendStaticMesh` 로 넣고
  `FVoronoiDiagram` + `FPlanarCells` + `CutMultipleWithPlanarCells`(PlanarCut) 로 20조각, `GetValidConvexHullData` 로 충돌 껍질 생성
- 복사한 템플릿 콘텐츠: `Content/Vehicles/` (엔진 `Templates/TemplateResources/Standard/Vehicles/Content` 그대로, FBX 제외 → `/Game/Vehicles/...`)

보조 클래스: `D0214_SportsCar` (AWheeledVehiclePawn), `UD0214_WheelFront/Rear` (UChaosVehicleWheel), `PhysicsDemoUtil` (고정 카메라·화면 중앙 트레이스).

## 3. 남은 문제 · 공통 변경 요청

- **SKM_SportsCar 가 렌더되지 않음**: 복사한 스켈레탈 메시는 로드·물리(Physics Asset, 바퀴 뼈)는 정상이지만 화면에 그려지지 않는다
  (SkeletalMeshActor 로 따로 놓아도 안 보임, LOD 1·렌더 데이터 있음). 그래서 같은 모양의 `SM_SportsCar`(차체)와 `SM_SportsCar_Wheel` 4개를
  바퀴 뼈 소켓에 붙여 보이게 했다. 물리·바퀴 애님(ABP_SportsCar)은 원래 스켈레탈 메시가 담당. 원인 조사는 에디터에서 SKM 를 열어 보는 것이 빠르다.
- 0211 `OnChaosBreakEvent` 가 오지 않는다(bNotifyBreaks 켬). 상태 줄은 직접 센 '움직인 조각 수'로 표시.
- 0211 GC 는 1단 클러스터라 한 번 맞으면 벽 전체가 무너진다(부분 파괴를 원하면 빌더에서 2단 Voronoi 또는 Connection Graph/Damage Threshold 조정).
- 0206 기본 상자도 처음 놓일 때 조금(약 130cm) 미끄러진다 — 얼음(770cm)과의 차이는 분명.
- `GlossaryPhysics` 가 `PlanarCut` 모듈에 의존 → UBT 경고 "uproject does not list plugin 'PlanarCut'". 동작은 Fracture 플러그인 의존으로 정상.
  **요청**: `.uproject` Plugins 에 `{ "Name": "PlanarCut", "Enabled": true, "TargetAllowList": ["Editor"] }` 추가하면 경고가 사라진다.
- 모듈 `bUseUnity = false` (데모 파일마다 익명 namespace 상수 이름이 겹쳐 Unity 빌드에서 충돌).

## 4. 대체 구현 항목과 이유

- 없음 (14개 모두 실제 엔진 기능으로 구현). 0214 의 차체·바퀴 '보이는 메시'만 위 3번 이유로 스태틱 메시를 덧붙였다(주행 물리는 Chaos Vehicle 그대로).
