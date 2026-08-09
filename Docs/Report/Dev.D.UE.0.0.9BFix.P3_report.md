# Dev.D.UE.0.0.9BFix.P3 Report

## Result

`READY_FOR_0_0_9BFIX_P4_OR_F_PLANNING`

P3 completes the sole formal M01 activation chain inside the existing 0.0.9B
project. It does not start the product or validate the chain at runtime.

## Scope and files

- Snapshot before implementation: `C:\AIDev\shanmen-ue\Snapshots\Dev.D.UE.0.0.9B_FixP3_20260808_1453`.
- Prompt archived: `Docs\Prompt\Dev.D.UE.0.0.9BFix.P3_prompt.md`.
- Added the correlated runtime receipt vocabulary and expanded readonly start
  diagnostics in `demo_map0909BFrameworkTypes.h`.
- Reworked `demo_map0909BM01RuntimeAdapter.{h,cpp}` into the only narrow
  activation adapter: formal M01 descriptor/config audit, one pending attempt,
  activation acceptance, correlated RuntimeReady validation, stale/duplicate
  event rejection, and matching-attempt cleanup.
- Reworked `demo_map0909BRunStartCoordinator.{h,cpp}` to own the sole
  `StartAttemptId`, monotonic attempt sequence, immutable P2 selection fields,
  state changes, receipt recording, RuntimeReady gate, and idempotent
  TechnicalStartFailure return.
- Extended `demo_map0909BCodeBItemBridge.{h,cpp}` only with StartAttemptId
  audit correlation; the existing GameMode-to-Code-B bridge contract remains
  the one P5-to-P6 call.
- Updated `demo_map0909BFramework.cpp`, `demo_map0909BSectWidget.cpp`, and
  `demo_map0909BEditorSupport.{h,cpp}` for state-only presentation and readonly
  default-entry/attempt diagnostics.
- Updated `PROJECT.md` and `PROJECT_INFO_CARD.md` for P3.
- Unchanged: `demo_mapGameMode.{h,cpp}`, `demo_mapPlayerController.{h,cpp}`
  (snapshot SHA-256 comparison), all map/content/config assets, Code B P5/P6/P8
  stores, combat, health, terminal/settlement, inventory authority, Loot, HUD,
  Actor, Player and Run Save code. No old path was deleted or re-enabled.

## StartAttempt and M01 adapter contract

- Only `Ademo_map0909BFrameworkHost::RequestStartM01FromUI` calls
  `StartM01Run`; only the Coordinator generates `StartAttemptId`.
- The immutable diagnostic records attempt sequence/time, OwnerId,
  P5/graph revision, selection digest, expected descriptor, live map identity,
  runtime receipt sequence/class, and the active or released RunInstanceId.
- Adapter receipts are keyed by StartAttemptId and contain descriptor/world,
  GameMode/WorldSettings, Controller/Pawn, input, OwnerId and RunInstanceId
  facts. It accepts only one pending attempt.
- `BeginActivation` verifies the P2 selection and current Profile owner,
  requests the retained authored M01 materialization seam, and reports only
  `ActivationRequestAccepted`. It does not claim RuntimeReady.
- After the Coordinator restores GameOnly input, `ConfirmRuntimeReady`
  revalidates the exact pending attempt, M01 World/identity, GameMode,
  WorldSettings, controller, pawn, input surface, OwnerId and active RunId.
  Stale, duplicate and mismatched callbacks are classified and ignored.
- No timer, watchdog, global pending flag or unbounded retry was introduced.

## Success and failure boundaries

- Only a correlated `RuntimeReady` in `ActivatingWorld` can transition the
  Coordinator to `InRun`; only after that transition does the existing
  selection-aware P5-to-P6 observer run.
- The bridge still re-reads durable P5 and validates exact ItemId roots,
  spatial closure, revisions and digest. A post-world bridge refusal remains an
  audit diagnostic and never changes the confirmed Code A Run into a technical
  failure.
- Before RuntimeReady, any descriptor/config/world/authority/player/input or
  correlation failure enters `TechnicalStartFailure`. The Adapter releases only
  the matching transient attempt and invokes the existing reversible prepared
  Code A rollback only if that attempt created one.
- The Coordinator then verifies that no P6 active session exists, clears the
  active RunId (retaining it only as released diagnostic evidence), returns to
  `AtSect`, and reports that the warehouse was not changed. It creates no P8
  receipt or terminal classification.
- P5 items are not moved, reserved, cloned or locked before world confirmation.
  Warehouse service retains its explicit `StartAttemptPending` write rejection
  during PreparingStart/ActivatingWorld and returns to normal AtSect handling
  after cleanup.

## Static audit

- Start route scan: `StartM01Run` has one product caller (the new Framework
  Host); the Coordinator is the only StartAttemptId creator.
- Adapter scan: `Prepare0909BRun`, `Activate0909BM01World` and
  `Rollback0909BPreparedRun` occur only inside the P3 adapter in the new
  framework path. `ObserveConfirmedActivation` follows the explicit
  `RuntimeReady->InRun` transition in the Coordinator.
- P5/P6/P8 scan found no direct P6/P8 write in Adapter/Coordinator; P3 only
  invokes the established observer and post-failure no-P6 verifier.
- Old-route scan over Framework/Coordinator/Adapter/SectWidget found zero
  references to `Open0909BOutOfRaidInventory`,
  `OpenProfilePreparationFromSect`, `StartPreparedProfileRunFromSect`,
  `GetSectNavigationWidget`, `Teleport`, `V2`, or `V3`.
- Editor Support is readonly: it audits formal map config/default entry and
  StartAttempt evidence; it has no P5/P6/P8 writer, attempt creator or forced
  success path.
- Code A static boundary: GameMode and PlayerController header/source hashes
  are identical to the P3 snapshot. No map, Actor, input, HUD, combat, health,
  Run, Run Save, settlement, inventory or Loot authority changed.

## Compilation

1. `"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`
   - Native exit code: `0`; UBT: `Result: Succeeded`.
2. `"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`
   - Native exit code: `0`; UBT: `Result: Succeeded`.

## Deferred to 0.0.9B.F

No product launch, CTA, PIE, map activation, screenshot, automation, smoke,
regression, gameplay, Cook, Package or final acceptance was run. F still owns
real sect start, warehouse/loadout manipulation, empty P5, duplicate clicks,
M01 success, each technical failure, P5-to-P6 success and bridge refusal, all
P8 terminals, recovery, screenshots and final verification.

No early-version rule was adopted; no project, parallel Profile, inventory,
Run preview, fixture, ItemId clone or Code A mirror was created; no P1-P21 or
Fix.P1/P2 result was discarded. P4/F has not been started.
