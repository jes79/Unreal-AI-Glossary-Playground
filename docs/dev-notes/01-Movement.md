# 01 캐릭터 이동·조작·카메라 (GlossaryMovement) 작업 노트

모듈 `Source/GlossaryMovement`, 레벨 `/Game/Levels/01_Movement/L_01xx_*`, 생성 에셋 `/Game/Glossary/01_Movement/`.

## id 별 표

| id | 구현 방식 | 씬에서 보여 주는 것 | 검증 |
| --- | --- | --- | --- |
| 0101 | 런타임 데모 | 30°·45°·50° 램프와 30·45·60cm 턱, Walkable Floor Angle·Max Step Height 실시간 조절(초록=오를 수 있음/빨강=못 오름 색이 즉시 갱신) | shot 확인 (`]` 키) |
| 0102 | 런타임 데모 | 실행 중 NewObject 로 만든 IA_Move·IA_Jump·IA_Attack·IA_HeavyAttack + IMC_Default(키보드+게임패드, Negate·Swizzle·DeadZone·Pressed·Hold 0.5초), 오른쪽 패널에 매핑표와 Started/Ongoing/Triggered/Completed/Canceled 이벤트 로그 | shot 확인 (좌클릭 → 로그) |
| 0103 | 런타임 데모 | 차에 타면(E) IMC_Vehicle 을 우선순위 1로 추가·내리면 제거, 같은 WASD/Space/E 가 운전·경적·내리기로 바뀜, 활성 컨텍스트 목록 | shot 확인 |
| 0104 | 런타임 데모 | 같은 시간만 달리고 멈추는 두 러너(기본 값 vs Max Acceleration 1000·Braking 500·Friction 2) — 최고속 도달 시간·멈춘 뒤 미끄러진 거리·멈춘 자리 기둥, 플레이어도 C 로 전환 | shot 확인 |
| 0105 | 런타임 데모 | 높은 발판 끝에서 걸어 나간 뒤 0.1초 안의 점프 허용(OnMovementModeChanged 로 Falling 시각 기록 + CanJumpInternal), 성공/실패 라벨, C ON/OFF·T 슬로 모션 | shot 확인 |
| 0106 | 런타임 데모 | 징검다리 연속 점프, 착지 0.15초 전 입력을 기억했다 Landed 에서 실행(머리 위 버퍼 표시), C ON/OFF·T 슬로 모션 | shot 확인 |
| 0107 | 런타임 데모 | Jump Max Hold Time 0.3초 — 0.05초/0.3초 자동 점프 마네킹과 플레이어의 최고점 구슬·높이 눈금 비교, C ON/OFF | shot 확인 |
| 0108 | 런타임 데모 | Shift(IA_Dash, 실행 중 IMC) → LaunchCharacter 대시 0.2초 + 무적(포탑 공이 몸 통과) + 쿨타임 1초, 잔상·피격/회피 카운트, C 무적 ON/OFF | shot 확인 (LeftShift) |
| 0109 | 런타임 데모 | 수직 통로 두 벽 사이에서 벽 슬라이드(낙하 150cm/s 제한) + 벽 반대쪽으로 LaunchCharacter 벽 점프, 노란 원뿔 = 벽 법선, 높이 눈금·목표 발판 | shot 확인 |
| 0110 | 런타임 데모 | 등 뒤 벽이 있는 좁은 골목에서 Spring Arm 400 + Control Rotation, Do Collision Test 로 당겨진 길이 표시, C 로 끄면 카메라가 벽 너머로 가서 가려짐, [ ] 팔 길이 | shot 확인 (기본·C) |
| 0111 | 런타임 데모 | Enable Camera Lag(Speed 5)·Rotation Lag(Speed 10), R 원형 트랙 자동 달리기로 목표 위치와의 지연 거리 표시, C ON/OFF·1/2/3 속도 | shot 확인 |
| 0112 | 런타임 데모 | 포탑 공 피격 시 PlayerCameraManager->StartCameraShake(UCameraShakeBase + UPerlinNoiseCameraShakePattern 0.2초), 2 = Wave Oscillator 폭발 1초 비교, C ON/OFF | shot 확인 (`1` 키) |
| 0113 | 런타임 데모 | Tab 으로 가장 가까운 적 록온, Control Rotation 을 RInterpTo(속도 8)로 보간 + 스트레이프, 회전하는 노란 마커, 움직이는 적 | shot 확인 (Tab) |
| 0114 | 런타임 데모 | 1~4 키 TPS / FPS(Owner No See) / 탑다운(Absolute Rotation, 커서 클릭 → SimpleMoveToLocation, NavMesh) / 횡스크롤(Plane Constraint) | shot 확인 (1·3·4: 0114, 0114_3, 0114_4) |
| 0115 | 런타임 데모 | E 로 근처 드론 Pawn(FloatingPawnMovement·자기 카메라·IMC_Drone 우선순위 1)에 Possess, 다시 E 로 캐릭터 복귀, 오른쪽 패널에 조종 중 Pawn·각 폰의 Controller·활성 컨텍스트 | shot 확인 (기본·E 빙의 = 0115_E) |
| 0116 | 런타임 데모 | BuildLevel 에서 놓은 Physics Volume(bWaterVolume, Fluid Friction 0.3, 반투명 M_Water 상자로 표시)에 들어가면 Swimming, F 로 Flying, G 물속 순간이동, 탑에서 Falling — 머리 위 모드 라벨과 전환 기록 | shot 확인 (기본·G=Swimming·F=Flying) |

