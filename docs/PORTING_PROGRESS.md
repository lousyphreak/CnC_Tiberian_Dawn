# Porting Progress

_Last updated: 2026-04-24_

## Goal

Port the Tiberian Dawn codebase to a reproducible cross-platform SDL3/CMake build that works on modern compilers and modern operating systems without relying on the legacy Watcom/Win32 toolchain.

## Current status

- Initial modern-build bring-up started (2026-04-24):
  - repo state at start:
    - this repository only contains `CODE/` and `TOOLS/`; it does not yet contain the shared `WIN32LIB` support layer or SDL compatibility wrappers that the Red Alert SDL3 port already uses;
    - there was no `docs/PORTING_PROGRESS.md` in-tree yet, and there was no top-level `CMakeLists.txt`;
    - the preserved Watcom build inventory still exists in `CODE/MAKEFILE`, and its `OBJECTS` list maps cleanly to in-tree sources (154 objects, 0 missing source files from that object list).
  - reference findings from the Red Alert SDL3 port:
    - Red Alert already builds through a top-level `CMakeLists.txt`, vendors SDL3 under `extern/SDL3`, and keeps the porting history in `docs/PORTING_PROGRESS.md` / `docs/PORTING_KNOWLEDGE.md`;
    - that tree confirms the general build shape we want here: bundled SDL3, top-level CMake, and deliberate compatibility layers instead of trying to restore the original toolchain.
  - completed in this checkpoint:
    - added SDL3 as a repo-owned git submodule at `extern/SDL3`, matching the Red Alert port's dependency strategy;
    - normalized local include casing across `CODE/`: `571` include directives were rewritten across `231` source/header files, and the in-repo include audit now reports `0` remaining local case mismatches;
    - added the first top-level `CMakeLists.txt` for Tiberian Dawn with bundled SDL3 wiring, modern compiler defaults, and optional ASan/UBSan flags.
  - first build result:
    - `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug` succeeds;
    - `cmake --build build --target tiberian-dawn --parallel` now starts a real build in this repository and reaches the first compiler errors instead of failing for missing build scaffolding;
    - the first blockers are the missing legacy platform/support headers that still need to be ported in-tree, starting with `windows.h`, `dos.h`, and `process.h` (`CODE/FUNCTION.H`, `CODE/ALLOC.CPP`, `CODE/CCDDE.CPP`, `CODE/DDE.CPP`, `CODE/CWSTUB.C`).
  - next concrete porting work exposed by this build:
    1. introduce the first SDL3/portable replacements for the Win32/DOS header surface instead of including raw platform headers directly;
    2. decide how to bring the missing WW support layer (`WWLIB32`, VQA headers, related compat code) into this repository cleanly;
    3. rerun the build after that first compatibility layer lands to expose the next compile/link blockers.
