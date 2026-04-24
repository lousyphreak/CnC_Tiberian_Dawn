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
- DOS/Win32 wrapper pass completed and the build frontier moved again (2026-04-24):
  - completed in this checkpoint:
    - added the first shared DOS-style filesystem wrappers in `SDL3_COMPAT/wrappers/sdl_fs.*`:
      - `_makepath`
      - `_dos_findfirst` / `_dos_findnext`
      - `_dos_getdrive`
      - `_dos_getdiskfree`
      - `find_t` / `diskfree_t`
    - added the first missing Win32/compiler helper shims in `SDL3_COMPAT/wrappers/win32_compat.*`:
      - `MEMORYSTATUS` / `GlobalMemoryStatus`
      - `stricmp` / `strcmpi` / `strnicmp`
      - `strupr` / `strlwr`
      - `htons` / `ntohs`
      - `htonl` / `ntohl`
      - `WM_USER`
    - removed another batch of raw DOS/Win32-era headers from TD sources now covered by the wrapper layer:
      - `CODE/HEAP.CPP`
      - `CODE/GADGET.CPP`
      - `CODE/DEBUG.CPP`
      - `CODE/LOADDLG.CPP`
      - `CODE/MONOC.CPP`
      - `CODE/VECTOR.CPP`
      - `CODE/MIXFILE.CPP`
      - `CODE/STARTUP.CPP`
      - `CODE/INIT.CPP`
      - `CODE/CONQUER.CPP`
    - fixed one C++ namespace collision exposed by the newer headers in `CODE/PACKET.CPP` (`min` -> `std::min`).
  - current build result:
    - the support libraries still compile cleanly, and the game build now gets well past the old DOS helper failures;
    - the current full build no longer stops on `_makepath`, `_dos_findfirst`, `_dos_getdiskfree`, `GlobalMemoryStatus`, `stricmp`, or byte-order helper errors in the non-network code path;
    - the next hard failures are now dominated by the still-unported networking layer and stricter modern-C++ semantics elsewhere in the game sources.
  - current blocker groups exposed by the latest rebuild:
    1. the TCP/IP / Internet multiplayer layer is now the main portability wall:
       - `TCPIP.H` / `TCPIP.CPP` still require WinSock-style types and APIs such as `SOCKET`, `WSADATA`, `sockaddr`, `FD_READ`, `FD_WRITE`, `WSAAsyncSelect`, and `WSAAsyncGetHostByName`;
       - these errors propagate into `COMQUEUE.CPP` and any source that still includes `TCPIP.H`;
       - `WM_USER` is now available, but the build still needs a deliberate socket compatibility story rather than more ad-hoc typedefs.
    2. several modern-compiler correctness failures are now visible outside the networking code:
       - legacy `++` / `--` on `bool` members in files such as `BUILDING.CPP`, `FACTORY.CPP`, `AIRCRAFT.CPP`, and `INI.CPP`;
       - const-correctness and symbol-resolution problems such as `UNIT.CPP` writing through a `const` object and overloaded-name collisions in `ANIM.CPP` / `MAP.CPP`;
       - missing or renamed support-library APIs in a few spots (`INTERPAL.CPP` direct-draw checks, `GAMEDLG.CPP`, `EVENT.CPP`).
    3. `CODE/RAWFILE.CPP` is still not fully ported:
       - it now gets far enough to expose TD-specific symbol collisions and stale globals instead of only DOS API failures;
       - it still needs to be aligned fully with the imported `RawFileClass`/SDL I/O model.
  - next concrete porting work:
     1. triage the TCP/IP layer against the Red Alert reference and decide whether to add a Linux socket compatibility layer or stub/exclude multiplayer for the first playable build;
     2. fix the newly exposed modern-C++ correctness errors that are independent of networking and safe to modernize without changing gameplay;
     3. finish the `RAWFILE` port once the network/header churn stops dominating the build.
- Support-API cleanup continued and the build moved past the previous `INIT`/timer wall (2026-04-24):
  - completed in this checkpoint:
    - aligned the long-lived scenario-initialization flag with the Red Alert reference by restoring `ScenarioInit` to an `int` counter instead of a `bool`, which removes a large class of illegal `++` / `--` uses without changing the nested-init semantics;
    - added SDL/support-backed compatibility shims for more legacy assumptions:
      - `LoadLibrary` / `FreeLibrary`
      - `SetForegroundWindow`
      - `ShowWindow`
      - `_splitpath`
      - `randomize` declaration exposure
      - lightweight audio-state helpers (`Get_Digi_Handle`, `Get_Sample_Type`, `SampleType` compatibility macro)
    - fixed another batch of strict-modern-C++ issues in `CODE/INIT.CPP`:
      - removed temporary `CCFileClass` / `RawFileClass` rvalue bindings passed to `Load_Alloc_Data(...)`;
      - replaced `strrev` with `SDL_strrev`;
      - switched the cursor hide path to the SDL input layer;
      - updated VQA audio hookup from the old DirectSound field names/types to the imported SDL audio backend fields (`AudioObject`, `PrimaryBuffer`);
    - restored the missing global timer declaration/definition with the current support-layer `TimerClass`, so `TickCount` users compile again.
  - current build result:
    - the build now gets beyond the earlier `INIT.CPP`, `MENUS.CPP`, and `CONNECT.CPP` failures that came from missing `TickCount`, `SampleType`, `LoadLibrary`, `_splitpath`, and stale DirectSound/VQA names;
    - the next dominant failures are now the still-unported communications/networking area plus a fresh set of stricter C++ issues in unrelated gameplay/support files.
  - current blocker groups exposed by the latest rebuild:
    1. communications/networking code is still the main portability wall:
       - `TCPIP.H` is still the old WinSock header surface;
       - `COMQUEUE.CPP` now fails as a major downstream consumer;
       - `IPX95.H` still uses old Win32 calling-convention assumptions;
    2. some imported SDL/input support still needs TD-side alignment:
       - `CODE/SDLINPUT.CPP` currently conflicts with TD globals such as `GameInFocus`;
       - newer support-side options helpers do not yet line up with TD's `GameOptionsClass`;
    3. modern compiler strictness continues to expose old code patterns elsewhere:
       - unresolved overloaded-name collisions (`SPECIAL.CPP`, `THEME.CPP`, `LOADDLG.CPP`);
       - invalid legacy casts in `IOOBJ.CPP`;
       - stale symbol names such as `_Kbd`, `Get_Key_Num`, and `SoundType`.
  - next concrete porting work:
    1. port `TCPIP.H/.CPP` and related communications code toward the Red Alert `SOCKETS.H` model;
    2. reconcile the imported SDL input layer with TD globals/options naming;
    3. continue the modern-C++ cleanup where the build now points next (`COMQUEUE`, `IOOBJ`, `SPECIAL`, `LOADDLG`, `THEME`).
