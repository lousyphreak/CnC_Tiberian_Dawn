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
- Communications support cleanup continued and the build moved past the old WChat/registry wall on Linux (2026-04-24):
  - completed in this checkpoint:
    - exposed the legacy keyboard globals/helpers that TD gameplay code still expects by restoring the declarations in `CODE/KEY.H` (`_Kbd`, `Check_Key`, `Get_Key`, `Get_Key_Num`, `Check_Key_Num`, `Clear_KeyBuffer`, `KN_To_VK`, `Key_Down`);
    - added a non-Windows compatibility path for `CODE/CCDDE.H`, so DDE/WChat symbols are visible on Linux even though the old Windows DDE transport itself remains disabled there;
    - resolved the `COMQUEUE.H` / `COMBUF.H` typedef collision by renaming the queue entry types inside `COMQUEUE` to unique names and fixing the stale `MaxPacketLen` reference while touching that code;
    - added a tiny `CODE/commlib.h` forward declaration shim so the legacy null-modem header can be parsed again while the full serial/null-modem implementation stays out of the current Linux build;
    - stubbed the old registry/launcher-dependent WChat helpers in `CODE/INTERNET.CPP` for non-Windows builds:
      - `Is_User_WChat_Registered(...)`
      - `Spawn_WChat(...)`
      - `Spawn_Registration_App(...)`
      - the INI fallback `HWND` lookup no longer tries to call `FindWindow(...)` on Linux.
    - aligned one imported SDL input declaration with TD globals by switching the local `SDLINPUT.CPP` `GameInFocus` declaration back to `bool`.
  - current build result:
    - the build no longer dies in the DDE/registry/WChat compatibility area;
    - the next hard failures are now mostly strict modern-C++ correctness issues plus a smaller remaining set of legacy Win32 surface gaps outside the internet stack.
  - current blocker groups exposed by the latest rebuild:
    1. old C-era identifier collisions and loop-scope assumptions are now the most visible compiler failures:
       - `index` collisions in `EXPAND.CPP`, `CELL.CPP`, `SPECIAL.CPP`, `SCENARIO.CPP`, and related sources;
       - loop variables escaping scope in `INI.CPP`, `LOADDLG.CPP`, `DISPLAY.CPP`, `MAPSEL.CPP`, and similar files;
       - a few remaining bool/constructor strictness issues (`ENDING.CPP`, countdown timer overload selection, etc.).
    2. a smaller batch of support/API mismatches still remains:
       - DirectDraw-only checks or helpers still referenced from `RADAR.CPP`, `INTERPAL.CPP`, `DISPLAY.CPP`, and `CONQUER.CPP`;
       - missing Win32 constants/messages in `WINSTUB.CPP`, `NETDLG.CPP`, and file-open code in `CONQUER.CPP`;
       - a few TD/import mismatches such as `SoundType`, `memicmp`, and some `JSHELL.CPP` type issues.
    3. some platform-obsolete subsystems are now intentionally compiled only as compatibility shells on Linux:
       - DDE/WChat launch/registration helpers;
       - legacy null-modem serial support.
  - next concrete porting work:
    1. rename or rescope the old `index` loop variables and similar overloaded-name collisions in the gameplay code the compiler now reaches first;
    2. trim or wrap the remaining DirectDraw/Win32-only surface area still referenced from UI/support files;
    3. keep documenting any subsystem that is being preserved as a non-Windows stub rather than reimplemented.
