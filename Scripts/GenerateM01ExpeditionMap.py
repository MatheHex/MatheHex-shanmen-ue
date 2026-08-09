"""Generate the authoritative M01 expedition map and P5 visual layer."""

from pathlib import Path
import unreal


PACKAGE_PATH = "/Game/M01/Maps/L_M01_Expedition"
TASK_ID = "Dev.D.UE.0.0.8.P5.0.r0"


def log(message):
    unreal.log("M01_MAP_GENERATION: " + message)


def require(value, message):
    if not value:
        raise RuntimeError(message)
    return value


def tags(actor):
    return actor.get_editor_property("tags")


def set_tags(actor, *values):
    actor.set_editor_property("tags", [unreal.Name(value) for value in values])


def spawn(actor_subsystem, actor_class, label, location, rotation=None, extra_tags=()):
    actor = require(
        actor_subsystem.spawn_actor_from_class(
            actor_class,
            unreal.Vector(*location),
            rotation or unreal.Rotator(0.0, 0.0, 0.0),
        ),
        "Failed to spawn " + label,
    )
    actor.set_actor_label(label)
    set_tags(actor, "M01_GENERATED", *extra_tags)
    return actor


def main():
    project_root = Path(__file__).resolve().parent.parent
    require((project_root / "demo_map.uproject").is_file(), "Project root is invalid")

    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    assets = unreal.EditorAssetLibrary
    require(levels and actors, "Required Editor subsystems are unavailable")

    if assets.does_asset_exist(PACKAGE_PATH):
        require(levels.load_level(PACKAGE_PATH), "Could not load existing M01 map")
        current = actors.get_all_level_actors()
        generated = [a for a in current if unreal.Name("M01_GENERATED") in tags(a)]
        unmanaged = [
            a for a in current
            if unreal.Name("M01_GENERATED") not in tags(a)
            and not isinstance(a, unreal.RecastNavMesh)
        ]
        require(
            (not current) or (len(generated) >= 20 and not unmanaged),
            "Existing M01 package is not recognized; refusing overwrite",
        )
        for actor in generated:
            require(actors.destroy_actor(actor), "Could not remove " + actor.get_actor_label())
    else:
        require(levels.new_level(PACKAGE_PATH, False), "Could not create M01 map")

    block_class = require(unreal.load_class(None, "/Script/demo_map.demo_mapM01GrayboxBlock"), "M01 block class unavailable")
    marker_class = require(unreal.load_class(None, "/Script/demo_map.demo_mapM01Marker"), "M01 marker class unavailable")
    risk_class = require(unreal.load_class(None, "/Script/demo_map.demo_mapM01RiskZone"), "M01 risk class unavailable")
    exit_class = require(unreal.load_class(None, "/Script/demo_map.demo_mapM01ExtractionZone"), "M01 exit class unavailable")
    art_class = require(unreal.load_class(None, "/Script/demo_map.demo_mapM01ArtProp"), "M01 art prop class unavailable")

    colors = {
        "ground": unreal.LinearColor(0.18, 0.20, 0.23, 1.0),
        "safe": unreal.LinearColor(0.10, 0.27, 0.30, 1.0),
        "low": unreal.LinearColor(0.16, 0.34, 0.20, 1.0),
        "mid": unreal.LinearColor(0.44, 0.32, 0.10, 1.0),
        "high": unreal.LinearColor(0.46, 0.12, 0.12, 1.0),
        "boundary": unreal.LinearColor(0.08, 0.09, 0.11, 1.0),
        "low_wall": unreal.LinearColor(0.25, 0.43, 0.50, 1.0),
        "solid": unreal.LinearColor(0.38, 0.20, 0.45, 1.0),
        "pine": unreal.LinearColor(0.12, 0.42, 0.20, 1.0),
        "timber": unreal.LinearColor(0.38, 0.20, 0.08, 1.0),
        "stone": unreal.LinearColor(0.38, 0.40, 0.43, 1.0),
        "rock": unreal.LinearColor(0.24, 0.20, 0.30, 1.0),
        "spirit": unreal.LinearColor(0.42, 0.08, 0.86, 1.0),
        "boss": unreal.LinearColor(0.82, 0.04, 0.96, 1.0),
        "exit_regular": unreal.LinearColor(0.08, 0.90, 0.42, 1.0),
        "exit_discard": unreal.LinearColor(1.0, 0.46, 0.05, 1.0),
        "exit_boss": unreal.LinearColor(0.85, 0.08, 1.0, 1.0),
    }
    built_blocks = []

    def block(label, stable_id, location, dimensions, method, color, zone):
        actor = spawn(actors, block_class, label, location, extra_tags=("M01_BLOCK", zone))
        getattr(actor, method)(stable_id, unreal.Vector(*dimensions), color)
        built_blocks.append(actor)
        return actor

    block("M01_Ground", "M01.Ground", (0, 0, -50), (12800, 7600, 100), "configure_ground", colors["ground"], "M01_ZONE_ALL")
    block("M01_Boundary_West", "M01.Boundary.West", (-6340, 0, 250), (120, 7600, 600), "configure_boundary", colors["boundary"], "M01_ZONE_ALL")
    block("M01_Boundary_East", "M01.Boundary.East", (6340, 0, 250), (120, 7600, 600), "configure_boundary", colors["boundary"], "M01_ZONE_ALL")
    block("M01_Boundary_South", "M01.Boundary.South", (0, -3740, 250), (12800, 120, 600), "configure_boundary", colors["boundary"], "M01_ZONE_ALL")
    block("M01_Boundary_North", "M01.Boundary.North", (0, 3740, 250), (12800, 120, 600), "configure_boundary", colors["boundary"], "M01_ZONE_ALL")

    block("M01_Region_Safe", "M01.Region.Safe", (-5350, 0, 4), (1700, 2200, 8), "configure_region_floor", colors["safe"], "M01_ZONE_SAFE")
    block("M01_Region_Low", "M01.Region.Low", (-3400, 0, 4), (2500, 6900, 8), "configure_region_floor", colors["low"], "M01_ZONE_LOW")
    block("M01_Region_Mid", "M01.Region.Mid", (-200, 0, 4), (3500, 6900, 8), "configure_region_floor", colors["mid"], "M01_ZONE_MID")
    block("M01_Region_High", "M01.Region.High", (3950, 0, 4), (4500, 6900, 8), "configure_region_floor", colors["high"], "M01_ZONE_HIGH")

    wall_specs = [
        ("Low_North_01", "M01.LowBarrier.Low.North.01", (-4200, 2250, 75), (900, 150, 150), "configure_low_barrier", "M01_ZONE_LOW"),
        ("Low_North_02", "M01.LowBarrier.Low.North.02", (-3000, 2750, 75), (850, 150, 150), "configure_low_barrier", "M01_ZONE_LOW"),
        ("Low_South_01", "M01.LowBarrier.Low.South.01", (-4200, -2250, 75), (900, 150, 150), "configure_low_barrier", "M01_ZONE_LOW"),
        ("Low_South_02", "M01.LowBarrier.Low.South.02", (-3000, -2750, 75), (850, 150, 150), "configure_low_barrier", "M01_ZONE_LOW"),
        ("Mid_Loop_North", "M01.LowBarrier.Mid.North.01", (-300, 2150, 75), (1000, 150, 150), "configure_low_barrier", "M01_ZONE_MID"),
        ("Mid_Loop_South", "M01.LowBarrier.Mid.South.01", (-300, -2150, 75), (1000, 150, 150), "configure_low_barrier", "M01_ZONE_MID"),
        ("High_Core_Left", "M01.LowBarrier.High.Core.01", (3600, -550, 75), (700, 160, 150), "configure_low_barrier", "M01_ZONE_HIGH"),
        ("High_Core_Right", "M01.LowBarrier.High.Core.02", (3600, 550, 75), (700, 160, 150), "configure_low_barrier", "M01_ZONE_HIGH"),
        ("Low_Splitter", "M01.SolidWall.Low.Splitter", (-3550, 0, 225), (1050, 260, 450), "configure_solid_wall", "M01_ZONE_LOW"),
        ("Mid_Core", "M01.SolidWall.Mid.Core", (-100, 0, 260), (950, 950, 520), "configure_solid_wall", "M01_ZONE_MID"),
        ("Mid_North_Gate", "M01.SolidWall.Mid.NorthGate", (1100, 2350, 220), (220, 1350, 440), "configure_solid_wall", "M01_ZONE_MID"),
        ("Mid_South_Gate", "M01.SolidWall.Mid.SouthGate", (1100, -2350, 220), (220, 1350, 440), "configure_solid_wall", "M01_ZONE_MID"),
        ("High_Elite_Pocket", "M01.SolidWall.High.ElitePocket", (3300, 2350, 240), (1400, 220, 480), "configure_solid_wall", "M01_ZONE_HIGH"),
        ("High_Boss_North", "M01.SolidWall.High.BossNorth", (5000, 1500, 260), (1900, 220, 520), "configure_solid_wall", "M01_ZONE_HIGH"),
        ("High_Boss_South", "M01.SolidWall.High.BossSouth", (5000, -1500, 260), (1900, 220, 520), "configure_solid_wall", "M01_ZONE_HIGH"),
    ]
    for suffix, stable_id, location, dimensions, method, zone in wall_specs:
        color = colors["low_wall"] if method == "configure_low_barrier" else colors["solid"]
        block("M01_" + suffix, stable_id, location, dimensions, method, color, zone)

    spawn(actors, unreal.PlayerStart, "M01_PlayerStart", (-5500, 0, 110), unreal.Rotator(0, 0, 0), ("M01_PLAYER_START",))

    def marker(label, stable_id, location, method):
        actor = spawn(actors, marker_class, label, location, extra_tags=("M01_MARKER", stable_id or label))
        getattr(actor, method)(stable_id) if stable_id else getattr(actor, method)()
        return actor

    marker("M01_Root", None, (-5700, -800, 60), "configure_root")
    marker("M01_Route_Low_North", "M01.Route.Low.North", (-3550, 2450, 60), "configure_low_route")
    marker("M01_Route_Low_South", "M01.Route.Low.South", (-3550, -2450, 60), "configure_low_route")
    marker("M01_Route_Mid_North", "M01.Route.Mid.Loop.North", (-200, 2600, 60), "configure_mid_route")
    marker("M01_Route_Mid_South", "M01.Route.Mid.Loop.South", (-200, -2600, 60), "configure_mid_route")
    marker("M01_Route_High_Core", "M01.Route.High.Core", (3650, 0, 60), "configure_high_route")
    marker("M01_Route_Return", "M01.Route.Return", (1450, -3100, 60), "configure_mid_route")
    marker("M01_Encounter_Normal_01", "M01.Encounter.Normal.01", (-3200, 1900, 60), "configure_low_encounter")
    marker("M01_Encounter_Normal_02", "M01.Encounter.Normal.02", (150, -1800, 60), "configure_mid_encounter")
    marker("M01_Encounter_Elite_01", "M01.Encounter.Elite.01", (3400, 2850, 60), "configure_high_encounter")
    marker("M01_Resource_T1", "M01.Resource.TIER_1.Cluster.01", (-4300, 2900, 60), "configure_low_resource_cluster")
    marker("M01_Resource_T2", "M01.Resource.TIER_2.Cluster.01", (600, 2850, 60), "configure_mid_resource_cluster")
    marker("M01_Resource_T3", "M01.Resource.TIER_3.Cluster.01", (4000, -2600, 60), "configure_high_resource_cluster")
    marker("M01_Boss_Main", None, (5200, 0, 60), "configure_boss")
    marker("M01_Retreat_Low", "M01.Retreat.LowToSafe", (-4700, -1200, 60), "configure_low_retreat_connection")
    marker("M01_Retreat_Mid", "M01.Retreat.MidToLow", (-1700, -2900, 60), "configure_mid_retreat_connection")
    marker("M01_Retreat_High", "M01.Retreat.HighToMid", (2200, -2900, 60), "configure_high_retreat_connection")

    for label, location, method in [
        ("M01_Risk_LOW", (-3450, 0, 260), "configure_low"),
        ("M01_Risk_MID", (-200, 0, 260), "configure_mid"),
        ("M01_Risk_HIGH", (3950, 0, 260), "configure_high"),
    ]:
        zone = spawn(actors, risk_class, label, location, extra_tags=("M01_RISK",))
        extent = unreal.Vector(1250, 3450, 450) if method == "configure_low" else (
            unreal.Vector(1750, 3450, 450) if method == "configure_mid" else unreal.Vector(2250, 3450, 450)
        )
        getattr(zone, method)(extent)

    for label, location, method, stable_id in [
        ("M01_Exit_DiscardSpatial", (-3600, -3050, 60), "configure_authored_discard_spatial", "M01.Exit.DiscardSpatial"),
        ("M01_Exit_Regular", (1300, 2900, 60), "configure_authored_regular", "M01.Exit.Regular"),
        ("M01_Exit_Boss", (5700, 0, 60), "configure_authored_boss", "M01.Exit.Boss"),
    ]:
        exit_actor = spawn(actors, exit_class, label, location, extra_tags=("M01_EXIT", stable_id))
        getattr(exit_actor, method)()

    # P5 initial art pass.  Every actor is visual-only; P2 graybox actors keep
    # collision, NavMesh, route, exit, encounter, and reward authority.
    art_props = []

    def art(label, stable_id, location, rotation, dimensions, method, color, zone, style_tag):
        actor = spawn(
            actors, art_class, label, location, unreal.Rotator(*rotation),
            ("M01_ART", zone, style_tag, stable_id),
        )
        getattr(actor, method)(stable_id, unreal.Vector(*dimensions), color)
        art_props.append(actor)
        return actor

    # SAFE / LOW: bright woodland trail and ruined timber outpost.
    for index, (x, y, height) in enumerate([
        (-5700, -1450, 430), (-5650, 1450, 390), (-5000, -3000, 520),
        (-4850, 3100, 470), (-4100, -3250, 420), (-3950, 3300, 500),
        (-3000, -3200, 410), (-2850, 3200, 450),
    ], 1):
        art("M01_Art_Low_Pine_%02d" % index, "M01.Art.Low.Pine.%02d" % index,
            (x, y, 0), (0, 0, index * 17), (150, 150, height),
            "configure_pine", colors["pine"], "M01_ART_LOW", "M01_ART_PINE")
    for index, (x, y, yaw) in enumerate([
        (-5050, -650, 12), (-4700, 720, -18), (-3600, -1350, 28), (-3300, 1450, -24),
    ], 1):
        art("M01_Art_Low_Timber_%02d" % index, "M01.Art.Low.Timber.%02d" % index,
            (x, y, 0), (0, yaw, 0), (430, 120, 320),
            "configure_timber_ruin", colors["timber"], "M01_ART_LOW", "M01_ART_TIMBER")

    # MID: broken stone courtyard, mine entrance, and mixed cover silhouette.
    for index, (x, y, size, yaw) in enumerate([
        (-1450, -3050, 340, 4), (-1450, 3050, 430, -6), (-850, -1250, 300, 12),
        (-780, 1380, 390, -18), (520, -2950, 360, 8), (500, 3000, 450, -8),
        (1020, -1100, 320, 20), (980, 1150, 380, -16),
    ], 1):
        art("M01_Art_Mid_Ruin_%02d" % index, "M01.Art.Mid.Ruin.%02d" % index,
            (x, y, 0), (0, yaw, 0), (size * 0.48, size * 0.48, size),
            "configure_stone_ruin", colors["stone"], "M01_ART_MID", "M01_ART_RUIN")

    # HIGH: bare rock and readable violet spirit crystals around elite pockets.
    for index, (x, y, height) in enumerate([
        (2200, -3300, 440), (2250, 3280, 520), (3050, -3050, 610),
        (3050, 3100, 480), (4300, -3250, 560), (4380, 3220, 640),
    ], 1):
        art("M01_Art_High_Rock_%02d" % index, "M01.Art.High.Rock.%02d" % index,
            (x, y, 0), (0, 0, index * 23), (230, 190, height),
            "configure_rock_spire", colors["rock"], "M01_ART_HIGH", "M01_ART_ROCK")
    for index, (x, y, height) in enumerate([
        (2850, -2250, 300), (2820, 2250, 340), (3650, -2850, 380),
        (3700, 2850, 320), (4700, -2250, 360), (4720, 2250, 390),
    ], 1):
        art("M01_Art_High_Crystal_%02d" % index, "M01.Art.High.Crystal.%02d" % index,
            (x, y, 0), (0, 0, index * 29), (115, 115, height),
            "configure_spirit_crystal", colors["spirit"], "M01_ART_HIGH", "M01_ART_CRYSTAL")

    # Four skyline landmarks make route progression readable from a distance.
    for label, stable_id, location, dimensions, color, zone in [
        ("M01_Art_Landmark_Start", "M01.Art.Landmark.Start", (-5350, 0, 0), (720, 130, 480), colors["pine"], "M01_ART_LOW"),
        ("M01_Art_Landmark_Mid", "M01.Art.Landmark.Mid", (-250, 0, 0), (900, 160, 620), colors["stone"], "M01_ART_MID"),
        ("M01_Art_Landmark_Elite", "M01.Art.Landmark.Elite", (3300, 2450, 0), (720, 140, 720), colors["spirit"], "M01_ART_HIGH"),
        ("M01_Art_Landmark_Boss", "M01.Art.Landmark.Boss", (5150, 0, 0), (960, 170, 920), colors["boss"], "M01_ART_BOSS"),
    ]:
        art(label, stable_id, location, (0, 0, 0), dimensions,
            "configure_landmark", color, zone, "M01_ART_LANDMARK")

    art("M01_Art_Boss_Altar", "M01.Art.Boss.Altar", (5200, 0, 0), (0, 0, 0),
        (760, 760, 520), "configure_boss_altar", colors["boss"], "M01_ART_BOSS", "M01_ART_ALTAR")

    # Distinct exit language: green regular, amber discard, violet Boss.
    for label, stable_id, location, dimensions, color, zone in [
        ("M01_Art_Exit_Regular", "M01.Art.Exit.Regular", (1300, 2900, 0), (420, 420, 500), colors["exit_regular"], "M01_ART_MID"),
        ("M01_Art_Exit_Discard", "M01.Art.Exit.DiscardSpatial", (-3600, -3050, 0), (520, 340, 440), colors["exit_discard"], "M01_ART_LOW"),
        ("M01_Art_Exit_Boss", "M01.Art.Exit.Boss", (5700, 0, 0), (460, 460, 620), colors["exit_boss"], "M01_ART_BOSS"),
    ]:
        art(label, stable_id, location, (0, 0, 0), dimensions,
            "configure_exit_beacon", color, zone, "M01_ART_EXIT")

    nav = spawn(actors, unreal.NavMeshBoundsVolume, "M01_NavMeshBounds", (0, 0, 260), extra_tags=("M01_NAV_BOUNDS",))
    nav.set_actor_scale3d(unreal.Vector(62.0, 36.0, 6.0))
    sun = spawn(actors, unreal.DirectionalLight, "M01_DirectionalLight", (0, 0, 3200), unreal.Rotator(-48, -28, 0), ("M01_LIGHT",))
    sun.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property("intensity", 3.2)
    sky = spawn(actors, unreal.SkyLight, "M01_SkyLight", (0, 0, 1500), extra_tags=("M01_LIGHT",))
    sky.get_component_by_class(unreal.SkyLightComponent).set_editor_property("intensity", 0.85)
    fog = spawn(actors, unreal.ExponentialHeightFog, "M01_HeightFog", (0, 0, 180), extra_tags=("M01_ATMOSPHERE",))
    fog_component = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fog_component.set_editor_property("fog_density", 0.0045)
    fog_component.set_editor_property("fog_height_falloff", 0.20)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    require(world, "Editor world unavailable")
    unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
    unreal.SystemLibrary.execute_console_command(world, "MAP CHECK")
    require(levels.save_current_level(), "Failed to save M01 map")
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    require(assets.does_asset_exist(PACKAGE_PATH), "M01 map asset was not saved")

    all_actors = actors.get_all_level_actors()
    require(len([a for a in all_actors if isinstance(a, unreal.PlayerStart)]) == 1, "Expected exactly one PlayerStart")
    require(len([a for a in all_actors if a.get_class() == risk_class]) == 3, "Expected three risk zones")
    require(len([a for a in all_actors if a.get_class() == exit_class]) == 3, "Expected three authored exits")
    require(len([a for a in all_actors if unreal.Name("M01_NAV_BOUNDS") in tags(a)]) == 1, "Expected one NavMesh bounds")
    require(len([a for a in all_actors if a.get_class() == art_class]) == len(art_props) and len(art_props) >= 40,
            "Expected complete M01 P5 visual layer")

    forbidden_fragments = ("V3ProgressionMarker", "EncounterMarker", "TrainingTarget", "EnemyCharacter", "SpiritStonePickup", "ExitZone")
    forbidden = [a.get_actor_label() for a in all_actors if any(part in a.get_class().get_name() for part in forbidden_fragments)]
    require(not forbidden, "Legacy content persisted in M01: " + ", ".join(forbidden))

    log("TASK_ID=" + TASK_ID)
    log("PACKAGE=" + PACKAGE_PATH)
    log("ACTOR_COUNT=%d BLOCK_COUNT=%d" % (len(all_actors), len(built_blocks)))
    log("REGIONS=SAFE,LOW,MID,HIGH ROUTES=LOW_DUAL,MID_LOOP,HIGH_CORE BOSS=M01.Boss.Main")
    log("EXITS=M01.Exit.DiscardSpatial,M01.Exit.Regular,M01.Exit.Boss")
    log("THEME=DESOLATE_SPIRIT_MINE ART_PROPS=%d VISUAL_COLLISION=SEPARATED" % len(art_props))
    log("CONTENT_ISOLATION=PASS")
    log("PASS")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        unreal.log_error("M01_MAP_GENERATION: FAIL: %s" % exc)
        raise
