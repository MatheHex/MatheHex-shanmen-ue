# Dev.D.UE.0.0.9B.P7.0.r0 Report

## Conclusion

`READY_FOR_P8_FUNCTIONAL_WITH_F_DEBT`

P7 implements the Code B active-Run personal-inventory vertical slice. It does not run or validate the game; all runtime evidence remains F-stage debt.

## Prompt and scope

- Archived Prompt: `Docs/Prompt/Dev.D.UE.0.0.9B.P7.0.r0_prompt.md`
- Prompt SHA-256: `0C65620BD5A8CEC964C16F2E9B57A9308C99BA4265850B52FA9A509E40E21E24`
- Scope: P6 committed session query, P3/P4 production presentation reuse, Run-only inventory visibility/input edge, and the P6 durable commit adapter.

## Changed files

| File | Responsibility |
| --- | --- |
| `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h/.cpp` | Exact active-session read facade; accepted P6 snapshot commit; frozen original carried-item receipt evidence so its digest stays immutable after P7 moves. |
| `Source/demo_map/CodeB/demo_mapCodeBP3.h/.cpp` | Adds an active-Run presentation scope. In that scope click/menu operation commands are rejected; P4 drag/drop remains the write route. |
| `Source/demo_map/CodeB/demo_mapCodeBP3UI.h/.cpp` | Reuses the mounted P3/P4 page as a Run personal bag, hides Warehouse, shows P6 layout containers, disables non-drop equipment guidance, and lets I/Esc manage only Host visibility/transient state. |
| `Source/demo_map/demo_mapV3ProgressionManager.h/.cpp` | Minimal existing-inventory-input edge: resolves current ProfileId + started RunId, opens only an exact P6 session, then owns only the Host lifecycle and in-memory P1 view. |
| `PROJECT.md`, `PROJECT_INFO_CARD.md` | P7 ownership, boundary, and F-debt status. |
| `Docs/Prompt/Dev.D.UE.0.0.9B.P7.0.r0_prompt.md` | Exact archived source Prompt. |

Not changed: P6 bridge/Prepared receipt/recovery/rebind code paths; P5 migration and out-of-raid product entry; Code A Start Run, map, Player Actor, combat, Loot/search, settlement, Run Save, old inventory, and PlayerController bindings.

## Functional boundary

1. Existing inventory input first gates on `ProfilePreparationFlow::RunActive`, a valid current `ProfileId`, and `Snapshot.ActiveRunId == GetStartedRunId()`.
2. `OpenMatchedActiveRunInventorySession` loads only an existing document, requires the store OwnerId, session OwnerId, committed state, and exact RunInstanceId. A miss creates no disk sidecar, P5 migration, fixture, new session, or Repository; the established Code A inventory behavior remains nonblocking.
3. Only after that read succeeds does P7 create the in-memory P1 Repository and bind the shared Host to the P6 snapshot/layout.
4. The active page displays equipment, Basic6, conditional QuickSpatial, PouchInternal, and disabled 1–9 placeholders; it intentionally omits the out-of-raid Warehouse.
5. The only accepted location-change route is mounted `NativeOnDrop → FCodeBP4InteractionController::CommitDrop → FCodeBP3UIController::CommitP4Operation → P2 ApplicationService → P1 Repository → CommitAcceptedActiveRunInventorySnapshot`.
6. Every accepted P1 result invokes one P6 snapshot commit. That commit changes only the active-session snapshot, increments active-session and record revisions once, and leaves the P5 snapshot and P6 bridge/rebind state untouched. The first such commit freezes the original carried item graph used by the immutable receipt digest.

## Static boundary audit

- Exact identity: accepted only for matching OwnerId + committed RunInstanceId; absent, stale, malformed, or mismatched sessions return before any Repository construction or write.
- P4x rule: active scope rejects legacy `BeginOperation`/click-command use, and its equip/unequip guidance is disabled. Double-click, right-click, details, empty cells, invalid drops, Cancel, I, Esc, close, and disabled 1–9 remain zero-write paths. `NativeOnDrop` is the only mounted UI call to P4 `CommitDrop`.
- P5 lock: P7 neither calls `OpenOrMigrate` nor `CommitAcceptedSnapshot`; the existing P5 active-run lock is unchanged.
- P6 invariants: P7 does not call or alter bridge/recovery/rebind APIs. It preserves receipt identity, origin RunId, moved-item IDs, digest, and recovery history; frozen evidence allows the immutable initial digest to survive legal in-Run position changes.
- Code A edge: only `Ademo_mapV3ProgressionManager` visibility/input lifecycle code changed. It does not change Code A Run activation, Player, HUD authority, map, Loot, search, settlement, Run Save, or old inventory writes.

## P-stage compile

Command:

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex
```

- Target: `demo_mapEditor Win64 Development`
- Final exit code: `0`
- Result: `Succeeded`
- Full source compile linked `UnrealEditor-demo_map.dll`; a final same-target freshness check also returned `Succeeded` with no actions pending.

## Deferred to 0.0.9B.F

No actual game launch, CTA/wrapper exercise, automated test, regression, screenshot, smoke, Game target build, Cook, Package, or final acceptance was run for P7. P1–P7 runtime integration and complete validation remain the sole responsibility of `0.0.9B.F`.
