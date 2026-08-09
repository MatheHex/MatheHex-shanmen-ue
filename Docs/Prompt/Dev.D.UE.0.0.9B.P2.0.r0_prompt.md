# Dev.D.UE.0.0.9B.P2.0.r0 Prompt

## Task identity

- Project: `Dev.D.UE.0.0.9B`
- Stage: P2 — Code B player configuration / out-of-raid stash non-runtime vertical slice
- Task: `Dev.D.UE.0.0.9B.P2.0.r0`
- Active root: `C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`
- Baseline: `C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1`
- Engine: Unreal Engine 5.8
- Report: `Dev.D.UE.0.0.9B.P2.0.r0_report.md`

## Accepted P1 result and immutable boundaries

P1 established the isolated Code B source boundary at `Source\\demo_map\\CodeB`, one Repository authority, unique item instances and placements, Revision, immutable value snapshots, atomic Move/Swap/Merge/Split/Equip/Unequip transactions, rollback on failure, fixed Seed `20260805` with 1,000 random transaction attempts, and passing Code B automation, Editor/Game builds, and default-map Smoke.

Code A remains the formal runtime authority for the existing game. P2 must not delete or rewrite Code A, alter XFix1, connect Code B to the formal player Actor, Loot, UI, Profile, Run, SaveGame, default map, or runtime path, and must not create A/B synchronization or double-write. P2 must not create a second Repository, second mutable item array, or a Presenter-owned item truth.

## Single objective

On top of the P1 Code B core, implement a real C++ and automation-verifiable player configuration / out-of-raid stash vertical slice. Establish a controlled player and stash Fixture, a stable read-only player/stash Projection or equivalent View Model, and one P2 Application Service / Command Adapter through which all changes become P1 transaction requests.

The slice must express one unique item moving between the stash, fixed six-slot basic storage, equipment slots, and the internal storage of an equipped spatial item while preserving ItemId, quantity, definition, level, quality, random seed, extension fields, Repository Revision, and atomic failure semantics.

## Required player and stash Fixture

The independent Code B Fixture must contain:

- one weapon equipment slot;
- one armor/robe equipment slot;
- one spatial-item equipment slot;
- at least two independently addressable configurable accessory slots;
- a fixed six-slot basic quick-item storage container;
- one spatial item with a stable associated internal ContainerId and explicit capacity;
- an independently expandable 1x1 stash with at least 30 slots;
- fixed DefinitionIds, deterministic ItemIds/ContainerIds or an equivalent reproducible Fixture, and an auditable initial layout.

Fixture items must include two equipment candidates, a robe, two accessories, a spatial item, stackable materials/consumables for Split/Merge, an invalid equipment negative case, and enough occupied/full targets for failure cases. The Fixture Builder is setup-only and must not be exposed as a normal runtime naked-write entry.

The spatial item association must remain a Repository relationship. Its internal contents are normal Repository item instances. P2 does not implement moving a loaded spatial item as one compound object, infinite nesting, spatial-item drop, world pickup, or persistence. A loaded spatial-item overall move may return a clear unsupported/rule-rejection result without clearing or copying contents.

## Projection requirements

Build a UI-independent read-only projection. It must carry the Repository Revision, player layout id, stash ContainerId/capacity/ordered slots, stable weapon/robe/spatial/accessory SlotIds, six stable basic-slot indices, spatial-item internal-container capacity and slots, ItemIds, and read-only DefinitionId/quantity/level/quality/type-or-tag fields. It must support later item-detail lookup by stable ItemId and identify the latest affected item/container or equivalent incremental-refresh information.

Projection rules:

- the same Repository state produces stable ordering and stable empty-slot addresses;
- projection values expose no mutable pointer/reference/container that can bypass transactions;
- no Widget, Actor, UMG lifecycle, icon, or UI type enters the domain model;
- instance truth comes from the Code B Repository; definition display data may come from a Code B catalog/read-only adapter;
- projection failure is explicit and never silently ignores duplicate ItemIds, invalid locations, or invariant failures;
- one ItemId cannot appear in two projection slots.

## Single P2 Application entry