## 생성 에셋
- `/Game/Glossary/01_Movement/M_Water` — 반투명·양면 머티리얼(Color·Opacity·Emissive 파라미터). 0116 Water Volume 시각화용.
- 레벨에 저장되는 액터: L_0116 의 `WaterVolume` (APhysicsVolume, BSP 큐브 브러시 1600×1600×400), L_0114 의 NavMeshBoundsVolume(빌더 공통).
- 그 밖의 Input Action / Mapping Context / Camera Shake 는 에셋 없이 실행 중 생성(NewObject) 또는 C++ 클래스(UD0112_PerlinHitShake, UD0112_WaveExplosionShake).

## 공용 코드
- `MovementKit` — 실행 중 IA/IMC 조립(MapWASD: Negate·Swizzle, MapStick: DeadZone), 활성 IMC 목록 문자열, 단순 직선 투사체(0108·0112).
- `UMovementSidePanel` — 오른쪽 위 보조 패널(0102 로그, 0103·0115 컨텍스트 목록, 0116 모드 기록).
- 데모별 보조 클래스: `D01xx_*` (캐릭터 서브클래스, 드론 폰, 카메라 셰이크).

## 메모
- Build.cs: `EngineCameras`(Perlin/Wave 셰이크 패턴, 기본 켜진 플러그인) 추가, 에디터 빌드에서만 `UnrealEd`·`BSPUtils`(0116 볼륨 브러시).
- Landed 안에서 Jump() 하면 같은 프레임 ClearJumpInput 으로 지워진다 → 0106 은 다음 Tick 에 실행.
- 0107 자동 점프 마네킹은 AIController 를 붙여야 CharacterMovement 가 Jump 입력을 처리한다(0104 러너도 같음).

## 남은 문제·공통 변경 요청
- 대체 구현 항목 없음(16개 모두 실제 UE 기능으로 런타임 데모).
- shot.sh 는 키를 같은 프레임에 눌렀다 떼므로 Enhanced Input 의 Hold/Down 계열(0102 의 Space=IA_Jump 등)은 shot 에서 이벤트가 안 찍힐 수 있다 — 실제 플레이에서는 정상. 이동·대시 중 모습은 정지 shot 으로는 확인 한계(0111 은 R 자동 달리기로 보완).
- glossary-build 가 매번 00_Common(M_Color) 빌더도 같이 돌리는데, 다른 프로세스가 M_Color.uasset 을 잡고 있으면 저장(MoveFile) 실패로 파일이 한때 사라졌다(재실행으로 복구됨). 요청: `-filter=NN` 일 때 00_Common 은 이미 있으면 건너뛰기, 또는 저장 실패 시 원본 유지.
- 0114 횡스크롤은 시작 위치가 코스(X 200~)보다 왼쪽이라 4 를 누른 직후에는 발판이 화면 오른쪽 밖 — 오른쪽(D)으로 가면 보인다.
