# Dev.D.UE.0.0.9B.P8.0.r0 Report

## Result

`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`

P8 closes a matching, committed Code B P6 Run session only after Code A has already committed its terminal result. No product, CTA, automated test, regression, screenshot, smoke run, Game build, Cook, Package, or other F-stage verification was run.

## Files

Modified:

- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h/.cpp` — added P8 terminal classifications/results, terminal receipt history, schema-v1-to-v2 read compatibility, receipt JSON, frozen graph/layout digest validation, exact-session finalization, idempotence/conflict gates, and one-record atomic P5/P6 resolution.
- `Source/demo_map/demo_mapV3ProgressionManager.h/.cpp` — added the post-Code-A terminal observer and the matching P7 Host closure boundary.
- `PROJECT.md`, `PROJECT_INFO_CARD.md` — updated P8 state and F-stage verification debt.
- `Docs/Prompt/Dev.D.UE.0.0.9B.P8.0.r0_prompt.md` — downloaded, fully read, archived, and SHA-256 matched to the planner attachment: `A21C83021EE80AFE6308A10700FDB23002FA45C60696F52B731EA613301397C7`.

Not modified:

- Code A terminal classification, Run lifecycle, map, Player Actor, combat, HUD, Run Save, old inventory, Loot, search, world items, rewards, or settlement return values.
- P6 Prepared receipt creation, recovery rebind semantics, Start Run success rules, P5 first migration, and P4x/P7 Drop-only transaction rules.
- No test-only path or fixture was added or run.

## Code A observer boundary

Two existing Code A success points are observed only after their own durable work:

1. `RequestSettlementAndReload` calls P8 only after `Items->RequestSettlement` succeeded and `ProfilePreparationFlow->CommitRuntimeSettlement` returned durable success. It maps only existing `Extraction → Extracted` and `Death → Dead`; other classifications do not write Code B.
2. Production startup observes `RecoveredAbandon` only after `InitializeProduction` has returned `RecoveredAbandonCommitted` or `RecoveredAbandonAlreadyCommitted`, with the existing recovered RunId.

Both pass only stable `OwnerId`, `RunInstanceId`, and the fixed terminal classification. The observer closes a matching P7 host first to discard its UI projection, DragOperation, preview, and input surface, then invokes Code B and only logs the result. It neither returns that result to Code A nor branches Code A cleanup on it.

## P8 terminal receipt and atomic rules

- The per-Owner managed Code B record now has `TerminalReceipts`; each receipt stores a unique receipt ID, exact Owner/Run identity, terminal classification, source session revision, timestamp, full frozen P7-current P6 snapshot/layout, and a digest over that frozen graph.
- The finalizer accepts only a loaded, Owner-matched, committed P6 session whose RunId exactly matches the notification. Absent sidecar, missing session, Prepared session, mismatched identity, unknown classification, duplicate same terminal, and conflicting terminal all produce no record creation, migration, rebind, P5 write, or P6 write.
- A same-Run/same-classification redelivery returns the persisted receipt idempotently. A different classification for that Run is rejected without altering the original result.
- The existing temp-write → read/verify → backup → replace mechanism writes the P5 update (when applicable), terminal receipt, P6 close, and P5 lock release in one durable-record replacement. Restart can therefore either retry the unchanged active session or confirm the one completed receipt; it cannot duplicate a return, confiscation, ItemId, or reopen the session.

## Resolution policy

- `Extracted`: copies the current P7/P6 roots and complete reachable ChildContainer graph back into P5 without new ItemIds, definitions, quantities, parent locations, slots, ContainerIds, or ChildContainerIds. It rejects any pre-existing P5 carry-root item, duplicate ItemId, or conflicting ChildContainerId; warehouse/non-deployed P5 state remains untouched.
- `Dead` and exact committed `RecoveredAbandon`: freeze the same current P6 graph in the receipt, do not merge it into P5, and close the session. They do not use P6’s initial receipt to recreate items and do not create corpses, world drops, rewards, currency, hotbar behavior, or UI settlement.
- A successful P8 record update clears `bHasActiveRunInventorySession`; this is the existing P5 entry lock release. Failed or refused notifications leave the lock and record unchanged.

## Static boundary review

- P4x remains Drop-only: P8 adds no Move, Swap, Merge, QuickMove, double-click, right-click, hotkey, detail, or hidden P1 write entry.
- P7 retains its only active-write chain (`NativeOnDrop → P4 → P3 → P2 → P1 → P6`); P8 only closes its host and then reads/finalizes the durable P6 snapshot outside the UI.
- P5 remains the P5 snapshot; P6 remains the active snapshot; no A/B mirror or dual write is introduced.
- P6 Prepared/rebind code is untouched. A `RecoveredAbandon` notification rejects a Prepared session, preserving the pre-existing P6 rebind authority for the later, Code-A-authorized new Run path.
- Code A changes are limited to post-commit identity/classification observation and P7 UI invalidation. They do not move authority over Run, Player, Loot, search, settlement, Run Save, or old inventory.

## Required P-stage compile

Command executed once after the implementation:

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex
```

- Target: `demo_mapEditor Win64 Development`
- Exit code: `0`
- Key result: `25 action(s)` completed, including `demo_mapCodeBOutOfRaidProfile.cpp`, `demo_mapV3ProgressionManager.cpp`, linking, and target metadata.
- UBT result: `Succeeded`

## F-stage debt

P1–P8 real execution and acceptance remain exclusively for `0.0.9B.F`: game launch, CTA/wrapper paths, unit/automation execution, regressions, restart/interruption verification, visible/UI validation, screenshots, smoke tests, Game target build, Cook, Package, and final acceptance.
