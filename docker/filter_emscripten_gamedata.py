#!/usr/bin/env python3

from __future__ import annotations

import argparse
import pathlib
import re
import shutil


LOCAL_USER_DATA_NAMES = {
    "hallfame.dat",
    "record.bin",
    "savegame.net",
    "assert.txt",
}

TOP_LEVEL_EXCLUDES = {
    "c&cnotes.ico",
    "covert_readme.txt",
    "covert_readme.wri",
    "ddrawcompat.ini",
    "ddrawcompat_license.txt",
    "game.dat",
    "installscript.vdf",
    "ipxemu license.txt",
    "ipxemu readme.txt",
    "readme.doc",
    "readme.wri",
    "readme.exe",
    "readme.txt",
    "steam_appid.txt",
    "steam_autocloud.vdf",
    "tuc_credits_updated.txt",
}

TOP_LEVEL_DIRECTORY_EXCLUDES = {
    "command & conquer",
    "manuals",
}

TOP_LEVEL_EXCLUDED_SUFFIXES = (
    ".386",
    ".dll",
    ".exe",
    ".icl",
    ".ico",
    ".isu",
)

SAVEGAME_RE = re.compile(r"savegame\.\d{3}\Z")
CAPTURE_RE = re.compile(r"cap\d{4}\.pcx\Z")


def normalize_manifest_path(raw_path: str) -> str:
    return raw_path.strip().replace("\\", "/").lstrip("/")


def should_include(relative_path: str) -> bool:
    folded_path = relative_path.lower()
    top_level_name = folded_path.split("/", 1)[0]

    if top_level_name == "conquer.ini":
        return True

    if top_level_name in TOP_LEVEL_DIRECTORY_EXCLUDES:
        return False

    if "/" not in folded_path:
        if top_level_name in LOCAL_USER_DATA_NAMES:
            return False
        if SAVEGAME_RE.fullmatch(top_level_name):
            return False
        if CAPTURE_RE.fullmatch(top_level_name):
            return False
        if top_level_name in TOP_LEVEL_EXCLUDES:
            return False
        if top_level_name.endswith(TOP_LEVEL_EXCLUDED_SUFFIXES):
            return False

    return True


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True, type=pathlib.Path)
    parser.add_argument("--source-root", required=True, type=pathlib.Path)
    parser.add_argument("--output-root", required=True, type=pathlib.Path)
    args = parser.parse_args()

    manifest_lines = args.manifest.read_text(encoding="utf-8").splitlines()

    if args.output_root.exists():
        shutil.rmtree(args.output_root)
    args.output_root.mkdir(parents=True, exist_ok=True)

    for line in manifest_lines:
        if not line.strip():
            continue

        relative_path = normalize_manifest_path(line)
        if not should_include(relative_path):
            continue

        source_path = args.source_root / pathlib.Path(relative_path)
        if not source_path.is_file():
            raise FileNotFoundError(f"manifest entry not found in GameData: {relative_path}")

        destination_path = args.output_root / pathlib.Path(relative_path)
        destination_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source_path, destination_path)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
