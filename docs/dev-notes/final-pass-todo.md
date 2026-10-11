# 마지막 통합 단계에서 처리할 공통 수정 (분야 에이전트 보고에서 모음)
- GlossaryEnvironment: ExposureEV100 주석이 반대 의미(값이 클수록 밝아짐 = 노출 보정 값-10) → 주석/이름 정리
- GlossaryEnvironment: Sky=Indoor 일 때 SkyLight 실시간 캡처 경고(빨간 글씨) → Indoor 에서는 RealTimeCapture 끄고 큐브맵/강도만
- 0512·0514 화면 구석 빨간 디버그 글씨 출처 확인
- 공통 카메라 오프셋 옵션(3인칭 캐릭터가 화면 중앙을 가림) 검토 — 11 분야 요청
- shot.sh 연속 키 3개째 반영 안 되는 경우 확인 (0807 One,Two,Three 에서 Three 미반영)
- 0802 레벨 재로드 후 GameInstanceSubsystem 유지 로그로 확인
- 0214: 복사한 SKM_SportsCar 가 렌더링되지 않음(물리는 정상) → 에디터에서 확인. 지금은 SM 차체를 소켓에 붙여 보이게 함
- 0904/0807 등: shot 연속 키 여러 개 중 일부만 반영 — 3프레임 홀드 수정 후에도 재확인 필요
- 1003: r.Nanite.Visualize 를 -game 에서 켜면 화면이 검게 나옴 (에디터 뷰포트에서는 확인 필요)
