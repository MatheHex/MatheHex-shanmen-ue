"""Generate the compact persisted V2-E combat map in place."""

from pathlib import Path
import unreal


PACKAGE_PATH = "/Game/DemoV2/Maps/L_V2_CombatDemo"
LAYOUT_VERSION = 3


def log(message):
    unreal.log("V2E_MAP_GENERATION: " + message)


def require(value, message):
    if not value:
        raise RuntimeError(message)
    return value


def set_tags(actor, *tags):
    actor.set_editor_property("tags", [unreal.Name(tag) for tag in tags])


def spawn(actor_subsystem, actor_class, label, location, rotation=None, tags=None):
    actor = require(
        actor_subsystem.spawn_actor_from_class(
            actor_class,
            unreal.Vector(*location),
            rotation or unreal.Rotator(0.0, 0.0, 0.0),
        ),
        "Failed to spawn " + label,
    )
    actor.set_actor_label(label)
    if tags:
        set_tags(actor, *tags)
    return actor


def main():
    project_root = Path(__file__).resolve().parent.parent
    expected_project = project_root / "demo_map.uproject"
    require(expected_project.is_file(), "Project root could not be derived from script location")

    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    asset_library = unreal.EditorAssetLibrary
    require(level_subsystem and actor_subsystem, "Required UE Editor subsystems are unavailable")

    if asset_library.does_asset_exist(PACKAGE_PATH):
        require(level_subsystem.load_level(PACKAGE_PATH), "Could not load existing V2-C map")
        existing = actor_subsystem.get_all_level_actors()
        generated = [a for a in existing if unreal.Name("V2C_GENERATED") in a.get_editor_property("tags")]
        require(len(generated) >= 20, "Existing target map is not a recognized V2-C generated asset; refusing overwrite")
        for actor in generated:
            require(actor_subsystem.destroy_actor(actor), "Could not remove prior generated actor " + actor.get_actor_label())
    else:
        require(level_subsystem.new_level(PACKAGE_PATH, False), "Could not create target level")
    graybox_class = require(unreal.load_class(None, "/Script/demo_map.demo_mapGrayboxBlock"), "Graybox class unavailable")
    marker_class = require(unreal.load_class(None, "/Script/demo_map.demo_mapEncounterMarker"), "Encounter marker class unavailable")

    gray = unreal.LinearColor(0.28, 0.31, 0.36, 1.0)
    dark = unreal.LinearColor(0.14, 0.17, 0.22, 1.0)
    cover = unreal.LinearColor(0.34, 0.43, 0.52, 1.0)
    blocker = unreal.LinearColor(0.55, 0.18, 0.10, 1.0)

    blocks = []

    def block(label, location, dimensions, configure_method, color, *extra_tags):
        actor = spawn(actor_subsystem, graybox_class, label, location, tags=("V2C_GENERATED", *extra_tags))
        getattr(actor, configure_method)(unreal.Vector(*dimensions), color)
        blocks.append(actor)
        return actor

    block("V2C_Ground", (0, 0, -50), (5040, 6580, 100), "configure_ground", gray, "V2C_GROUND")
    block("V2C_Boundary_West", (-2460, 0, 175), (120, 6580, 350), "configure_boundary_wall", dark, "V2C_BOUNDARY", "V2C_BOUNDARY_WEST")
    block("V2C_Boundary_East", (2460, 0, 175), (120, 6580, 350), "configure_boundary_wall", dark, "V2C_BOUNDARY", "V2C_BOUNDARY_EAST")
    block("V2C_Boundary_South", (0, -3230, 175), (5040, 120, 350), "configure_boundary_wall", dark, "V2C_BOUNDARY", "V2C_BOUNDARY_SOUTH")
    block("V2C_Boundary_North", (0, 3230, 175), (5040, 120, 350), "configure_boundary_wall", dark, "V2C_BOUNDARY", "V2C_BOUNDARY_NORTH")

    block("V2C_NarrowWall_A", (-1348, -1820, 150), (2135, 180, 300), "configure_route_wall", dark, "V2C_NARROW_WALL")
    block("V2C_NarrowWall_B", (1348, -1365, 150), (2135, 180, 300), "configure_route_wall", dark, "V2C_NARROW_WALL")
    block("V2C_NarrowWall_C", (-1348, -910, 150), (2135, 180, 300), "configure_route_wall", dark, "V2C_NARROW_WALL")

    block("V2C_CoverYard_CentralBlocker", (0, 175, 180), (600, 735, 360), "configure_route_wall", dark, "V2C_CENTRAL_BLOCKER")
    block("V2C_LowCover_SW", (-385, -525, 75), (400, 160, 150), "configure_low_cover", cover, "V2C_LOW_COVER")
    block("V2C_LowCover_SE", (385, -525, 75), (400, 160, 150), "configure_low_cover", cover, "V2C_LOW_COVER")
    block("V2C_LowCover_NW", (-385, -70, 75), (400, 160, 150), "configure_low_cover", cover, "V2C_LOW_COVER")
    block("V2C_LowCover_NE", (385, -70, 75), (400, 160, 150), "configure_low_cover", cover, "V2C_LOW_COVER")
    block("V2C_ProjectileBlocker", (-560, 840, 160), (180, 630, 320), "configure_projectile_blocker", blocker, "V2C_PROJECTILE_BLOCKER")
    block("V2C_CoreCover_Left", (-1540, 1750, 100), (490, 220, 200), "configure_low_cover", cover, "V2C_LOW_COVER")
    block("V2C_CoreCover_Right", (1540, 1750, 100), (490, 220, 200), "configure_low_cover", cover, "V2C_LOW_COVER")

    regions = [
        ("V2C_Region_Start", (0, -2555, 4), (1540, 840, 8), unreal.LinearColor(0.10, 0.25, 0.55, 1), "V2C_REGION_START"),
        ("V2C_Region_Narrow", (0, -1365, 4), (900, 500, 8), unreal.LinearColor(0.55, 0.42, 0.08, 1), "V2C_REGION_NARROW"),
        ("V2C_Region_Cover", (-840, 175, 4), (700, 700, 8), unreal.LinearColor(0.15, 0.38, 0.32, 1), "V2C_REGION_COVER"),
        ("V2C_Region_Core", (0, 1750, 4), (3010, 1400, 8), unreal.LinearColor(0.38, 0.20, 0.42, 1), "V2C_REGION_CORE"),
        ("V2C_Region_Exit", (0, 2800, 4), (1610, 630, 8), unreal.LinearColor(0.12, 0.42, 0.46, 1), "V2C_REGION_EXIT"),
        ("V2C_Region_OpenProjectileLane", (1190, 875, 4), (800, 600, 8), unreal.LinearColor(0.15, 0.30, 0.45, 1), "V2C_OPEN_PROJECTILE_LANE"),
    ]
    for label, location, dimensions, color, tag in regions:
        block(label, location, dimensions, "configure_region_marker", color, tag)

    player_start = spawn(actor_subsystem, unreal.PlayerStart, "V2C_PlayerStart", (0, -2765, 110), unreal.Rotator(0, 90, 0), ("V2C_GENERATED", "V2C_PLAYER_START"))

    marker_specs = [
        ("V2C_Marker_Friendly", (-315, -2555, 50), "configure_friendly_spawn", None, "V2C_MARKER_FRIENDLY"),
        ("V2C_Marker_Enemy", (175, -2135, 88), "configure_melee_enemy_spawn", None, "V2C_MARKER_ENEMY"),
        ("V2D_Marker_RangedEnemy", (980, 455, 88), "configure_ranged_enemy_spawn", None, "V2D_MARKER_RANGED_ENEMY"),
        ("V2D_Marker_HeavyEnemy", (0, 1085, 100), "configure_heavy_enemy_spawn", None, "V2D_MARKER_HEAVY_ENEMY"),
        ("V2C_Marker_Target_0", (-385, 1610, 50), "configure_training_target_spawn", 0, "V2C_MARKER_TARGET_0"),
        ("V2C_Marker_Target_1", (385, 1610, 50), "configure_training_target_spawn", 1, "V2C_MARKER_TARGET_1"),
        ("V2C_Marker_Target_2", (0, 1995, 50), "configure_training_target_spawn", 2, "V2C_MARKER_TARGET_2"),
        ("V2C_Marker_Exit", (0, 2700, 20), "configure_exit_spawn", None, "V2C_MARKER_EXIT"),
    ]
    markers = []
    for label, location, method, index, tag in marker_specs:
        marker = spawn(actor_subsystem, marker_class, label, location, tags=("V2C_GENERATED", "V2C_ENCOUNTER_MARKER", tag))
        getattr(marker, method)(index) if index is not None else getattr(marker, method)()
        markers.append(marker)

    nav = spawn(actor_subsystem, unreal.NavMeshBoundsVolume, "V2C_NavMeshBounds", (0, 0, 250), tags=("V2C_GENERATED", "V2C_NAV_BOUNDS"))
    nav.set_actor_scale3d(unreal.Vector(23.8, 31.5, 5.0))

    sun = spawn(actor_subsystem, unreal.DirectionalLight, "V2C_DirectionalLight", (0, 0, 3000), unreal.Rotator(-50, -35, 0), ("V2C_GENERATED", "V2C_LIGHT"))
    sun.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property("intensity", 7.0)
    sky = spawn(actor_subsystem, unreal.SkyLight, "V2C_SkyLight", (0, 0, 1200), tags=("V2C_GENERATED", "V2C_LIGHT"))
    sky.get_component_by_class(unreal.SkyLightComponent).set_editor_property("intensity", 1.2)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    require(world is not None, "Editor world unavailable")
    unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
    require(level_subsystem.save_current_level(), "Failed to save V2-C map")
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    require(asset_library.does_asset_exist(PACKAGE_PATH), "Saved V2-C map package does not exist")

    all_actors = actor_subsystem.get_all_level_actors()
    require(len([a for a in all_actors if isinstance(a, unreal.PlayerStart)]) == 1, "Expected one PlayerStart")
    require(len([a for a in all_actors if a.get_class() == marker_class]) == 8, "Expected eight encounter markers")
    require(len([a for a in all_actors if unreal.Name("V2C_BOUNDARY") in a.get_editor_property("tags")]) == 4, "Expected four boundary walls")

    log("V2E_GENERATED_LAYOUT_VERSION=%d" % LAYOUT_VERSION)
    log("PACKAGE=%s" % PACKAGE_PATH)
    log("MAP_SIZE=(5040.0,6580.0) SCALE_RATIO=0.700")
    log("ACTOR_COUNT=%d BLOCK_COUNT=%d MARKER_COUNT=%d" % (len(all_actors), len(blocks), len(markers)))
    for actor in sorted(all_actors, key=lambda a: a.get_actor_label()):
        p = actor.get_actor_location()
        log("ACTOR %s CLASS=%s LOCATION=(%.1f,%.1f,%.1f)" % (actor.get_actor_label(), actor.get_class().get_name(), p.x, p.y, p.z))
    log("PASS")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        unreal.log_error("V2E_MAP_GENERATION: FAIL: %s" % exc)
        raise
