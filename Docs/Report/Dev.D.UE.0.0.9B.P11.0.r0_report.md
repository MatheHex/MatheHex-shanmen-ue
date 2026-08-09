# Dev.D.UE.0.0.9B.P11.0.r0 Report

## Result

`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`

P11 establishes one independent, Run-local Code B body-container truth. No
product run or interaction validation was performed.

## Implemented scope

- Added `CodeB.BodyContainer.BasicCorpse`: a fixed, revisioned two-slot recipe
  (`IronShard ×1`, `SpiritDust ×1`) with Code B P1 root/item identities and
  all materialized items marked `Hidden`.
- Added `RunLocalBodyContainers` to the Owner durable record (schema `4 → 5`).
  Each record is gated by `OwnerId + RunInstanceId + BodyTargetId + DefinitionId`
  and stores the complete P1 graph, definition digest, materialization digest,
  immutable death receipt, and a read-only projection only.
- The selected production source is exactly
  `M01.Encounter.LOW.Skirmisher.01` at static spawn
  `M01.Spawn.LOW.Skirmisher.01`, ordinal `0`. `BodyTargetId` is deterministically
  derived from the static map identity; `DeathReceiptId` is deterministically
  derived from the exact Owner/Run/body-target/definition tuple. Neither uses
  an Actor pointer, name, UI address, controller, or random runtime GUID.
- `ObserveCodeBBodyContainerAfterCodeADeath` is called only after the enemy
  has committed `EnemyState::Dead`. It forwards the minimal static receipt to
  Code B and discards its result, so a Code B refusal or persistence failure
  cannot alter Code A damage, death, Actor handling, map, Run, rewards, or
  terminal authority.
- P11 replays the same receipt as an existing read-only projection without a
  write. A conflicting definition for one body target, one receipt for two
  targets, invalid provenance, non-committed/Prepared/terminal/mismatched P6,
  bad definition, or stale durable record is rejected without fallback,
  rebind, partial graph, or duplicate materialization.
- P8's existing exact-session close transaction now discards both P9 normal
  containers and P11 body containers. It does not return, confiscate,
  compensate, recreate, or otherwise write P11 items into P5, P6, Code A Loot,
  Run Save, or world state.

## Changed files

- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h/.cpp` — P11 types,
  fixed definition, JSON/schema migration, graph validation, one-shot receipt
  materialization, read-only projection, and terminal residual discard.
- `Source/demo_map/demo_mapV3ProgressionManager.h/.cpp` — one static M01
  post-death observer and deterministic production identities.
- `PROJECT.md`, `PROJECT_INFO_CARD.md`, and this Report.

## Static boundary audit

- P4x/P5/P6: unchanged. No P5 migration, P6 bridge, Prepared receipt, or
  recovery rebind behavior was modified.
- P7: unchanged. No input, host, Drop-only path, UI, QuickMove, details, or
  player-location write entry was added.
- P8: settlement/terminal authority, idempotency, locks, receipt freeze, and
  P6 merge/confiscation sequence are unchanged; only the P11 residual array is
  reset in the same already-authorized close transaction.
- P9/P10: their definition, target identity, action/reveal state, transfer
  semantics, UI, and records are not generalized or reused as a body alias.
- Code A: the legacy M01 corpse/reward path remains an independent Code A
  route. P11 neither reads nor mirrors its graph/ItemIds and does not write
  back to it. The P11 fixed Code B graph has no player-visible route yet; no
  same physical `ItemId` truth is shared or dual-written.
- No corpse Actor, map prompt, body UI, search/reveal state, item detail,
  drag payload, transfer, world drop, currency pickup, player inventory write,
  random loot table, or combat/AI behavior was added.

## Minimal P-stage verification

The sole required Editor target compiled successfully:

```text
C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat demo_mapEditor Win64 Development C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject -WaitMutex
Result: Succeeded
Total execution time: 18.26 seconds
Native exit code: 0
```

No product launch, death/repeat-death/recovery/terminal runtime check, CTA,
wrapper, automation, regression, screenshot, Smoke, Game target build, Cook,
Package, or final verification was run. All remain `0.0.9B.F` work.

## Next functional boundary

P12 may add the separately authorized corpse map interaction, search UI,
reveal flow, and real drag/drop transfer. None of that work was started here.
