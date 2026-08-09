# Dev.D.UE.0.0.9B.P12.0.r0 Report

## Result

`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`

P12 completes the one authorized production corpse interaction slice. It was
implemented and code-reviewed without launching the product or running F-stage
validation.

## Implemented scope

- `CodeB.BodyContainer.BasicCorpse` remains restricted to
  `M01.Encounter.LOW.Skirmisher.01` at
  `M01.Spawn.LOW.Skirmisher.01`, ordinal `0`. The map-facing static route is
  `M01.BodyTarget.LOW.Skirmisher.01.Ordinal.0`; its `BodyTargetId` is
  deterministically derived from that static route, and its existing P11
  `DeathReceiptId` is derived from `OwnerId + RunInstanceId + BodyTargetId +
  DefinitionId`. No durable identity uses an Actor pointer, display name, UI
  address, controller, or random runtime identifier.
- The Code A adapter is limited to the selected corpse's existing visual,
  contextual-interaction/range shell, timer notification, and page lifecycle.
  It first reads the already materialized, exact P11 projection. It neither
  materializes a body nor creates a receipt, session, fallback, rebind, or
  mutable Code A mirror. Its refusal, persistence failure, page failure, range
  loss, or actor destruction cannot modify Code A death, rewards, map, Run,
  terminal, or Actor authority.
- Owner sidecar schema is `5 → 6`; P11 body records are `1 → 2` in-memory.
  P12 preserves P11's immutable receipt, fixed recipe, definition and first
  materialization digests, ContainerId, ItemId, and child-container graph. It
  adds only persisted action/search identities and states:
  `BodyMaterialized (closed-equivalent) → Opening → Open`,
  `Opening → Interrupted`, and `Hidden → Searching → Revealed`.
  `FCodeBBodyContainerInteractionTiming::OpenSeconds = 0.85f` and
  `ItemSearchSeconds = 0.70f` are named production configuration. Close,
  cancel, distance loss, destruction, teardown, and terminal interrupt only
  the matching action; a cancelled search returns only that item to `Hidden`.
  Stale recovered actions are explicitly interrupted before a new request.
- The existing production P7/P3/P4 Host now presents a transient
  `BodyContainerTarget` second column. `Hidden` and `Searching` slots have no
  widget ItemId or occupied drag payload and expose only unknown/searching
  labels; they cannot show details, drag, merge, swap, equip, or move. One
  explicit click begins one body search; only a completed `Revealed` item is a
  transfer source.
- Position writes remain exclusively
  `NativeOnDrop → P4 → P3 → P2 → P1`. The P12 transfer service validates the
  exact committed P6 session, body target/definition, both revisions, Open/no
  action state, full composite P1 graph, item/container identity union,
  capacities, parent/child graph, visibility, and Move/Merge/Swap legality
  before one durable Owner-record replacement commits both P6 player graph and
  P11 body graph. Failed validation, stale state, or save failure leaves both
  graphs unchanged. A player item deliberately placed into the corpse becomes
  `Revealed`; P8 still settles P6 only and discards all remaining P11 body
  content, including those returned player items, without P5/world/Code A
  fallback.

## Files

- Modified `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h/.cpp` — P12
  schema migration, action/reveal state, exact body projection, interruption,
  and atomic P6/P11 transfer service.
- Modified `Source/demo_map/demo_mapCorpseContainerActor.h/.cpp` — one
  transient corpse route, action timer, distance/lifecycle cancellation, and
  interaction forwarding.
- Modified `Source/demo_map/demo_mapV3ProgressionManager.h/.cpp` — static
  source binding, existing-P11 gate, Host lifecycle, body action dispatch, and
  terminal/teardown cleanup.
- Modified `Source/demo_map/CodeB/demo_mapCodeBP3UI.h/.cpp` — body presenter,
  non-leaking slots, one-item reveal request, and body-aware native Drop gate.
- Added archived Prompt
  `Docs/Prompt/Dev.D.UE.0.0.9B.P12.0.r0_prompt.md`; updated `PROJECT.md` and
  `PROJECT_INFO_CARD.md`; added this Report.
- Unchanged by P12: P4x, P5, P6 Prepared/rebind, P7's player-only Drop policy,
  P8 settlement/finalizer authority, P9/P10 normal-container identities and
  behavior, P11 death receipt/first materialization, and Code A combat, map,
  Loot, Run, Save, and terminal authority.

## Static boundary audit

- The sole M01 actor hook is after Code A has committed its death and after the
  normal Code A corpse succeeds. It attaches only the static route; existing
  legacy corpse/reward data is neither read nor written by P12's Code B graph.
- Open/reopen reads the existing P11 record. There is no P12 call to body
  materialization, no recipe reroll, no identity replacement, and no Open
  rewrite after the body is already open.
- The UI rejects both protected destination slots and protected source payloads;
  cross-column native Drops are the only P6/P11 position-write entry. No
  QuickMove, double-click, right-click, detail action, hotkey, close, cancel,
  or empty Drop gained a write path.
- P8 receives the same post-Code-A terminal observer and discards only
  Run-local P11 residuals; it does not return them to P5/P6 or alter its
  Extracted/Dead/RecoveredAbandon classification, receipts, locks, or order.

## Minimal P-stage verification

The required Editor target was run after the P12-local compile corrections:

```text
C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat demo_mapEditor Win64 Development C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject -WaitMutex
Result: Succeeded
Native exit code: 0
```

No product launch, real death/repeat-death/destruction/reopen/terminal check,
CTA, wrapper, automation, regression, screenshot, Smoke, Game build, Cook,
Package, or final verification was performed. Those items, plus second corpses,
all-enemy migration, random loot, world drops/pickups, other map containers,
and later P functionality, remain out of scope and belong to `0.0.9B.F` or a
future Planner Prompt.
