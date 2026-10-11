#!/usr/bin/env bash
# 예제 레벨을 게임 모드로 실행해 몇 초 뒤 자동 스크린샷을 찍고 종료한다(게임 안에서 찍으므로 화면 배율·창 위치와 무관).
#   tools/shot.sh 0101                       → shots/0101.png (기본 6초 후)
#   tools/shot.sh 0105 8 SpaceBar,C          → 8초 후, 그 전에 SpaceBar·C 를 눌러 본다 (FKey 이름)
#   tools/shot.sh hub                        → 허브
# 결과 PNG 경로와 게임 로그의 Error/Warning(Glossary·Script 관련)을 출력한다.
set -uo pipefail
export MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL="*"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT="$ROOT/GlossaryPlayground/GlossaryPlayground.uproject"
UE="${UE_ROOT:-/c/Program Files/Epic Games/UE_5.8}"
ID="$1"; DELAY="${2:-6}"; KEYS="${3:-}"
if [ "$ID" = "hub" ]; then
  LEVEL="/Game/Levels/L_0000_Hub"
else
  LEVEL=$(ls "$ROOT"/GlossaryPlayground/Content/Levels/*/L_"$ID"_*.umap 2>/dev/null | head -1)
  [ -z "$LEVEL" ] && { echo "레벨 없음: $ID (먼저 tools/glossary-build.sh -filter=$ID)"; exit 1; }
  LEVEL="/Game/Levels/$(basename "$(dirname "$LEVEL")")/$(basename "$LEVEL" .umap)"
fi
mkdir -p "$ROOT/shots"
OUT="$ROOT/shots/$ID.png"
LOG="$ROOT/GlossaryPlayground/Saved/Logs/Shot_$ID.log"
rm -f "$OUT"
ARGS=("$(cygpath -w "$PROJECT")" "$LEVEL" -game -windowed -ResX=1600 -ResY=900 -nosplash -unattended
      "-abslog=$(cygpath -w "$LOG")" "-GlossaryShot=$(cygpath -w "$OUT")" "-GlossaryShotDelay=$DELAY")
[ -n "$KEYS" ] && ARGS+=("-GlossaryShotKeys=$KEYS")
timeout $((DELAY + 120)) "$UE/Engine/Binaries/Win64/UnrealEditor.exe" "${ARGS[@]}"
if [ -f "$OUT" ]; then echo "shot=$OUT"; else echo "스크린샷 실패 (크래시?) log=$LOG"; fi
grep -E "Error:|Fatal|Ensure|LogScript: Warning|Glossary.*Warning" "$LOG" 2>/dev/null | grep -v "LogInit\|Failed to load '.*\.dll'" | head -20
true
