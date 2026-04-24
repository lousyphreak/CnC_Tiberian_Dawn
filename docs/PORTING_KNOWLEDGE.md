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
