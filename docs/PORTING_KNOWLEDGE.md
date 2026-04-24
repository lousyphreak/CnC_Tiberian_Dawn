# Porting Knowledge

_Last updated: 2026-04-24_

- This repository starts from a much earlier state than the Red Alert SDL3 port:
  - there is no top-level CMake project yet;
  - there is no in-tree `WIN32LIB` or SDL compatibility layer yet;
  - the original Watcom `CODE/MAKEFILE` is currently the authoritative source inventory for the game-side objects.
- The preserved Watcom object list in `CODE/MAKEFILE` currently resolves to in-tree source files without gaps (`154` objects, `0` missing source files), so it is a reliable starting point for the first CMake source list.
- Linux case-sensitive builds will fail immediately unless include directives are normalized:
  - the initial audit found `553` local include-case mismatches in `CODE/`;
  - the first cleanup pass rewrote `571` include directives across `231` files and reduced the in-repo mismatch count to `0`;
  - these need direct fixes in the source files themselves, not symlinks or forwarding headers.
- The first standalone build will need an explicit answer for the missing shared support layer (`WWLIB32`, VQA headers, Win32 compatibility, etc.). The Red Alert port is the reference for how those layers were modernized, but Tiberian Dawn should stay standalone and not grow a hard build dependency on the sibling Red Alert checkout.
- The first top-level SDL3/CMake bring-up now configures successfully and starts compiling the game sources. The first hard compiler blockers are missing legacy platform headers, not CMake/scaffolding problems:
  - `windows.h` from `CODE/FUNCTION.H`, `CODE/CCDDE.CPP`, and `CODE/DDE.CPP`;
  - `dos.h` from `CODE/ALLOC.CPP`;
  - `process.h` from `CODE/CWSTUB.C`.
- `WIN32LIB` and `SDL3_COMPAT` can be imported from the Red Alert port as the first practical support layer for Tiberian Dawn, but they are not drop-in:
  - TD still needs game-side header adoption (`COMPAT.H`, `FUNCTION.H`, `REAL.H`, etc.) before the imported libraries can compile cleanly;
  - the imported support code expects at least `CODE/SDLINPUT.*` and `CODE/KEY.*` to exist in-tree.
- The current support-layer/CMake integration does work well enough to move the build dramatically forward:
  - bundled SDL3 compiles in-tree;
  - the imported `sdl3_compat` and `win32lib` targets compile;
  - the main game target now reaches roughly `92%` of the compile before hitting the next TD-specific portability blockers.
- A few TD/RA integration hazards are now confirmed:
  - old umbrella-header defines such as `RAWFILE_H` and `MONOC_H` can suppress TD declarations that the modernized build still needs;
  - some old TD headers still redefine C++ built-ins or standard operators (`bool`, placement `new`) and must be trimmed for modern compilers;
  - several TD classes still use `long`-based virtual function signatures that must be converted to explicit 32-bit types to match the imported support layer on Linux.
- The next blocker stack is no longer “missing support layer”; it is “legacy OS API surface still unported”:
  - DOS/Win32 file/path/system helpers like `_makepath`, `_dos_findfirst`, `_dos_getdiskfree`, and `GlobalMemoryStatus` are now the dominant compile failures in non-network code;
  - `CODE/RAWFILE.CPP` is still using the old DOS handle model and no longer matches the imported `RawFileClass` declaration surface;
  - `TCPIP.H` / `TCPIP.CPP` still assume WinSock/Win32 async APIs and are the main remaining multiplayer bring-up blocker on Linux.
- `CODE/TEMP.CPP` is not part of the preserved `CODE/MAKEFILE` object list and is malformed in-tree, so it should stay out of the modern build unless/until its provenance is understood.
