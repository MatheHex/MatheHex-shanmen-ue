# Dev.D.UE.0.0.9BFix.F1 Report

## Result

`NEEDS_0_0_9BFIX_F1_REWORK`

F1 reproduced and fixed the reported post-return P5 lock: a terminal
`ActivationFailure` tombstone retains its historical `ActiveRunId`, while the
P5 gate incorrectly treated that history as a live Run.  The fixed default
Editor runtime now opens the owner-matched P5 graph at `AtSect` with that
terminal RunId still present.  The actual UI input bridge failed before B--D
could be exercised, and the Development Game executable cannot start without
cooked Global shaders.  No success, duplicate-click, or technical-failure
result has been fabricated.

## Safety snapshot and baseline

- Snapshot: `C:\AIDev\shanmen-ue\Snapshots\Dev.D.UE.0.0.9B_BFixF1_20260808_1525`.
  It covers `Source`, `Config`, `Content`, `Docs`, `demo_map.uproject`,
  `Saved\SaveGames`, and `Saved\P4xStartRun`.
- Owner: `b64b94f1-4b7f-6d3e-2f25-7796c570efdd`.
- Baseline `Profile_Default.json`: generation `26`, one permanent
  `TrainingBlade`, empty equipment selections, no active run, historical
  `ActivationFailure` RunId `affc45fa-446c-9e8b-b47d-7489b31fb2d5`, and last
  settlement `f289aa6d-4188-8f47-53bc-81be045121f7`.
- The authoritative Code A Profile SHA-256 before and after the F1 runtime is
  unchanged: `8CB1E1A253E931E54DD4DF9056F66256F44073B2D0520BED120CD6309A97C494`.
  `Saved\P4xStartRun\Normal` is also byte-identical to the snapshot.
- The prior gate prevented first durable P5 enrollment, so the successful
  fixed startup created the one required owner-matched P5 sidecar:
  `Saved\SaveGames\Shanmen\CodeBOutOfRaid\b64b94f1-4b7f-6d3e-2f25-7796c570efdd.json`.
  It has one mapping of the original TrainingBlade to the same ItemId,
  persistent revision `2`, no active P6 session, and zero Code B terminal/P8
  receipts.  No item was reset, copied, or assigned a second parent.

## F1 repair

1. `Source\demo_map\CodeB\demo_mapCodeBOutOfRaidProfile.cpp`
   - Reproduction: `OpenOrMigrate` rejected a snapshot whenever
     `ActiveRunId.IsValid()`, including the valid terminal tombstone above.
     The visible result was `AtSect` plus “结束当前 Run 后再整理”, even though
     the Profile session was `ReadyForPreparation` and P6 was inactive.
   - Root cause: terminal identity and liveness were conflated at the P5
     entry gate.  The persistent Profile model intentionally retains the
     terminal RunId/settlement as first-event-wins history.
   - Fix: P5 now treats `ReadyForPreparation` as the authoritative liveness
     gate.  It keeps the terminal tombstone read-only rather than deleting or
     rewriting it; genuine `RunActive` snapshots remain rejected.
2. `Source\demo_map\CodeB\demo_mapCodeBOutOfRaidProfileTests.cpp`
   - Added a narrow regression assertion: ReadyForPreparation plus terminal
     Run history opens P5; RunActive still rejects with zero sidecar write.
3. `Source\demo_map\demo_map0909BFramework.cpp`
   - Added one read-only `0_0_9BFIX_WAREHOUSE Event=OpenForSect` audit log for
     the real host decision.  It does not create attempts, write P5/P6/P8, or
     change Code A combat/health/death/settlement authority.

Only those three files differ from the F1 snapshot.  The repair changes no
map, config, Code A inventory mirror, second inventory, item definition,
combat, world, P6 bridge, or P8 settlement implementation.

## Real execution evidence

### A. Default startup / recovered P5 gate

- Pre-fix real Editor Game run: the default entry loaded M01 and displayed
  `AtSect`, but the P5 warning blocked organization because the historical
  terminal RunId was considered active.  Real screenshot:
  `Saved\Screenshots\BFixF1_AtSect_EditorRuntime_unlocked.png`.