Implement a lightweight Application Service / Command Adapter. It accepts page intent, translates it to a P1 transaction request, and returns transaction id, operation, Expected Revision, success/failure and P1 error category, old/new Revision, affected item/container ids, and a new read-only projection on success (or a safe refresh-equivalent result).

Through this one entry, exercise:

- stash <-> basic-six Move;
- stash <-> equipment Equip/Unequip and occupied-slot replacement;
- Split/Merge in stash or player storage;
- stash/basic-six <-> equipped spatial-item internal storage Move;
- occupied/full target, incompatible type, source mismatch, and stale Revision failures.

The Application Service may resolve page intent to an explicit target slot but must not re-implement P1 legality, quantity, capacity, or equipment rules. It must not provide drag visuals, menus, split dialogs, auto-sort, drop/pickup, or item use.

## Transaction and Revision rules

On success, Repository commits once, increments Revision once, projection Revision equals Repository Revision, the item appears only at its new parent/slot, untouched instance fields remain equal, and affected ids are returned.

On failure, Repository and projection remain field-for-field identical, Revision does not change, no temporary ItemId is retained, no source item is lost, no equipment slot is prematurely cleared, and the result is explicit enough for future UI messaging.

## Automation requirements

Re-run all `demo_map.CodeB.P1` automation.

Add at least 18 deterministic P2 checks covering Fixture identity/placement, projection contents and stable ordering, stash<->basic round trip, weapon equip and replacement, robe/accessory/spatial equip, incompatible equipment failure, Split, Merge, spatial internal-container round trip, occupied/full target failures, SourceMismatch, StaleRevision, success Revision/affected ids, projection immutability, and a complete stash-configure-player-reorganize-stash chain.

Replay a fixed command sequence from the same Fixture at least 100 times. Each round must have the same final Repository snapshot and player/stash projection, expected ItemId set, quantity conservation, no duplicate placement, no zero-quantity valid item, no dangling location, consistent success/failure Revision rules, and no cross-round residue. This is deterministic application-layer replay, not another 100,000 random transactions.

Build `demo_mapEditor Win64 Development` and `demo_map Win64 Development`; run all P1/P2 automation; run default-map startup Smoke and inspect Fatal/crash/new blocking errors. Do not Cook, Package, make `Latest_Demo`, or perform manual play.

## Non-goals and stop conditions

Do not hand Code B formal player/UI/Loot/Profile/Run/SaveGame authority, modify XFix1, migrate Code A, add A/B sync, implement UMG, drag/drop UI, world items, persistence, shortcut binding/use, loaded spatial-item compound movement, multi-cell/rotation/irregular items, or start P3.

Stop the affected work and report if P1 regression fails, XFix1 must change, A/B double-write is required, a non-runtime Fixture cannot be made, projection requires a copied item array, Profile/SaveGame/Run/player Authority/UMG must be modified, spatial containment creates cycles/dangling references, overlapping unexplained edits are found, or build/automation has an unresolved blocking failure.

## Project records and report

Update `PROJECT.md` and `PROJECT_INFO_CARD.md` to record P2.0.r0, the new Application/Projection/Fixture boundaries, supported player/stash regions, test/build/smoke entry points, and that formal runtime ownership remains Code A. Preserve I0/P1 history.

Create `C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report\Dev.D.UE.0.0.9B.P2.0.r0_report.md` with task/status, file list, P1 impact, Fixture structure, Application and Projection interfaces, immutable truth/boundary proof, operation semantics, Revision behavior, P1 regression, 18 P2 tests, 100 replay Seed/sequence/statistics, build/smoke/log/产物 results, A/B and XFix1 evidence, project updates, findings, and one next-task recommendation.

Allowed statuses:

- `READY_FOR_CODE_B_STASH_LOADOUT_UI`
- `READY_FOR_CODE_B_STASH_LOADOUT_UI_WITH_NONBLOCKING_FINDINGS`
- `NEEDS_P2_REWORK`
- `NEEDS_PLANNER_DECISION`
- `BLOCKED`

Do not start the next P task automatically. After completion, send only the same-name Report to the planning chat with this first line:

`[CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P2.0.r0","file":"Dev.D.UE.0.0.9B.P2.0.r0_report.md"}`

