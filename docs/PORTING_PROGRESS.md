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
- Support-layer import and Linux compiler bring-up continued (2026-04-24):
  - completed in this checkpoint:
    - imported the Red Alert-derived support trees into this repository:
      - `WIN32LIB/`
      - `SDL3_COMPAT/`
      - `CODE/SDLINPUT.H`
      - `CODE/SDLINPUT.CPP`
      - `CODE/KEY.H`
      - `CODE/KEY.CPP`
    - extended the top-level `CMakeLists.txt` so the build now compiles:
      - bundled SDL3 from `extern/SDL3`
      - `sdl3_compat` as an in-tree static library
      - `win32lib` as an in-tree static library
      - the main `tiberian-dawn` executable linked against both support libraries plus SDL3 and threads;
    - excluded a first batch of clearly obsolete or not-yet-ported legacy sources from the initial bring-up path:
      - `ALLOC.CPP`
      - `CWSTUB.C`
      - `CCDDE.CPP`
      - `DDE.CPP`
      - `DPMI.CPP`
      - `IPX.CPP`
      - `IPXMGR.CPP`
      - `NULLCONN.CPP`
      - `NULLDLG.CPP`
      - `NULLMGR.CPP`
      - `TEMP.CPP` (this file is malformed in-tree and is not part of the preserved Watcom object list);
    - converted the first TD umbrella/header chokepoints away from raw Win32/DOS includes and into the imported compatibility layer:
      - `CODE/COMPAT.H`
      - `CODE/FUNCTION.H`
      - `CODE/REAL.H`
      - `CODE/FIELD.H`
      - `CODE/CDFILE.H`
      - `CODE/RAWFILE.CPP`
    - fixed several integration mismatches exposed by GCC/Linux:
      - removed old `bool` redefinitions when compiling as modern C++;
      - removed the obsolete local placement-`new` operators in `CODE/VECTOR.H`;
      - aligned `CCFileClass` and `AircraftClass` signatures with the imported support-layer interfaces;
      - exposed missing keyboard/class shims through `CODE/JSHELL.H`;
      - added initial Win32 calling-convention/type aliases to `SDL3_COMPAT/wrappers/win32_compat.h`.
  - current build result:
    - `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug` still succeeds;
    - `cmake --build build --target tiberian-dawn --parallel` now gets through SDL3, the imported support libraries, and most of the game target;
    - the current compile reaches roughly `92%` before stopping on deeper TD portability issues.
  - current blocker groups exposed by the latest rebuild:
    1. legacy DOS/Win32 file/path/system helpers are still missing modern replacements in TD:
       - `_makepath`
       - `MEMORYSTATUS` / `GlobalMemoryStatus`
       - `_dos_findfirst` / `_dos_findnext`
       - `_dos_getdrive` / `_dos_getdiskfree`
       - `direct.h` / `mem.h` era assumptions still present in individual sources;
    2. `CODE/RAWFILE.CPP` is still structurally behind the imported `WIN32LIB` `RawFileClass` interface:
       - old `long`-based method signatures conflict with the imported `int32_t` declarations;
       - DOS file APIs (`_dos_open`, `_dos_read`, `_dos_write`, etc.) still need proper wrapper-backed replacements;
       - some TD-local globals and mode constants in that source still do not line up with the new support path;
    3. the TCP/IP / Internet multiplayer layer is still effectively unported on Linux:
       - `TCPIP.H` / `TCPIP.CPP` and related files still expect WinSock types and async APIs such as `SOCKET`, `WSADATA`, `WM_USER`, and `WSAAsync*`;
       - these errors now fan out into `COMQUEUE.CPP`, `SEQCONN.CPP`, `INTERNET.CPP`, `NETDLG.CPP`, and any core source that still includes `TCPIP.H`.
  - next concrete porting work:
    1. add proper compatibility wrappers for the remaining DOS-era file/path/system calls that are still used widely in TD code;
    2. port `CODE/RAWFILE.CPP` to the imported wrapper/`int32_t` model instead of the old DOS-handle implementation;
    3. decide whether the first playable bring-up should temporarily stub or exclude multiplayer/TCPIP code paths, or whether to add a first Linux socket-compat layer now.
