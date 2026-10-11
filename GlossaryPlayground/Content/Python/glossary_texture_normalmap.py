# -*- coding: utf-8 -*-
"""
1111 Python 에디터 스크립팅 예제 — 노멀맵 텍스처 압축 설정 일괄 변경

폴더 안의 모든 Texture2D 를 찾아, 이름이 노멀맵 규칙(_N 으로 끝나거나 'normal' 포함)에 맞으면
  compression_settings = TC_NORMALMAP, srgb = False, lod_group = TEXTUREGROUP_WORLD_NORMAL_MAP
으로 바꾸고 저장한다. 이미 바뀐 텍스처는 건너뛴다(여러 번 실행해도 안전).

실행 방법 (에디터에서, Python Editor Script Plugin 이 켜져 있어야 한다)
  1) Output Log 아래 입력칸 왼쪽을 'Cmd' 로 두고
         py glossary_texture_normalmap.py                      ← 기본 폴더
         py glossary_texture_normalmap.py /Game/Textures       ← 다른 폴더 (요청 예문의 Content/Textures)
         py glossary_texture_normalmap.py /Game/Textures --dry  ← 바꾸지 않고 목록만
  2) 입력칸을 'Python' 으로 바꾸고
         import glossary_texture_normalmap as g; g.run("/Game/Textures")
  3) File > Execute Python Script... 에서 이 파일 선택

Content/Python 폴더는 엔진이 시작할 때 자동으로 sys.path 에 추가하므로 파일 이름만으로 찾을 수 있다.
"""

import sys

import unreal

# 요청 예문은 Content/Textures(=/Game/Textures). 이 프로젝트의 예제 텍스처는 빌더가 아래 폴더에 만든다.
DEFAULT_FOLDER = "/Game/Glossary/11_Patterns/Textures"


def is_normal_map(asset_name):
    """노멀맵 이름 규칙. 레벨의 C++ 미리보기(ADemo1111::IsNormalMapName)와 같은 규칙."""
    return asset_name.endswith("_N") or "normal" in asset_name.lower()


def run(folder=DEFAULT_FOLDER, dry_run=False):
    """folder 아래(하위 폴더 포함) 텍스처를 검사하고 바꾼 개수를 돌려준다."""
    asset_paths = unreal.EditorAssetLibrary.list_assets(folder, recursive=True, include_folder=False)
    changed, skipped, others = [], [], []

    # 진행 표시줄 (큰 프로젝트에서 수천 개를 돌 때 에디터가 멈춘 것처럼 보이지 않게)
    with unreal.ScopedSlowTask(len(asset_paths), "노멀맵 압축 설정 검사 중...") as task:
        task.make_dialog(True)
        for path in asset_paths:
            if task.should_cancel():
                break
            task.enter_progress_frame(1, path)

            asset = unreal.EditorAssetLibrary.load_asset(path)
            if not isinstance(asset, unreal.Texture2D):
                continue

            name = asset.get_name()
            if not is_normal_map(name):
                others.append(name)
                continue

            if asset.get_editor_property("compression_settings") == unreal.TextureCompressionSettings.TC_NORMALMAP:
                skipped.append(name)
                continue

            if not dry_run:
                # set_editor_property 는 에디터에서 값을 바꾼 것처럼 PostEditChange 를 불러 텍스처를 다시 압축한다
                asset.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
                asset.set_editor_property("srgb", False)
                asset.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD_NORMAL_MAP)
                unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
            changed.append(name)

    verb = "바꿀 예정" if dry_run else "변경"
    unreal.log("[glossary_texture_normalmap] 폴더 {}: {} {}개 {}".format(folder, verb, len(changed), changed))
    unreal.log("[glossary_texture_normalmap] 이미 Normalmap {}개 {}, 노멀맵 아님 {}개 {}".format(len(skipped), skipped, len(others), others))
    return len(changed)


if __name__ == "__main__":
    # py glossary_texture_normalmap.py [폴더] [--dry]
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    run(args[0] if args else DEFAULT_FOLDER, dry_run="--dry" in sys.argv)