- Modern compiler cleanup continued and the build moved past the old file/mix/direct-draw/savecode wall (2026-04-24):
  - completed in this checkpoint:
    - extended `SDL3_COMPAT/wrappers/win32_compat.h` with the Win32 file-handle constants that TD still uses through the imported wrapper layer:
      - `GENERIC_READ`
      - `GENERIC_WRITE`
      - `FILE_SHARE_READ`
      - `FILE_SHARE_WRITE`
      - `CREATE_ALWAYS`
      - `OPEN_EXISTING`
      - `OPEN_ALWAYS`
      - `FILE_ATTRIBUTE_NORMAL`
      - `INVALID_HANDLE_VALUE`
    - fixed more 32-bit-sensitive file and mixfile code:
      - `CODE/CONQUER.CPP` now passes `DWORD`-sized fields to `GetVolumeInformation(...)` instead of Linux-sized `unsigned long*`;
      - `CODE/CCFILE.CPP` now bridges the old `MixFileClass::Offset(..., long*, long*)` API through local `long` temporaries before storing the results back into TD's explicit `int32_t` members.
    - removed another wave of strict modern-C++ failures caused by loop-scope leakage and template deduction mismatches:
      - `CODE/CELL.CPP`
      - `CODE/EXPAND.CPP`
      - `CODE/HOUSE.CPP`
      - `CODE/ENDING.CPP`
      - `CODE/INTRO.CPP`
    - replaced missing DirectDraw-only viewport helpers with SDL/software-safe equivalents:
      - `CODE/CONQUER.CPP` now falls back to a tiled software fill for `CC_Texture_Fill(...)` instead of calling the missing `Texture_Fill_Rect(...)`;
      - `CODE/DISPLAY.CPP`
      - `CODE/RADAR.CPP`
      - `CODE/GSCREEN.CPP`
      - `CODE/INTERPAL.CPP`
      now use `Get_IsVideoSurface()` in the same places that the Red Alert SDL port no longer uses `Get_IsDirectDraw()`.
    - removed another stale Win32/audio-era symbol check in `CODE/GAMEDLG.CPP` by following the Red Alert path and opening the sound-controls dialog unconditionally.
    - fixed the first 64-bit save/load decode hazards in `CODE/IOOBJ.CPP` by decoding saved pointer-sized enum IDs through `reinterpret_cast<uintptr_t>(...)` before converting them back to TD enum values.
  - current build result:
    - the support libraries still build cleanly;
    - the game build now gets past the previous `CONQUER`, `CCFILE`, `CELL`, `ENDING`, `EXPAND`, `GAMEDLG`, `HOUSE`, `INTRO`, and first `IOOBJ` decode failures;
    - the current compile frontier is now in later UI/gameplay files such as `CODE/JSHELL.CPP` and `CODE/LAYER.CPP`.
  - current blocker groups exposed by the latest rebuild:
    1. some remaining imported-support mismatches are still surfacing in later UI code:
       - `CODE/JSHELL.CPP` is indexing into imported icon metadata with stale assumptions about the support structure layout;
       - more imported UI helpers may still need TD-side alignment as those files compile.
    2. classic old-C++ identifier/scope issues still remain deeper in the gameplay code:
       - `CODE/LAYER.CPP` is now the next visible `index` collision site;
       - more files in that family are likely still waiting behind it.
    3. save/load modernization is not finished yet:
       - the first `IOOBJ.CPP` pointer-to-enum decode fixes are in, but more serialization code should be reviewed with the same 32-bit/64-bit care as the build keeps moving.
  - next concrete porting work:
     1. fix `CODE/JSHELL.CPP` against the imported support-layer icon structure used by the SDL/Win32LIB path;
     2. continue the local `index`/scope cleanup in `CODE/LAYER.CPP` and whichever gameplay files appear next;
     3. keep checking save/load decode code for pointer-sized assumptions as more of `IOOBJ.CPP` and adjacent serialization files come into view.
- Startup/window bring-up and missing helper restoration pushed the build to the final link stage (2026-04-25):
  - completed in this checkpoint:
    - fixed another long run of strict modern-C++ scope/constness failures in gameplay code:
      - `CODE/SEQCONN.CPP`
      - `CODE/NOSEQCON.CPP`
      - `CODE/STATS.CPP`
      - `CODE/STARTUP.CPP`
      - `CODE/TARGET.CPP`
      - `CODE/TECHNO.CPP`
      - `CODE/THEME.CPP`
      - `CODE/UNIT.CPP`
    - restored the packet/field string interface to accept modern const data, following the Red Alert porting direction instead of casting string literals through mutable pointers:
      - `CODE/PACKET.H`
      - `CODE/PACKET.CPP`
      - `CODE/FIELD.H`
      - `CODE/FIELD.CPP`
    - removed another stale startup-only option/config dependency that no longer exists in the SDL path:
      - dropped the dead `AllowHardwareBlitFills` read from `CODE/STARTUP.CPP`;
      - moved the stats hardware/build-date collection in `CODE/STATS.CPP` from DirectDraw/Win32 file handles to SDL-backed RAM/video/path info.
    - replaced the old Win32 entry/window shell with the first SDL-safe startup path:
      - `CODE/STARTUP.CPP` now enters through `main(int, char**)` instead of `WinMain(...)`;
      - `CODE/WINSTUB.CPP` now uses the SDL input pump and `RA_CreateWindow` / `RA_DestroyWindow` path for focus loss, window creation, and shutdown instead of raw Win32 messages;
      - shutdown/memory-error handling no longer depends on `PostMessage`, `PostQuitMessage`, `ExitProcess`, or other missing Win32 message-loop APIs.
    - restored several missing helper/operator/template definitions that the original Watcom/ASM build used implicitly:
      - bitwise enum helpers in `CODE/DEFINES.H`, `CODE/JSHELL.H`, and `CODE/GADGET.H`;
      - portable `Bound`, `Cardinal_To_Fixed`, and `Fixed_To_Cardinal` helpers in `CODE/JSHELL.H`;
      - inline `Coord_Cell(...)` in `CODE/FUNCTION.H` / `CODE/REAL.H`;
      - first explicit template instantiations in `CODE/VECTOR.CPP` for types that now reach link.
    - added a temporary SDL-port bring-up stub for screen shake in `CODE/WINSTUB.CPP` so missing legacy support code no longer blocks the build.
  - current build result:
    - the project now compiles every translation unit in the `tiberian-dawn` target and reaches the final executable link step;
    - the previous source-level blockers in `SEQCONN`, `NOSEQCON`, `STATS`, `STARTUP`, `TARGET`, `TECHNO`, `THEME`, `UNIT`, and `WINSTUB` are cleared;
    - the current failure mode is now unresolved-link symbols rather than C++ compile errors.
  - current blocker groups exposed by the latest rebuild:
    1. excluded or still-unported legacy multiplayer backends are now the biggest linker gap:
       - `IPXManagerClass::*` methods from the intentionally excluded `IPX.CPP` / `IPXMGR.CPP` path;
       - `Destroy_Null_Connection(...)`, `Reconnect_Modem()`, and `Shutdown_Modem()` from the still-disabled null-modem path;
       - `Calculate_CRC(...)` still needs to come from the preserved support code or a modernized replacement.
    2. more old template bodies are still missing concrete instantiations now that the linker sees the whole program:
       - `DynamicVectorClass<NodeNameTag*>`
       - `DynamicVectorClass<ObjectClass*>`
       - `TFixedIHeapClass<...>::Save(...)` for several save/load heaps.
    3. a smaller set of preserved legacy helpers still needs non-ASM/non-Win32 implementations or wiring:
       - `strtrim`
       - `Fat_Put_Pixel`
       - any remaining fixed-point / heap / utility routines that were formerly satisfied by omitted assembly or Watcom-era object files.
  - next concrete porting work:
    1. decide which excluded legacy backends should be stubbed for first-playable Linux bring-up versus ported properly now (`IPX`, null-modem, CRC helpers);
    2. continue the explicit template-instantiation or header-definition cleanup for vector/heap/save-load templates now that link-time gaps are visible;
    3. replace the last assembly-era utility holdouts (`strtrim`, `Fat_Put_Pixel`, related helpers) with SDL/C++ implementations.
