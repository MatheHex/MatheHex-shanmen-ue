# Dev.D.UE.0.0.9B.P13.0.r0 Report

## Result

`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`

P13 adds the requested Code B 1—9 hotbar-reference lifecycle. It is a
versioned Owner-sidecar feature only: no Code A hotbar, P1 item graph, world
use, consumption, keyboard use, or F-stage product validation was added.

## Implemented scope

- `FCodeBHotbarBindings` contains exactly nine logical entries. Each entry is
  only `SlotIndex` in `[1..9]` plus either an explicit empty state or one
  `ItemId`. The durable binding record contains no quantity, definition,
  location, `ContainerId`, child graph, widget, actor, or runtime GUID.
- `FCodeBItemDefinition::bQuickUsable` is the explicit Code B eligibility
  semantic. New P5 migration derives it from the existing authoritative Code A
  consumable category. Older serialized definitions that predate the field are
  migrated from that same existing category on read and are written explicitly
  on the next accepted sidecar save; unknown and non-consumable definitions
  remain false. P13 creates no item, starter grant, recipe, use effect, or
  consumption rule.
- A binding is accepted only when the ItemId currently exists, has positive
  quantity, resolves to `bQuickUsable`, and has parent container exactly equal
  to the P5/P6 `BasicContainerId`. Rebinding atomically removes the same ItemId
  from any old slot before setting its new slot. Unbinding only clears the
  selected reference. The Store binding APIs never modify a P1 snapshot or an
  item position.
- P5 Owner records migrate `schema 6 → 7`; P6 sessions migrate `schema 2 →
  3`. Current schemas serialize the nine-slot model. Older P5/P6 records
  receive the explicit empty nine-slot model and reconciliation before the
  next durable replacement.
- P5→P6 bridge copies the reconciled references with the exact carry graph;
  P5 remains locked while the prepared receipt owns P6. After P6 extraction,
  the P5 and P6 candidates are reconciled together. P8 `Extracted` carries
  only P6 references whose ItemIds truly returned to P5 `BasicContainerId`;
  `Dead` and `RecoveredAbandon` discard P6 references. P7 player snapshot
  commits and P10/P12 composite transfers reconcile P6 references inside the
  same Owner durable replacement before save. Reconcile only clears invalid
  references and never auto-binds an item.
- The existing production P3 Host has a P13 projection/callback presenter for
  real P5 and exact P6 scope. It displays nine read-only reference states and
  provides separate explicit `绑定` and `清空` actions. Binding first requires
  a selected, currently visible BaseQuick `QuickUsable` item; all durable
  validation remains in the Store. Drag/drop is not a binding route. No
  `1..9` keyboard mapping, double-click, right-click, detail, close, cancel,
  outside click, no-payload interaction, use, or consume path writes a
  hotbar reference.

## Files

- Modified `Source/demo_map/CodeB/demo_mapCodeBInventory.h/.cpp` — explicit
  Code B `bQuickUsable` definition semantic and equality.
- Modified `Source/demo_map/CodeB/demo_mapCodeBP2.h/.cpp` — read-only
  `bQuickUsable` projection for the existing P3 Host; fixture-only data marks
  its consumable definition eligible without changing production P5 sources.
- Modified `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h/.cpp` —
  nine-slot record/session schemas, JSON migration, projection/bind APIs,
  P5/P6/P7/P8/P10/P12 reconciliation, and lifecycle handoff rules.
- Modified `Source/demo_map/CodeB/demo_mapCodeBP3UI.h/.cpp` — transient P13
  presenter and explicit bind/unbind controls; the UI holds no durable item
  truth and has no use/consume behavior.
- Modified `Source/demo_map/demo_mapV3ProgressionManager.cpp` — P5 and exact
  active-P6 presenter callbacks, each scoped to the real OwnerId and the P6
  RunInstanceId where applicable.
- Archived `Docs/Prompt/Dev.D.UE.0.0.9B.P13.0.r0_prompt.md`; updated
  `PROJECT.md` and `PROJECT_INFO_CARD.md`; added this report.
- Unchanged by P13: P1 item/container/child graph transactions, Code A
  hotbar/runtime/save ownership, P4x input semantics, P9/P10 normal-container
  truth, P11/P12 corpse truth, Code A combat/map/Loot/Run/terminal authority,
  and all actual item-position writes.

## Static boundary audit

- `IsHotbarItemEligible` is the sole shared eligibility predicate and checks
  existing item, quantity, explicit semantic, and exact `BasicContainerId`.
  Shape validation rejects any non-nine-slot document and duplicate ItemId.
- `ReconcileHotbarBindings` appears at P5 snapshot commits, P6 snapshot
  commits, the P5→P6 bridge/finalizer, P8 terminal close, and the P10/P12
  composite transfer candidates before their same-Owner saves. It cannot
  create a reference.
- P3 has no numeric-key branch. Its Hotbar controls invoke only Store-owned
  bind/unbind callbacks; neither control takes a drag payload or calls P1/P2
  item-position services.
- No P13 automation, wrapper, run command, screenshot route, runtime actor,
  or test source was added. P13 test execution is intentionally deferred to
  `0.0.9B.F` per the stage rule.

## Minimal P-stage verification

The required Editor target initially caught one P13-local UMG local-variable
name that shadowed `UWidget::Slot`; it was renamed without changing behavior.
The same required command was then rerun successfully:

```text
C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat demo_mapEditor Win64 Development C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject -WaitMutex
Result: Succeeded
Native exit code: 0
```

No product launch, CTA, wrapper, automation, regression, screenshot, Smoke,
Game build, Cook, Package, or functional verification was run. Those checks,
plus real bind/rebind/unbind, P5→P6, P8 terminal, P10/P12 transfer, and
multi-profile execution scenarios, remain explicit `0.0.9B.F` debt unless a
future Planner Prompt changes scope.
