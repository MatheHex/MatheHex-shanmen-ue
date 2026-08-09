# Dev.D.UE.0.0.9B.P17.0.r0 Report

## Status

`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`

## Delivered

P17 makes the real Code B spatial-item child graph definition-driven and durable. It adds no item source, starter grant, fixture, map pickup, Code A inventory authority, P16 loot entry, or runtime test path.

### Formal product mapping

| Semantic | Stable definition data | Compatible slot | Child layout |
| --- | --- | --- | --- |
| Quick spatial ring | `Prototype.Item.Accessory.WindTalisman` (`Fdemo_mapItemIds::WindTalisman`), plus the existing tier ring definitions that carry `RingQuickCapacity` | `Prototype.Slot.SpatialRing` | `QuickRing`; Wind Talisman is 4 slots (tier rings remain definition-derived: 6/8/10/12). |
| Storage pouch | `Prototype.Item.Backpack.Level1` (`Fdemo_mapItemIds::BackpackLevel1`), plus existing level 2 bag | `Prototype.Slot.Backpack` | `StoragePouch`; 36 slots. |

The mapping is derived from stable DefinitionId, category, slot compatibility, stack rule, and `RingQuickCapacity` / `TotalCapacity` effects through the existing catalog resolvers. Display names, widget text, actor labels, fixture IDs, and P16 rolls are not used.

### Changed files

| File | Change and responsibility |
| --- | --- |
| `Source/demo_map/CodeB/demo_mapCodeBInventory.h` | Adds immutable `ECodeBSpatialContainerSemantic` and definition-owned child capacity provenance. |
| `Source/demo_map/CodeB/demo_mapCodeBInventory.cpp` | Validates semantic/capacity compatibility, unique child ownership, self references, child capacity, and the one-level rule; candidate P1 transaction failures remain zero-write. |
| `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp` | Serializes compatible definition fields; creates/migrates missing child containers from catalog capacity with stable `SpatialChildGuid`; performs P5 plus active P6 replacement in one Owner document save, including the active session revision/digest; uses canonical capacity/type during first legacy migration. Existing child graphs are read unchanged. |
| `Source/demo_map/CodeB/demo_mapCodeBP3.h` | Adds the P7 read-only spatial-container projection contract and transient entry state. |
| `Source/demo_map/CodeB/demo_mapCodeBP3.cpp` | Adds active-P6-only selection/entry/return. Quick entry requires the exact current `SpatialContainerId`; a real placed pouch can enter without becoming BaseQuick. Refresh, close, stale state, and invalidation discard the transient projection. |
| `Docs/Prompt/Dev.D.UE.0.0.9B.P17.0.r0_prompt.md` | Archived downloaded task prompt. |
| `Docs/Report/Dev.D.UE.0.0.9B.P17.0.r0_report.md` | This report. |

### Reviewed, intentionally unchanged boundaries

- Code A files: none changed. Map, Actor, input, HUD, combat, life, Run, Run Save, old inventory/Loot/search, and terminal authority remain Code A-owned.
- P5→P6 bridge: the existing recursive carry copy follows each `ChildContainerId`; it carries owner and child graph together.
- P8: existing player-only closure and extracted merge operate on the validated P1 snapshot; extracted preserves the complete current player graph, while Dead/RecoveredAbandon retain their existing confiscation behavior. P9/P11 residual discard timing is unchanged.
- P13/P15: existing eligibility requires a currently valid P6 BaseQuick quick-use item; child storage does not qualify. P15 also rejects a complex parent item.
- P14: existing gate accepts only one simple item currently in P6 BaseQuick and rejects an item with `ChildContainerId`; no flattening, duplicate ground graph, or whole-bag drop was added.
- P9/P10/P11/P12/P13/P14/P15/P16 code paths and all fixtures/tests were not modified or executed.

## Graph and durability audit

- A canonical quick ring/pouch gets one stable `SpatialChildGuid(ItemId)` child container whose type and capacity are derived from the formal definition.
- P1 validates one parent placement, valid definition/capacity, no self-child relation, no duplicate child owner, and rejects any spatial item or further child graph inside a space child container. Move/Swap/Merge/Split/Equip/Unequip still validate a candidate snapshot before commit.
- First P5 migration creates the complete graph before its durable record save. Existing P5/P6 records preserve their current item/container IDs, slots, contents, and placements; only a missing child is added. P5 and any active P6 snapshot are saved as one Owner document replacement, with the P6 revision and pre-freeze payload digest advanced when required.
- P6 remains the same OwnerId + RunInstanceId session and existing P1 transaction path. P7 does not write a widget inventory and introduces no direct space-slot mutation route.
- P7 quick access is only available while the ring item is in the exact current spatial-ring equipment container. Unequip/move/stale/recovery/close invalidates the transient view. A pouch is selected from its real placed P6 item, gains no 1–9/P15/new-input behavior, and all later movement stays on existing P6/P1 operations.

## Static audit conclusions

- No new starter, fake ID, fixture, map source, Code A Loot source, P16 profile entry, world pickup, or second inventory was introduced.
- No Code A inventory/space-container authority was added.
- P14 remains an eligibility rejection for complex space graphs; P13/P15 stale or non-BaseQuick references remain governed by existing Code B reconciliation/eligibility rules.
- No P10/P12 container UI, actor, target identity, reader, reveal, or drag state machine was altered.

## Editor compile

Command:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' demo_mapEditor Win64 Development 'C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject' -WaitMutex
```

- Target: `demo_mapEditor Win64 Development`
- Final native exit code: `0`
- Result: `Succeeded`.
- A first Editor build compiled the affected module successfully. Static review then identified the local active-P6 migration revision/digest detail; the same Editor target was incrementally rebuilt after that correction and also returned `0`. No executable, test, CTA/wrapper, screenshot, smoke, Game target, Cook, or Package operation was run.

## Deferred to 0.0.9B.F

Not run: real P5→P6→P8 lifecycle; ring/pouch P7 entry and return; child-container movement; recovery/rebind; P14 compatibility; P13/P15 cleanup; terminal outcomes; automation; regression; screenshots; Smoke; Game Build; Cook; Package; final validation.

Not started: whole-bag ground drop/pickup, space-item sources, nested bags, space equipment effects, weapon/robe/accessory effects, other consumables, and later P-stage features.
