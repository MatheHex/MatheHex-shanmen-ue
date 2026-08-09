# Dev.D.UE.0.0.9B.P9.0.r0 Report

## Status

`NEEDS_P9_COMPILE_REWORK`

P9 functional implementation and static boundary review are complete. The required Editor compile did not return within the execution tool's wait limit and emitted no compiler diagnostic, so this report does not claim a successful compilation or P9 acceptance.

## Changed files

- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h`
  - Adds the normal-container definition, exact materialization result, Run-local record, receipt, state/reveal enums, and read-only projection contracts.
  - Bumps the Owner durable record schema from `2` to `3` and adds `RunLocalNormalContainers`.
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`
  - Adds the production declarative definition `CodeB.NormalContainer.BasicCache` with formal `SpiritDust` and `IronShard` content entries; it does not use fixtures, actor pointers, player inventory, UI, or random generation.
  - Adds schema `1/2 -> 3` in-memory migration, JSON persistence, graph/reveal validation, deterministic receipt/digest generation, temp-verify-backup-replace participation, exact materialization, and read-only projection.
  - P8 terminal closing commit explicitly clears `RunLocalNormalContainers`; its frozen receipt and P5 merge continue to consume only the P6 player-carry snapshot.
- `PROJECT.md` and `PROJECT_INFO_CARD.md`
  - Record P9 ownership, F-stage debt, and the incomplete Editor compile.
- `Docs/Prompt/Dev.D.UE.0.0.9B.P9.0.r0_prompt.md`
  - Exact Planner Prompt archive (SHA-256 `447FF5889681AE44701098E3263B6CBB97E4EC66F9EAA04B5A6EA47D302C8A22`).

## P9 normal-container model

- The durable identity is exactly `OwnerId + RunInstanceId + SearchTargetId + DefinitionId`; `ContainerId` is generated only on the accepted first materialization.
- A `FCodeBRunLocalNormalContainerRecord` carries the Owner/Run/Target/Definition/Container identity, `Closed|Opening|Open|Interrupted` container state, per-item `Hidden|Searching|Revealed` state, revision, receipt/digests, and a complete independent Code B item/container/child-container snapshot.
- P9 materialization creates only `Closed` containers with `Hidden` entries. It adds no opening timer, input, interaction distance, actor, HUD, UI, transfer, QuickMove, pickup, world item, corpse, or player-inventory write path.
- The defined content plan is validated against the existing canonical item registry, capacity, stack rules, slots, duplicate identities, child-container ownership, cycles, and the P1 repository invariant checker before the one durable record commit.

## Exact gate, idempotence, and query

- `MaterializeMatchedRunNormalContainer` first requires an existing, Owner-matched, committed Profile sidecar and an exact committed active P6 `RunInstanceId`.
- Missing/terminal/Prepared/mismatched sessions, invalid identity, unknown definition, invalid plan, duplicate target, and target/definition conflicts do not create, migrate, rebind, or write P5/P6 data.
- A repeated matching target/definition returns the existing projection without a write. A different definition for the same target is rejected without changing the existing record.
- `TryGetMatchedRunNormalContainerProjection` is read-only: it neither materializes, opens, reveals, changes player items, nor writes a sidecar.

## Boundary review

- P4x/P7: no P7 page, Host, input, `NativeOnDrop`, P4/P3/P2/P1 route, or player-carry snapshot code was changed. A source-wide reference audit found P9 normal-container identifiers only in `demo_mapCodeBOutOfRaidProfile.h/.cpp`.
- P5/P6: the new records live in the existing Owner durable record but outside `RepositorySnapshot` and `ActiveRunInventorySession.RepositorySnapshot`; the P6 bridge and Prepared/Rebind path are unchanged.
- P8: `MergeCurrentRunInventoryIntoOutOfRaid` still receives only the frozen P6 session. Before P8 clears the active session it explicitly resets Run-local normal-container records, so unopened container contents cannot enter the P8 receipt or P5.
- Code A: no Code A source files were changed. Code A remains authority for map actors, player, interaction, Loot/search runtime, Run lifecycle, terminal classification, Run Save, and old inventory.

## Required Editor compile

Required command:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' demo_mapEditor Win64 Development 'C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject' -WaitMutex
```

Two invocations were made only because the first was stopped by the tool's 64-second command timeout before it emitted output. The second used the same target and command but reached the 304-second tool timeout, again with no compiler/UBT diagnostic and no exit result from the build itself. No source compile error was observed, but no successful exit code is available; P9 therefore remains `NEEDS_P9_COMPILE_REWORK` pending a completed Editor compilation.

## Deferred F-stage work

No product launch, CTA, wrapper, automation, regression, screenshot, Smoke, Game target build, Cook, Package, or final acceptance was executed. Real P1-P9 verification remains exclusively assigned to `0.0.9B.F`.
