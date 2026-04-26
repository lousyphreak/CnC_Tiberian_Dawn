# Porting Progress

_Last updated: 2026-04-26_

## Current checkpoint

- Multiplayer naming cleanup pass (2026-04-26):
	- completed in this checkpoint:
		- performed a broad symbol rename sweep to remove legacy `MPlayer*` identifiers from active runtime globals and helper routines, replacing them with `Multiplayer*` names (examples: player identity, colors, score tracking, option fields, and ID helper conversions).
		- updated key call sites in gameplay/network code (`INIT.CPP`, `NETDLG.CPP`, `INTERNET.CPP`, `HOUSE.CPP`, and related headers) to use renamed symbols consistently.
		- fixed rename-collision regressions introduced during the sweep (notably malformed `Build_MultiplayerID(...)` arguments in `NETDLG.CPP`/`INTERNET.CPP`).
		- refreshed several stale comments that still referenced removed WChat/modem paths in edited files.
	- why this mattered:
		- it reduces legacy naming debt and makes the active SDL3 multiplayer path clearer by separating preserved gameplay networking from removed historical backends.
	- validation result:
		- `cmake --build build --target tiberian-dawn --parallel 4` succeeds after the rename/cleanup pass.

- Legacy networking cleanup pass removed WChat / NullModem / Modem code paths from the active TD port (2026-04-26):
	- completed in this checkpoint:
		- removed active legacy include dependencies (`CCDDE`) from runtime files and common headers.
		- removed legacy null-modem modules from the tree:
			- `CODE/NULLCONN.CPP`
			- `CODE/NULLCONN.H`
			- `CODE/NULLDLG.CPP`
			- `CODE/NULLMGR.CPP`
			- `CODE/NULLMGR.H`
			- `CODE/NULLSTUB.CPP`
		- removed legacy DDE/WChat modules from the tree:
			- `CODE/CCDDE.CPP`
			- `CODE/CCDDE.H`
		- replaced old role/timing symbols with internet-specific names in active paths:
			- `ModemGameToPlay` -> `InternetGameRole`
			- `WChatMaxAhead` -> `InternetMaxAhead`
			- `WChatSendRate` -> `InternetSendRate`
		- rewrote `CODE/INTERNET.CPP` to keep only internet/TCP lobby functionality used by the SDL3 port and dropped legacy external-chat spawn/packet paths.
		- simplified `Read_MultiPlayer_Settings(...)` / `Write_MultiPlayer_Settings(...)` in `CODE/MPLAYER.CPP` by removing serial phonebook/init-string persistence tied to removed modem systems.
	- why this mattered:
		- these legacy subsystems were no longer in the supported SDL3 multiplayer path, but they still leaked stale symbols/types/headers into active runtime code.
		- removing them reduces maintenance noise and avoids accidental coupling to dead connection flows.
	- validation result:
		- `cmake --build build --target tiberian-dawn --parallel 4` succeeds after the removals.

- Post-power unlock follow-up: force completion-time sidebar/buildability refresh (2026-04-26):
	- completed in this checkpoint:
		- kept construction-yard deploy fixes intact, then added a targeted post-completion refresh in `BuildingClass::Grand_Opening(...)` for player-owned structures.
		- when a building finishes, player-owned building factories now immediately refresh buildables and trigger `Map.Recalc()`.
	- why this mattered:
		- the reported chain (`MCV -> conyard -> power`, then no new buildables) indicates that construction completion could leave sidebar/buildability state stale even though prerequisites were now satisfied.
		- anchoring the refresh at completion (`Grand_Opening`) guarantees unlock updates at the exact transition where new prerequisites become valid.
	- validation result:
		- `cmake --build build --target tiberian-dawn -- -j$(nproc)` succeeds.
		- `cmake --build build-asan --target tiberian-dawn -- -j$(nproc)` succeeds.
		- 20s ASan smoke run launches and times out as expected (`timeout`), with only pre-existing leak reports in startup paths.

