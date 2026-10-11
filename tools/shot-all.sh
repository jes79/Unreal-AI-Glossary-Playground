#!/usr/bin/env bash
# 허브 + 모든 예제 레벨을 차례로 실행해 shots/all/{id}.png 를 찍고, 실패·로그 오류를 shots/all/summary.txt 에 모은다.
#   tools/shot-all.sh [초=8] [분야필터 예: 05]
set -uo pipefail
export MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL="*"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DELAY="${1:-8}"; FILTER="${2:-}"
OUT="$ROOT/shots/all"; mkdir -p "$OUT"
SUMMARY="$OUT/summary.txt"; : > "$SUMMARY"
IDS=$(grep -o 'GLOSSARY_ENTRY("[0-9]*"' "$ROOT/GlossaryPlayground/Source/GlossaryCore/Private/GlossaryCatalogData.inl" | grep -o '[0-9]\{4\}')
for ID in hub $IDS; do
  [ -n "$FILTER" ] && [ "$ID" != "hub" ] && [[ "$ID" != $FILTER* ]] && continue
  RES=$("$ROOT/tools/locked.sh" "$ROOT/tools/shot.sh" "$ID" "$DELAY" 2>&1)
  if [ -f "$ROOT/shots/$ID.png" ] && echo "$RES" | grep -q "^shot="; then
    cp "$ROOT/shots/$ID.png" "$OUT/$ID.png"; STATUS=OK
  else
    STATUS=FAIL
  fi
  ERRS=$(echo "$RES" | grep -E "Error:|Fatal|Ensure" | grep -v "CommonUI without\|Video memory" | head -3 | tr '\n' ' ')
  echo "$ID $STATUS ${ERRS}" >> "$SUMMARY"
done
echo "done: $(grep -c ' OK' "$SUMMARY") OK / $(grep -c ' FAIL' "$SUMMARY") FAIL" | tee -a "$SUMMARY"
