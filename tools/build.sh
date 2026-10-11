#!/usr/bin/env bash
# 에디터 타깃 C++ 빌드. 사용: tools/build.sh [모듈이름]
#   tools/build.sh                 전체 (GlossaryPlaygroundEditor)
#   tools/build.sh GlossaryEffects 해당 모듈만 컴파일 (다른 모듈 오류와 분리해서 확인할 때)
# UBT 는 동시에 하나만 돌 수 있다. "conflicting instance" 가 나오면 잠시 뒤 다시 실행.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT="$ROOT/GlossaryPlayground/GlossaryPlayground.uproject"
UE="${UE_ROOT:-/c/Program Files/Epic Games/UE_5.8}"
ARGS=(GlossaryPlaygroundEditor Win64 Development "-Project=$(cygpath -w "$PROJECT")" -WaitMutex -NoHotReloadFromIDE)
if [ $# -ge 1 ]; then
  ARGS+=("-Module=$1")
fi
"$UE/Engine/Build/BatchFiles/Build.bat" "${ARGS[@]}"
