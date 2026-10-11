#!/usr/bin/env bash
# 컴파일·레벨 빌드·캡처처럼 언리얼 프로세스를 띄우는 작업을 한 번에 하나만 실행한다.
# (에디터 프로세스가 모듈 DLL 을 잡고 있으면 다른 모듈의 링크가 실패하기 때문)
#   tools/locked.sh tools/build.sh GlossaryEffects
#   tools/locked.sh tools/glossary-build.sh -filter=05
#   tools/locked.sh powershell -ExecutionPolicy Bypass -File tools/capture-level.ps1 -Level ... -Out ...
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LOCK="$ROOT/.build-lock"
WAITED=0
until mkdir "$LOCK" 2>/dev/null; do
  # 30분 넘게 잡힌 잠금은 죽은 프로세스가 남긴 것으로 보고 푼다
  MTIME=$(stat -c %Y "$LOCK" 2>/dev/null || echo "")
  if [ -n "$MTIME" ] && [ $(( $(date +%s) - MTIME )) -gt 1800 ]; then
    rmdir "$LOCK" 2>/dev/null
    continue
  fi
  if [ $WAITED -eq 0 ]; then echo "[locked] 다른 빌드가 끝나기를 기다리는 중 ($(cat "$LOCK/owner" 2>/dev/null))"; fi
  WAITED=1
  sleep 5
done
echo "$* (pid $$)" > "$LOCK/owner"
trap 'rm -rf "$LOCK"' EXIT
"$@"
