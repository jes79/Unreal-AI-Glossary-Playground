# 04 머티리얼·셰이더 (GlossaryMaterial) 작업 노트

모든 머티리얼은 `Private/GlossaryMaterialAssets.cpp` 빌더(`04_Material`)가 **진짜 노드 그래프**로 만든다(에디터에서 열면 Fresnel·Panner·DepthFade·VertexColor·CollectionParameter·MaterialFunctionCall 노드가 그대로 보임).
Custom(HLSL) 노드는 0419(용어 자체가 Custom 노드)에서만 썼다. 공통 베이스 `AGlossaryMaterialDemoBase`(고정 카메라·에셋 로드·마네킹 헬퍼).
빌드 로그: 머티리얼 31개 모두 `컴파일 OK`, 레벨 20개 생성.

## id 별 표

| id | 구현 방식 | 씬에서 보여 주는 것 | 검증 |
| --- | --- | --- | --- |
| 0401 | 런타임 데모 | Metallic×Roughness 5×5 구 격자(M_PBR MID)와 상수 노드로 만든 빨강·Metallic 1·Roughness 0.2 금속 구 | shot 확인 |
| 0402 | 런타임 데모 | 부모 M_Base 하나에서 색만 바꾼 Material Instance 5개(MI_Base_*), C 로 부모와 비교 | shot 확인 |
| 0403 | 런타임 데모 | 피격 시 CreateDynamicMaterialInstance 로 만든 MID 의 Emissive 를 0.1초 빨갛게(자동 피격·슬로 모션·플래시 고정) | shot 확인 |
| 0404 | 런타임 데모 | MPC_Player.PlayerPosition 으로 플레이어 근처 풀(WPO)이 눌리고 바닥 링이 따라옴 — 머티리얼 2개가 값 공유 | shot 확인 |
| 0405 | 런타임 데모 | MF_RimLight(FunctionInput/Output + Fresnel) 하나를 금속·돌·유령 머티리얼 3개가 Material Function Call 로 재사용 | shot 확인 |
| 0406 | 런타임 데모 | Fresnel(Exponent 3) 파란 림 라이트 ON/OFF, Exponent 조절 | shot 확인 |
| 0407 | 런타임 데모 | Emissive 주황×20 오브·막대가 Bloom 으로 번지고 Lumen 으로 바닥을 비춤, Bloom ON/OFF | shot 확인 |
| 0408 | 런타임 데모 | 노이즈 + Opacity Mask 1초 디졸브, 경계 주황 Emissive, 자동 반복·0.5 고정 | shot 확인 |
| 0409 | 런타임 데모 | Translucent·Unlit 홀로그램(위로 흐르는 스캔라인·Fresnel·Sine 깜빡임) 마네킹, 원래 머티리얼과 비교 | shot 확인 |
| 0410 | 런타임 데모 | Dot(N,L) 3단계 툰 머티리얼(해 방향을 매 Tick 전달) vs Lit, 포스트 프로세스 툰(화면 양자화) 비교 | shot 확인 |
| 0411 | 런타임 데모 | 선택한 적에만 Render CustomDepth + 포스트 프로세스 머티리얼(Custom Depth 경계 검출)로 노란 외곽선 | shot 확인 |
| 0412 | 런타임 데모 | WPO 에 Sine(Time) — 풀밭(ISM)과 정점 많은 깃발(Procedural Mesh)이 흔들림 | shot 확인 |
| 0413 | 런타임 데모 | Panner(Speed=(PanSpeed 0.1,0))로 용암 강이 흐름, 멈춘 판과 비교 | shot 확인 |
| 0414 | 런타임 데모 | 크기 다른 바위: 메시 UV(늘어남) vs 엔진 WorldAlignedTexture 함수(트라이플래너) | shot 확인 |
| 0415 | 런타임 데모 | 줄무늬 벽 앞 Opaque·Masked 잎(Clip 0.33)·Translucent 유리·Additive 불빛 | shot 확인 |
| 0416 | 런타임 데모 | 맞은 지점에 Deferred Decal 총알 자국 SpawnDecalAtLocation, SetFadeOut(10초, 1초) + Decal Lifetime Opacity | shot 확인 |
| 0417 | 런타임 데모 | Procedural Mesh 바닥 정점 색 R 을 마우스 브러시로 칠해 돌↔이끼 Lerp (Mesh Paint 대용) | shot 확인 |
| 0418 | 런타임 데모 | 물 평면 Opacity=DepthFade(0.85, 100cm) — 바위·경사·기둥 교차선이 부드럽게, ON/OFF | shot 확인 |
| 0419 | 런타임 데모 | Custom 노드 HLSL 체커보드(floor·fmod), CellCount 파라미터 조절 | shot 확인 |
| 0420 | 런타임 데모 | 노이즈 Panner 두 장 → Refraction(2D Offset) 열 아지랑이, 뒤 줄무늬가 일렁임 | shot 확인 |

## 생성 에셋 (/Game/Glossary/04_Material)

- 텍스처: T_Noise, T_Lava, T_RockGrid, T_Leaf(알파), T_BulletHole(알파), T_Stone, T_Moss
- MPC: MPC_Player (PlayerPosition, PushRadius, PushStrength)
- 함수: MF_RimLight
- 머티리얼: M_PBR, M_PBR_RedMetal, M_Base, M_HitFlash, M_Grass_MPC, M_Soil_MPC, M_Rim_Metal, M_Rim_Stone, M_Rim_Ghost, M_Fresnel, M_Emissive, M_Dissolve, M_Hologram,
  M_Toon, M_ToonPostProcess, M_OutlinePostProcess, M_WindGrass, M_Flag, M_Lava, M_Rock_UV, M_Rock_WorldAligned, M_Blend_Opaque, M_Blend_Masked, M_Blend_Translucent,
  M_Blend_Additive, M_BulletHoleDecal, M_VertexColorBlend, M_Water_DepthFade, M_CustomChecker, M_HeatHaze
- 인스턴스: MI_Base_Red/Orange/Green/Blue/Purple

## 남은 문제·메모

- Build.cs 에 `ProceduralMeshComponent`(기본 활성 플러그인) 의존 추가. .uproject 에 플러그인이 명시돼 있지 않지만 EnabledByDefault 라 동작. 명시하려면 메인 세션에서 .uproject 에 추가 (요청).
- `UMaterialParameterCollection::Get*ParameterIndexByName` 은 export 되지 않아 배열 검색으로 대체.
- 0413: Panner 는 Time×Speed 라 실행 중 PanSpeed 를 바꾸면 무늬가 한 번 튄다(Panner 본래 특성).
- 0417: Mesh Paint 는 에디터 모드라 런타임에는 같은 데이터(정점 색)를 Procedural Mesh 에 마우스로 칠한다 — 머티리얼(VertexColor.R Lerp)은 Mesh Paint 결과와 동일하게 동작.
- 추가 shot: `shots/0408_half.png`(키 1 = 0.5 고정, 경계 Emissive), `shots/0410_pp.png`(키 2 = 포스트 프로세스 툰).
- 0410 포스트 프로세스 방식은 그림자 영역의 디더 노이즈까지 양자화돼 점무늬가 보인다(데모 비교 용도로 둠). 또 톤매핑 후 밝기 양자화(색조 유지)라 하늘도 계단이 진다 — 물체별 제어는 머티리얼 방식이 낫다는 비교용.

## 대체 구현

- 없음(모두 런타임 데모). 0417 의 "Mesh Paint 로 칠한" 부분만 런타임 마우스 브러시로 같은 정점 색 데이터를 만든다(머티리얼은 실제 VertexColor 노드).