- Helper/instantiation follow-up kept the project at the final-link frontier while shrinking the remaining gap set (2026-04-25):
  - completed in this checkpoint:
    - added new C++ support translation units to replace several missing asm-era or reference-only sources:
      - `CODE/FACE.CPP`
      - `CODE/KEYFBUFF.CPP`
      - `CODE/READLINE.CPP`
      - `CODE/PORTSUPP.CPP`
      - `CODE/NULLSTUB.CPP`
    - restored more concrete template output so TD’s monolithic build keeps emitting code under modern C++:
      - expanded `CODE/VECTOR.CPP` explicit instantiations for `BaseNodeClass`, `ObjectClass*`, `NodeNameTag*`, `FileEntryClass*`, `PhoneEntryClass*`, `char`, `char*`, `char const*`, `int`, and `void*`;
      - expanded `CODE/HEAP.CPP` explicit `TFixedIHeapClass<...>` instantiations for the active object/save heap types TD actually uses at link.
    - added or aligned several compatibility definitions that were previously only available through missing asm/object files:
      - `Calculate_CRC(...)` in `CODE/INIT.CPP`
      - `Set_Buffer_Size(...)` in `CODE/RAWFILE.H` / `CODE/RAWFILE.CPP`
      - `TrackControlType` and `EditStyle` bitwise operators in `CODE/DRIVE.H` / `CODE/EDIT.H`
      - `ShapeBuffer` call sites in `CODE/CONQUER.CPP` moved to `_ShapeBuffer`
    - tried re-enabling the real TD IPX sources (`CODE/IPX.CPP`, `CODE/IPXMGR.CPP`) in the build so network code can link against the preserved implementation instead of only stubs.
  - current build result:
    - the target now recompiles essentially the entire game plus the new helper sources and still reaches the final executable link step;
    - build failures are now concentrated in a smaller set of ABI/legacy-support glue points instead of broad compile-frontier issues.
  - current blocker groups exposed by the latest rebuild:
    1. keyboard/input ABI is still split between TD’s `CODE/KEY.*` expectations and the imported support layer:
       - `Keyboard` global wiring and `WWKeyboardClass` method signatures still need to be normalized cleanly;
       - SDL input callback hooks are now clearly identified (`Keyboard_Handle_*`, focus/close handlers, bootstrap focus state).
    2. a last set of old asm support routines still needs C++ replacements or wiring:
       - `Distance_Coord`, `Set_Bit`, `First_False_Bit`, `ModeX_Blit`
       - palette/interpolation/MMX helpers
       - `LCW_Uncompress`, `Get_EAX`, and a few remaining legacy codec helpers
    3. obsolete UI/network entry points still need either SDL replacements or temporary documented stubs:
       - `Select_Serial_Dialog()`
       - `Com_Scenario_Dialog()`
       - `Com_Show_Scenario_Dialog()`
       - some IPX95 wrapper exports that are still not provided by the current support import.
