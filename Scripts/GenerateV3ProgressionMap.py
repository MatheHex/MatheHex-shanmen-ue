"""Create/update the isolated V3 progression map through UE Editor asset APIs."""

import unreal


SOURCE_PACKAGE = "/Game/DemoV2/Maps/L_V2_CombatDemo"
TARGET_PACKAGE = "/Game/DemoV3/Maps/L_V3_ProgressionDemo"
GENERATED_TAG = unreal.Name("V3_PROGRESSION_GENERATED")


def require(value, message):
    if not value:
        raise RuntimeError(message)
    return value


def log(message):
    unreal.log("V3_MAP_GENERATION: " + message)


def spawn(actor_subsystem, actor_class, label, location):
    actor = require(
        actor_subsystem.spawn_actor_from_class(actor_class, unreal.Vector(*location), unreal.Rotator()),
        "Could not spawn " + label,
    )
    actor.set_actor_label(label)
    actor.set_editor_property("tags", [GENERATED_TAG, unreal.Name(label)])
    return actor


def main():
    assets = unreal.EditorAssetLibrary
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    require(levels and actors, "Required UE 5.8 editor subsystems are unavailable")
    require(assets.does_asset_exist(SOURCE_PACKAGE), "Frozen V2 source map is missing")
    created_now = not assets.does_asset_exist(TARGET_PACKAGE)
    if created_now:
        require(assets.duplicate_asset(SOURCE_PACKAGE, TARGET_PACKAGE), "UE asset duplication failed")
        log("DUPLICATED_WITH_EDITOR_ASSET_API source=%s target=%s" % (SOURCE_PACKAGE, TARGET_PACKAGE))
    else:
        require(levels.load_level(TARGET_PACKAGE), "Could not load isolated V3 target map")

    existing = actors.get_all_level_actors()
    for actor in [a for a in existing if GENERATED_TAG in a.get_editor_property("tags")]:
        require(actors.destroy_actor(actor), "Could not remove old generated V3 marker " + actor.get_actor_label())

    marker_class = require(unreal.load_class(None, "/Script/demo_map.demo_mapV3ProgressionMarker"), "V3 marker class unavailable")
    root = spawn(actors, marker_class, "V3_Progression_Root", (0, -2555, 25))
    root.configure_root()

    chest_locations = [(-880, -2555, 45), (880, -2555, 45), (-930, -440, 45)]
    for index, location in enumerate(chest_locations):
        chest = spawn(actors, marker_class, "V3_Chest_Marker_%02d" % (index + 1), location)
        chest.configure_chest(index)

    world_specs = [
        ("Prototype.Item.Weapon.HeavyPracticeBlade", 1, (-330, -2555, 45)),
        ("Prototype.Item.Armor.TrainingVest", 1, (330, -2555, 45)),
        ("Prototype.Item.Accessory.WindTalisman", 1, (0, -2200, 45)),
    ]
    for index, (definition_id, quantity, location) in enumerate(world_specs):
        marker = spawn(actors, marker_class, "V3_World_Item_Marker_%02d" % (index + 1), location)
        marker.configure_world_item(index, unreal.Name(definition_id), quantity)

    loot_area = spawn(actors, marker_class, "V3_Loot_Display_Area", (0, -2450, 20))
    loot_area.configure_loot_display_area()
    enemy_drop = spawn(actors, marker_class, "V3_Enemy_Drop_Validation", (700, 1100, 20))
    enemy_drop.configure_enemy_drop_validation()

    require(levels.save_current_level(), "Could not save V3 progression map")
    require(assets.does_asset_exist(TARGET_PACKAGE), "V3 map package was not saved")
    all_actors = actors.get_all_level_actors()
    generated = [a for a in all_actors if GENERATED_TAG in a.get_editor_property("tags")]
    require(len(generated) == 9, "Expected exactly nine V3 progression markers")
    log("PACKAGE=%s ROOT=1 CHESTS=3 WORLD_ITEMS=3 LOOT_AREA=1 ENEMY_DROP_VALIDATION=1" % TARGET_PACKAGE)
    for actor in sorted(generated, key=lambda a: a.get_actor_label()):
        p = actor.get_actor_location()
        log("MARKER %s LOCATION=(%.1f,%.1f,%.1f)" % (actor.get_actor_label(), p.x, p.y, p.z))
    log("PASS")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        unreal.log_error("V3_MAP_GENERATION: FAIL: %s" % exc)
        raise
