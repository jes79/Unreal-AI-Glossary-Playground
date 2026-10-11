#!/usr/bin/env bash
# 생성 에셋 + 예제 레벨을 코드로 다시 만든다 (에디터를 닫은 상태에서).
#   tools/glossary-build.sh                전부
#   tools/glossary-build.sh -filter=05     05 분야만 (+ 공통 에셋, 허브)
#   tools/glossary-build.sh -filter=0101,0105
#   tools/glossary-build.sh -assetsonly / -levelsonly
set -euo pipefail
export MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL="*"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT="$ROOT/GlossaryPlayground/GlossaryPlayground.uproject"
UE="${UE_ROOT:-/c/Program Files/Epic Games/UE_5.8}"
"$UE/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "$(cygpath -w "$PROJECT")" -run=GlossaryBuild "$@" -unattended -nop4 -nosplash -stdout -FullStdOutLogOutput
