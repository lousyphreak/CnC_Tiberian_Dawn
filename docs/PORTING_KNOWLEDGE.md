# Porting Knowledge

_Last updated: 2026-04-26_

- Building prerequisite legality should stay close to the original split:
	- `HouseClass::Can_Build(...)` should use `ActiveBScan` for prerequisite legality;
	- deploy-specific/sidebar timing issues should be solved locally in the building deploy/unlimbo path rather than by redefining the meaning of `BScan` globally.
- The modern port still needs a safe fixed-width structure scan helper:
	- use `Structure_Scan_Bit(StructType)` for `BScan` / `ActiveBScan` updates instead of raw shifts on `StructType` values.
- The MCV -> construction-yard path needs an immediate local-player sidebar/buildable refresh in `BuildingClass::Unlimbo(...)`:
	- the construction yard must be revealed to the local player immediately;
	- local buildables should be updated/recalculated immediately for player-owned building factories.
- Player-owned building unlock state can still transiently miss strict discovery/lock gates during live deploy/build transitions:
	- a constrained local-player fallback in `LogicClass::AI()` for `NewBScan` / `NewActiveBScan` is safer than globally broadening all house scan semantics.
- Buildability/sidebar state must refresh at building-completion transitions, not only placement/deploy transitions:
	- force a local-player `Update_Buildables()` + `Map.Recalc()` refresh in `BuildingClass::Grand_Opening(...)` to keep progression unlocks (e.g. power -> barracks) in sync.
- Missing multiplayer/internet option state can silently collapse the tech tree:
	- `Read_Game_Options(...)` must not default `BuildLevel` to `0` when `Options/BuildLevel` is absent.
- Legacy WChat/DDE and modem/null-modem modules are intentionally removed from this SDL3 port baseline:
	- active runtime code should not include or depend on `CCDDE.*` or `NULL*` modules.
- Keep internet multiplayer role/timing state on neutral internet-specific symbols in active code:
	- role: `InternetGameRole`
	- timing: `InternetMaxAhead`, `InternetSendRate`
