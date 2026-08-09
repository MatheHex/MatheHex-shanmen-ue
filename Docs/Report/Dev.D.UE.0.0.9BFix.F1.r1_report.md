# Dev.D.UE.0.0.9BFix.F1.r1 Report

**Task:** `Dev.D.UE.0.0.9BFix.F1.r1`  
**Project:** `C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`  
**Result:** `BLOCKED`  
**Blocker:** `EXECUTION_HARNESS_UNAVAILABLE`

## Scope and preserved F1 result

This is a real-runtime acceptance rework, not a feature pass. No product source, config, or content was changed during F1.r1.

F1's narrow, retained correction remains in place:

- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp` gates P5 availability on `ReadyForPreparation`, rather than treating the historical terminal `ActiveRunId` tombstone as a live run.
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfileTests.cpp` retains the terminal-history / P5-entry regression assertion.
- `Source/demo_map/demo_map0909BFramework.cpp` retains read-only P5-entry audit logging.

This does not alter Code A combat, life, death, or terminal authority. It does not introduce a second inventory, Code A mirror, P5/P6/P8 double-write, fixture, starter, migration, or historical-route reconnect.

## Snapshot and immutable test baseline

Created complete recovery snapshot before F1.r1 runtime work:

`C:\AIDev\shanmen-ue\Snapshots\Dev.D.UE.0.0.9B_BFixF1r1_20260808_1701`

The snapshot contains `Source`, `Config`, `Content`, `Docs`, `demo_map.uproject`, and all `Saved\SaveGames` data. File counts and byte totals matched live data at creation:

| Area | Files | Bytes | Snapshot match |
|---|---:|---:|---|
| Source | 335 | 4,666,290 | yes |
| Config | 5 | 14,253 | yes |
| Content | 485 | 142,107,585 | yes |
| Docs | 73 | 828,277 | yes |
| Saved\\SaveGames | 4 | 24,323 | yes |

The project descriptor SHA-256 matched between live project and snapshot. The F1.r1 prompt was downloaded from Planner Chat, fully read, and archived as `Docs\Prompt\Dev.D.UE.0.0.9BFix.F1.r1_prompt.md`; its download/archive SHA-256 matched.

Baseline owner and real P5 graph:

- Owner/Profile: `b64b94f1-4b7f-6d3e-2f25-7796c570efdd`, Code A save generation `26`.
- Code A retains one permanent `TrainingBlade` root item `edf4eaf6-4e70-8331-ff90-478206b6a372`; PreparationLayout is empty.
- Code A has `HasActiveRun=false`, while retaining historical terminal `ActivationFailure` run `affc45fa-446c-9e8b-b47d-7489b31fb2d5` and settlement `f289aa6d-4188-8f47-53bc-81be045121f7`.
- Code B sidecar persistent revision is `2`; repository revision is `9`; P5 warehouse is `7d1b94f0-fb72-6d2f-2625-7789ca6eeff2` and contains the same root item at slot 0.
- P6 active inventory session is false; normal/body run-local container arrays and P8 terminal receipts are empty.

After the blocked harness attempts, live `Source`, `Config`, and `Content` each matched the snapshot byte-for-byte by per-file SHA-256 comparison. All four persisted save files also matched their baseline SHA-256 values. No recovery action was necessary.

## Required real UMG input: blocked before Scenario A

The required real window existed and was enumerated after the remote computer was unlocked:

- window: `demo_map （64-位 Development PCD3D_SM6）`
- application: UE 5.8 `UnrealEditor.exe`
- live window identifier reported by the desktop control surface: `985804`

The desktop control surface could enumerate and resolve that window, but every operation needed to inspect, focus, or send normal input to it failed immediately with the same external bridge error:

`node_repl exec context not found`

Observed failed operations:

1. Window-state capture with screenshot after resolving the live UE window.
2. Window focus activation for normal desktop input.
3. Fresh desktop-control session: window-state capture with screenshot.
4. Fresh desktop-control session: window-state capture without screenshot.

This is the prior F1 input-channel failure, not a game failure. No normal OS click, keyboard action, UMG hit test, widget callback, direct `StartM01Run` call, `OnClicked` call, reflection, console state force, save edit, fixture, hard-coded RuntimeReady/RunInstanceId, or P5/P6/P8 write was used as a substitute.

Accordingly, the following F1.r1 acceptance work was **not executed** and is not claimed:

- no-op UI navigation proof and actual Editor PIE/New Editor Window input;
- Editor-originated Standalone Game input;
- A: Warehouse/Loadout same-P5 projection, reversible P5 transaction, and UI-originated invalid rejection;
- B/C: repeated Submit Expedition input, unique StartAttemptId, M01 RuntimeReady/InRun, and one P5→P6 bridge;
- D: UI-originated correlated technical failure, same-session cleanup, and immediate reversible P5 transaction.

The existing F1 real Editor evidence remains available only as prior context:

- `Saved\Screenshots\BFixF1_AtSect_EditorRuntime_unlocked.png`
- `Saved\Logs\BFixF1_EditorRuntime_Final.log` (including the retained `0_0_9BFIX_WAREHOUSE Event=OpenForSect Ready=1` audit record)

It cannot replace the F1.r1 A–D evidence and is not counted as it.

## Build evidence

No F1.r1 source change was made, but both required native targets were compiled against the retained F1 correction using UE's bundled .NET 10 runtime:

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- Result: `Succeeded`
- Native exit code: `0`
- Log: `Saved\Logs\BFixF1r1_EditorBuild.log`

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- Result: `Succeeded`
- Native exit code: `0`
- Log: `Saved\Logs\BFixF1r1_GameBuild.log`

The uncooked `demo_map.exe` shader-library limitation was not retested and was not worked around with Cook, Package, or project changes, as required.

## Static boundary review

No test injection was added. The existing formal top-level path remains the F1/P1–P21 path: Framework → Coordinator → M01 Runtime Adapter → Code B Item Bridge → Sect Widget/Editor Support. Since no F1.r1 code was changed, there is no new StartAttempt creator, product `StartM01Run` caller, bridge observer, profile authority, or historical route to audit beyond the preserved F1 review.

## Required follow-up

Restore a working desktop-control bridge capable of screenshot/state capture, focus, and genuine mouse/keyboard delivery to the visible UE window. Then rerun F1.r1 from the saved baseline and collect the required real A–D evidence. Do not treat this report as F1 acceptance.

Deferred after genuine F1 completion: complete warehouse/equipment/spatial operations; ordinary container, corpse, and ground loot transfer; quick use; P8 terminal variants; recovery; full regression; Cook; Package; and final release acceptance.

