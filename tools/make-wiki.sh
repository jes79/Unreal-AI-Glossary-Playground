#!/usr/bin/env bash
# docs/wiki/ 의 분야별 페이지를 카탈로그(GlossaryCatalogData.inl)와 분야 노트(docs/dev-notes/NN-*.md)의 표로 다시 만든다.
#   tools/make-wiki.sh
# Home·시작하기 등 손으로 쓴 페이지는 건드리지 않는다.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT/GlossaryPlayground/Source"
OUT="$ROOT/docs/wiki"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$OUT"

# id \t name \t term \t plain \t request
grep 'GLOSSARY_ENTRY' "$SRC/GlossaryCore/Private/GlossaryCatalogData.inl" \
  | sed -e 's/^GLOSSARY_ENTRY("//' -e 's/")$//' -e 's/", "/\t/g' -e 's/\\"/"/g' > "$TMP/catalog.tsv"

# id \t impl \t showcase
cat "$ROOT"/docs/dev-notes/[01][0-9]-*.md \
  | grep -E '^\| *[0-9]{4} *\|' \
  | awk -F'|' '{ for (i = 2; i <= 4; i++) gsub(/^ +| +$/, "", $i); print $2 "\t" $3 "\t" $4 }' > "$TMP/notes.tsv"

# 코드 · 페이지 이름 · 제목 · 폴더 · 모듈 · 소개
cat > "$TMP/categories.tsv" <<'EOF'
01	01-캐릭터-이동·조작·카메라	캐릭터 이동·조작·카메라	01_Movement	GlossaryMovement	'손맛'은 대부분 이 분야에서 나옵니다. 점프가 답답하면 코요테 타임과 점프 버퍼링부터 요청하세요.
02	02-물리·충돌	물리·충돌 (Chaos)	02_Physics	GlossaryPhysics	충돌 채널·Hit/Overlap·Chaos 파괴까지. "안 부딪혀요"는 대부분 콜리전 프리셋 문제입니다.
03	03-애니메이션	애니메이션	03_Animation	GlossaryAnimation	Animation Blueprint·Blend Space·Montage·Notify. 마네킹(Manny)과 엔진 템플릿 애니메이션으로 시연합니다.
04	04-머티리얼·셰이더	머티리얼·셰이더	04_Material	GlossaryMaterial	모든 머티리얼은 UMaterialEditingLibrary 로 코드에서 노드를 이어 만든 실제 에셋입니다.
05	05-이펙트	이펙트 (Niagara·포스트 프로세스)	05_Effects	GlossaryEffects	Niagara 시스템도 코드로 만들었습니다(이미터·모듈 스택 조립). 만드는 법은 docs/dev-notes/niagara-authoring.md 참고.
06	06-UI·UX	UI·UX (UMG)	06_UI	GlossaryUI	UMG 위젯 트리를 C++ 에서 만듭니다. Widget Blueprint 가 필요한 용어는 같은 구조를 코드로 보여 줍니다.
07	07-적-AI·길찾기	적 AI·길찾기	07_AI	GlossaryAI	AIController·NavMesh·Behavior Tree·StateTree·EQS·Perception. BT·StateTree 에셋도 코드로 생성합니다.
08	08-게임-시스템·프레임워크	게임 시스템·프레임워크	08_Systems	GlossarySystems	GameMode·GameState·PlayerState·SaveGame·GAS 등 게임의 뼈대가 되는 클래스들입니다.
09	09-오디오	오디오	09_Audio	GlossaryAudio	Sound Cue·MetaSound·Attenuation·Submix. 사운드는 코드로 합성한 WAV 를 임포트해 씁니다.
10	10-조명·렌더링·최적화	조명·렌더링·최적화	10_Rendering	GlossaryRendering	Lumen·Nanite·Virtual Shadow Map·HISM·PCG. 대부분 ON/OFF 와 stat 수치로 차이를 보여 줍니다.
11	11-코드-구조·블루프린트·에디터	코드 구조·블루프린트·에디터	11_Patterns	GlossaryPatterns	UPROPERTY·컴포넌트·델리게이트·인터페이스·서브시스템, 그리고 에디터 도구(Editor Utility·Python·Live Coding).
EOF

normalize_impl() {
  case "$1" in
    런타임*) echo "런타임 데모" ;;
    에디터*) echo "에디터 도구 데모" ;;
    *) echo "설명 + 대체 구현" ;;
  esac
}

while IFS=$'\t' read -r CODE PAGE TITLE FOLDER MODULE INTRO; do
  FILE="$OUT/$PAGE.md"
  COUNT=$(grep -c "^$CODE" "$TMP/catalog.tsv")
  {
    echo "# $CODE $TITLE"
    echo
    echo "$INTRO"
    echo
    echo "레벨 ${COUNT}개 · 폴더 \`/Game/Levels/$FOLDER\` · 모듈 \`Source/$MODULE\`"
    echo
    echo "| 번호 | 용어 | 쉽게 말하면 | 레벨에서 보여 주는 것 | 구현 방식 |"
    echo "| --- | --- | --- | --- | --- |"
    while IFS=$'\t' read -r ID NAME TERM PLAIN REQUEST; do
      IFS=$'\t' read -r _ IMPL SHOW < <(grep "^$ID	" "$TMP/notes.tsv" || printf '%s\t\t\n' "$ID")
      echo "| $ID | [$TERM](#$ID) | $PLAIN | $SHOW | $(normalize_impl "$IMPL") |"
    done < <(grep "^$CODE" "$TMP/catalog.tsv")
    echo
    echo "## 항목별 상세"
    while IFS=$'\t' read -r ID NAME TERM PLAIN REQUEST; do
      IFS=$'\t' read -r _ IMPL SHOW < <(grep "^$ID	" "$TMP/notes.tsv" || printf '%s\t\t\n' "$ID")
      HEADER=$(cd "$SRC" && ls "$MODULE/Public/Demo${ID}_"*.h 2>/dev/null | head -1 || true)
      echo
      echo "### $ID"
      echo
      echo "**$TERM** — $PLAIN"
      echo
      echo "> 요청 예문: \"$REQUEST\""
      echo
      echo "- 레벨: \`/Game/Levels/$FOLDER/L_${ID}_$NAME\`"
      echo "- 데모 클래스: \`ADemo${ID}_$NAME\`${HEADER:+ (\`Source/$HEADER\`)}"
      echo "- 구현 방식: $IMPL"
      echo "- 레벨에서 보여 주는 것: $SHOW"
    done < <(grep "^$CODE" "$TMP/catalog.tsv")
  } > "$FILE"
  echo "$PAGE.md ($COUNT)"
done < "$TMP/categories.tsv"
