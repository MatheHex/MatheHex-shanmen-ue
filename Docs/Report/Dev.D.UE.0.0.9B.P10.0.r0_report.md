# Dev.D.UE.0.0.9B.P10.0.r0 Report

## Result

`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`

P10 adds the first normal-container interaction slice on top of the existing
Code A map/input/Run path and Code B P1/P5/P6/P7/P8/P9 boundaries.  The only
production target is `M01.CodeBNormalContainer.BasicCache.01`, backed by
`CodeB.NormalContainer.BasicCache`.

## Implemented scope

- Added the map-facing `Ademo_mapCodeBNormalContainerActor`.  It owns only the
  existing interactable prompt, range surface and named completion timers; it
  forwards the static map identity to the progression manager and owns no
  inventory, reveal or persistence truth.
- The M01 run initializes exactly one BasicCache adapter.  It accepts a
  pre-authored actor with the exact identity or safely projects one from the
  static `M01.Resource.TIER_1.Cluster.01` anchor.  Duplicate identities are
  rejected and adapter failure is non-blocking for Code A's already-formal Run.
- Code A supplies only current `OwnerId`, committed `RunInstanceId`, static
  target-derived `SearchTargetId`, focus/input and distance cancellation.
  The target GUID is deterministic from the static identity; no actor pointer,
  UI address, random session value or display name becomes persistence identity.
- P9 records migrated from schema 1 to 2 add durable action and searching-item
  IDs.  The Owner record migrated from schema 3 to 4.  `Closed -> Opening ->
  Open`, `Opening -> Interrupted`, one-at-a-time `Hidden -> Searching ->
  Revealed`, cancellation, terminal/destroy handling, and stale runtime action
  recovery are all exact Owner/Run/target/definition operations.
- The retained P9 materialization receipt remains immutable after P10 reveal
  and transfer mutations; a reopen of an Open target returns its existing
  projection without a write or reroll.
- P10 reuses the existing production P3/P4 Host and dual-column layout.  P2's
  `NormalContainerTarget` reference is transient presentation-only, not P5/P6
  layout truth.  Hidden and Searching cells omit ItemId, definition, quantity,
  details and drag capability; revealed cells return to the normal detail and
  drag/drop surface.
- Physical transfers remain `NativeOnDrop -> P4 -> P3 -> P2 -> P1`.  P10 adds
  target-aware preview/drop rejection for hidden/searching cells and for
  non-cross-column drops.  Accepted transfers are limited to Move, Merge and
  Swap between the revealed target and player columns.
- `CommitAcceptedMatchedRunNormalContainerTransfer` validates the exact active
  P6 session and open P9 target, checks both source revisions and all P1 graph
  identities, partitions the composite graph by the exact P9 root, independently
  validates both resulting snapshots, rejects Hidden/Searching extraction, then
  writes P6 carry plus P9 target in one temp-verify-backup-replace Owner record
  commit.  P8 terminal finalization continues to discard P9 leftovers while
  keeping only P6 carry for its existing settlement rule.

## Changed files

- `Source/demo_map/demo_mapCodeBNormalContainerActor.h/.cpp`
- `Source/demo_map/demo_mapV3ProgressionManager.h/.cpp`
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h/.cpp`
- `Source/demo_map/CodeB/demo_mapCodeBP2.h/.cpp`
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.h/.cpp`
- `PROJECT.md` and `PROJECT_INFO_CARD.md`

## Explicitly unchanged scope

- P5's out-of-raid migration and layout authority, P6 bridge/Prepared receipt
  and rebind policy, and P8 terminal settlement semantics were not changed.
- P7's player-carry source and Drop-only rule remain intact; P3 UI changes only
  add the transient P10 target column and protection checks around that column.
- Legacy Code A loot/search mutators were not widened or reused: BasicCache is
  a new, exact adapter-only target, so it has no old mutable loot route to
  disconnect.  Code A map, Player, combat, Run Save, old inventory and terminal
  authority remain unchanged.

## Boundary audit

Code B remains the only mutable item/reveal/container truth.  P10 does not add
an editable Code A inventory projection, Run Save field, sidecar, test fixture,
fallback source or dual write.  Code A Run lifecycle, player, combat, map,
terminal decision, HUD and existing P8 ordering remain authoritative.  The
adapter is cleaned up at run-world teardown; target destruction cancels a
pending action and closes the P10 page.

## Minimal P-stage verification

The single permitted target compile completed successfully:

```text
C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat demo_mapEditor Win64 Development C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject -WaitMutex
Result: Succeeded
Total execution time: 28.28 seconds
Native exit code: 0
```

No game launch, real interaction, CTA, automation suite, regression, smoke,
screenshot, Game build, Cook or Package was run.  Those acceptance obligations
remain exclusively in `0.0.9B.F`.

## Handoff

P10 is ready for the next P-stage functional prompt.  F-stage debt includes
runtime interaction/reveal/transfer/restart/terminal validation, UI visibility
and privacy checks, regression, Game build, Cook, Package and final acceptance.