- Post-fix real Editor Game run, 2026-08-08 20:46:25:
  `Saved\Logs\BFixF1_EditorRuntime_Final.log:1576-1578` records
  `PROFILE_NORMAL_STARTUP`, then
  `0_0_9BFIX_WAREHOUSE Event=OpenForSect Ready=1 CoordinatorState=0
  SessionState=1 ActiveRunId=affc45fa-446c-9e8b-b47d-7489b31fb2d5
  LastTerminalReason=5` and an owner-matched P5 projection validation.
  The same log records `DefaultEntryValidation Valid=1`, default M01 map,
  RuntimeM01=1, RuntimeReady=1, and the new GameMode/Adapter route.
- No P5 drag/equip transaction was performed, because the available Windows
  input controller returned `node_repl exec context not found` for the real
  game window.  Therefore no reversible-item operation or invalid-selection
  UI submission is claimed.

### B. Duplicate submit

Not executed.  The real UI controller could enumerate the running window but
could not capture/click it after unlock; no `StartAttemptId` was created and
no duplicate submit was simulated or substituted with a fake call.

### C. M01 success / P5-to-P6 bridge

Not executed.  There is no real `StartAttemptId`, `RunInstanceId`, `InRun`
transition, or P5-to-P6 bridge result in this report.  Static route review
confirms the Coordinator is the sole `FGuid::NewGuid()` StartAttempt creator,
the Framework Host is the sole product caller of `StartM01Run`, and the sole
bridge observer remains after `RuntimeReady->InRun` in the Coordinator.

### D. Technical failure / immediate return

Not executed as a new live attempt, for the same real input-controller
failure.  The F1 reproduction used the existing historical technical return:
the Profile was already `ReadyForPreparation`, P6 inactive, and P5 was the
only broken gate.  The repair re-opened P5 without mutating that historical
record.  No controlled injection, fake RuntimeReady, forced Run identity,
manual save edit, P6 cleanup, or P8 write was used.

## Game target execution

- `Binaries\Win64\demo_map.exe` was launched after the final build.
- It initializes D3D12 on the actual RTX 4090 and then exits before map/UI
  startup because the uncooked development executable cannot find the Global
  ShaderCodeLibrary.  Evidence:
  `Saved\Logs\BFixF1_GameRuntime_Final.stderr.log` / runtime stdout capture,
  `Failed to initialize ShaderCodeLibrary ... Global shader library is
  missing`.
- Cook/Package is explicitly deferred by F1, so this was recorded rather than
  repaired through an out-of-scope packaging change.

## Compilation and static boundary audit

Both mandated `Build.bat` commands were run with UE 5.8's bundled .NET 10
host and exited natively with `0`:

1. `Build.bat demo_mapEditor Win64 Development demo_map.uproject -WaitMutex -NoHotReload`
   - `Saved\Logs\BFixF1_EditorBuild_Batch.log`: `Result: Succeeded`, native
     exit `0`.
2. `Build.bat demo_map Win64 Development demo_map.uproject -WaitMutex -NoHotReload`
   - `Saved\Logs\BFixF1_GameBuild_Batch.log`: `Result: Succeeded`, native
     exit `0`.

Static scans found no retired route names in Framework/Coordinator/Adapter/
Bridge/SectWidget.  There is one StartAttempt creator, one product
`StartM01Run` call, and one bridge observer call.  No temporary injection,
second Profile, Code A mirror, P5/P6/P8 dual writer, or historical page route
was added.

## Remaining F1 work and deferred F scope

F1 rework must complete the real UI-input portions of A--D once the Windows
controller is available: reversible P5 operation, invalid selection, fast
duplicate submit, correlated M01 success, one P5-to-P6 bridge, and a real
correlated technical failure returning to immediately usable P5.  It must
also capture the required live pending/InRun/failure screenshots.

Still deferred to later F work: full warehouse/equipment/spatial operations,
containers/corpses/ground transfer, quick-use, all P8 outcomes, recovery,
Cook, Package, and full regression/final acceptance.

No project was created; no legacy rule/path was reconnected; no P1--P21 item,
container, world, or inventory result was reset or discarded.
