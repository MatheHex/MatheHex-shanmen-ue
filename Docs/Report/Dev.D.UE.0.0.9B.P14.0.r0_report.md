# Dev.D.UE.0.0.9B.P14.0.r0 Report

## Result

`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`

P14 implements one voluntary, whole-instance in-run ground-drop and return
path. P6 is its only durable item truth. Code A supplies only a legal floor
placement and a disposable visible projection; neither the actor nor its
failure changes the committed P6 record.

## Implemented scope

- P6 session schema advances `3 → 4` only. P5 record schema, P5 snapshot,
  Profile lock, bridge receipt and Code A Run ownership remain unchanged.
  Older valid P6 documents read as an empty P14 `WorldDrops` section with
  `NextWorldDropOrdinal = 1` and serialize schema 4 on their next accepted
  Owner-record save.
- A ground record contains only deterministic `WorldDropId`, derived P1
  single-slot world-container id, one existing `ItemId`, map route,
  floor-resolved transform and `Available` state. The deterministic ordinal is
  consumed only in the same successful save that adds its record; rejected
  requests create no record, actor or ordinal advance.
- `DropMatchedActiveRunBaseQuickItem` accepts only an exact committed
  Owner/Run P6 item that is positive-quantity, simple (no child graph), and
  currently in `BasicContainerId`. It creates the forced P1 world container,
  performs a whole-stack P1 `Move`, appends the P14 record, reconciles P13
  bindings, validates P1/P6 and saves one Owner document. It rejects equipment,
  spatial, partial, split, merge and swap routes.
- `CommitAcceptedMatchedRunWorldDropPickup` accepts exactly the P1 candidate
  produced by moving that one ground item to an empty P6 BaseQuick slot. It
  rejects alternate graph changes, merges, swaps, splits, stale revisions and
  nonmatching identities; then removes the empty world root and matching P14
  record in the same P6 save. Pickup remains unbound by P13 reconciliation.
- P8 first derives a player-only P6 graph. `Extracted` returns only that graph
  to P5 and freezes only that graph in the P8 receipt; all P14 world roots,
  items and records are discarded on `Extracted`, `Dead` and
  `RecoveredAbandon` exactly with the closing P6 session.
- The active P3/P4 Host now exposes one visible `UCodeBP3GroundDropZone`; its
  only write event is `NativeOnDrop`. The zone invokes the manager/store-owned
  callback, and no click, key, detail, context, close, cancellation, outside
  or no-payload path writes P14 data. A transient `WorldDropTarget` second
  column opens from the actor and allows only the ground source to move into an
  empty BaseQuick destination by actual `NativeOnDrop`.
- New `Ademo_mapCodeBWorldDropActor` is a projection adapter with exact
  Owner/Run/drop identity, existing focus/range interaction and a visible
  prompt. The manager resolves a downward floor hit before the P6 drop write,
  spawns/refreshes actor projections after a successful save, and removes them
  after P8 terminal closure. Actor destruction removes only its local
  projection; it never rolls back P6.

## Boundary audit

- Code A still owns map, pawn, focus, input, Run lifecycle and terminal
  classification. It neither creates P14 items nor holds a second inventory,
  quantity, definition, world-drop id or persistence record.
- P1 remains the only mutable item graph. P6 contains the sole durable ground
  relationship; P3/P4 and the actor carry only transient projection/drag data.
- No random loot, auto pickup, direct actor pickup, button pickup, use,
  consumption, numerical key binding, hidden target state, P9/P10/P11/P12
  semantic change or Code A/Code B mirroring was introduced.

## Files

- Modified `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h/.cpp` — P14
  schema, migration, validation, deterministic world records, atomic drop and
  pickup services, P13 reconciliation and P8 player-only extraction.
- Modified `Source/demo_map/CodeB/demo_mapCodeBP3UI.h/.cpp` — explicit native
  ground drop zone and transient P14 target-column restrictions.
- Added `Source/demo_map/demo_mapCodeBWorldDropActor.h/.cpp` — disposable Code
  A projection actor only.
- Modified `Source/demo_map/demo_mapV3ProgressionManager.h/.cpp` — floor
  adapter, actor lifecycle and exact P6/P3/P4 handoff.
- Archived `Docs/Prompt/Dev.D.UE.0.0.9B.P14.0.r0_prompt.md`; updated project
  tracking documents and added this Report.

## Minimal P-stage verification

The requested final Editor target was compiled after the P14 source additions:

```text
C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat demo_mapEditor Win64 Development C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject -WaitMutex
Result: Succeeded
Native exit code: 0
```

No game launch, CTA, wrapper, automation, regression, screenshot, Smoke, Game
build, Cook, Package or functional test was run. Real drop/retrieve,
multi-profile, interrupted persistence, actor destruction and terminal-path
validation remain explicit `0.0.9B.F` debt.