- Original-tag unlock follow-up added a constrained local-player scan fallback for post-power unlock progression (2026-04-26):
	- completed in this checkpoint:
		- continued deep comparison with `original` after confirming that MCV -> construction yard and construction yard -> power were working, but power -> barracks still failed in runtime testing.
		- kept global unlock semantics close to original (`Can_Build` still keys off `ActiveBScan`), but added a narrow runtime fallback in `LogicClass::AI()`:
			- if a prerequisite-capable building belongs to `PlayerPtr` and is not in limbo, it now contributes to `NewBScan` / `NewActiveBScan` even when discovery/lock gating is transient.
		- this fallback is intentionally local-player-only and does not broaden AI/enemy scan semantics.
	- why this mattered:
		- the SDL runtime still shows transient player-building scan-state gaps during deploy/build transitions on this port, which can suppress expected immediate unlock progression (the reported `power -> barracks` gap).
		- keeping the correction local-player-scoped avoids reintroducing the earlier broad global drift while still making sidebar progression robust.
	- validation result:
		- `cmake --build build --target tiberian-dawn -- -j$(nproc)` succeeds.
		- `cmake --build build-asan --target tiberian-dawn -- -j$(nproc)` succeeds.

- Building/unlock audit against the `original` tag narrowed the active regressions to a small set of targeted fixes (2026-04-26):
	- completed in this checkpoint:
		- compared the live building/unlock paths against `original`, with focus on:
			- `CODE/BUILDING.CPP`
			- `CODE/HOUSE.CPP`
			- `CODE/LOGIC.CPP`
			- `CODE/SIDEBAR.CPP`
			- `CODE/INTERNET.CPP`
		- restored original global scan-mask semantics after confirming the broader drift was causing systemic building-state mismatch:
			- `HouseClass::Can_Build(...)` again uses `ActiveBScan` for prerequisite legality instead of broadening legality through `BScan`
			- `LogicClass::AI()` again populates building scan masks using the original `IsLocked` / human-visibility gate, while still using safe fixed-width structure-bit generation
		- kept the targeted SDL/Linux-port fixes that were still needed to make MCV deployment behave correctly in the modern runtime:
			- `BuildingClass::Unlimbo(...)` now uses the safe `Structure_Scan_Bit(...)` helper
			- player-owned deployed building factories immediately reveal/update/recalc so MCV -> construction-yard unlocks appear at placement time
			- `BuildingClass::Update_Buildables()` no longer blocks local-player factories on a stale `IsDiscoveredByPlayer` flag during deploy/place transitions
		- found and fixed an independent systemic unlock blocker in `CODE/INTERNET.CPP`:
			- `Read_Game_Options(...)` was defaulting `BuildLevel` to `0` when the option was missing
			- that collapses the tech tree to the exact reported symptom pattern: construction yard can place power, but subsequent tech does not unlock normally
			- the loader now preserves the current/default build level and clamps it to at least `1`
	- why this mattered:
		- the earlier scan-mask widening made `BScan` / `ActiveBScan` mean something different from the original game across the whole building and sidebar pipeline, which risks unlock, sell, capture, and special-weapon state bugs far beyond the original MCV symptom
		- the missing-INI `BuildLevel = 0` path explains a separate “power plant is the last thing I can build” failure mode that survives even after the construction-yard refresh path is corrected
	- validation result:
		- `cmake --build build --target tiberian-dawn -- -j$(nproc)` succeeds
		- `cmake --build build-asan --target tiberian-dawn -- -j$(nproc)` succeeds
	- remaining follow-up:
		- runtime-verify the full unlock chain from a restored baseline:
			- MCV -> construction yard
			- power -> refinery / barracks
			- refinery -> radar / airstrip / storage
			- later barracks/power-dependent tech branches
		- keep an eye on any remaining building lifetime/runtime sanitizer issues separately from unlock-state behavior
