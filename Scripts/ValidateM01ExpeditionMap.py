"""Lightweight structural validation for the M01 expedition map."""

from pathlib import Path
import unreal


PACKAGE_PATH = "/Game/M01/Maps/L_M01_Expedition"


def log(message):
    unreal.log("M01_MAP_VALIDATION: " + message)


def require(value, message):
    if not value:
        raise RuntimeError(message)
    return value


def main():
    project_root = Path(__file__).resolve().parent.parent
    require((project_root / "demo_map.uproject").is_file(), "Project root is invalid")
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    require(levels.load_level(PACKAGE_PATH), "M01 map could not be loaded")
    actors = actor_subsystem.get_all_level_actors()

    marker_class = require(unreal.load_class(None, "/Script/demo_map.demo_mapM01Marker"), "M01 marker class unavailable")
    block_class = require(unreal.load_class(None, "/Script/demo_map.demo_mapM01GrayboxBlock"), "M01 block class unavailable")
    risk_class = require(unreal.load_class(None, "/Script/demo_map.demo_mapM01RiskZone"), "M01 risk class unavailable")
    exit_class = require(unreal.load_class(None, "/Script/demo_map.demo_mapM01ExtractionZone"), "M01 exit class unavailable")
    art_class = require(unreal.load_class(None, "/Script/demo_map.demo_mapM01ArtProp"), "M01 art prop class unavailable")

    starts = [a for a in actors if isinstance(a, unreal.PlayerStart)]
    markers = [a for a in actors if a.get_class() == marker_class]
    blocks = [a for a in actors if a.get_class() == block_class]
    risks = [a for a in actors if a.get_class() == risk_class]
    exits = [a for a in actors if a.get_class() == exit_class]
    navs = [a for a in actors if isinstance(a, unreal.NavMeshBoundsVolume)]
    art_props = [a for a in actors if a.get_class() == art_class]

    require(len(starts) == 1, "Expected exactly one PlayerStart")
    require(len(risks) == 3, "Expected three risk volumes")
    require(len(exits) == 3, "Expected three authored exits")
    require(len(navs) == 1, "Expected one navigation bounds volume")
    require(len(blocks) >= 20, "Graybox block count is too low")
    require(len(art_props) >= 40, "P5 visual layer is incomplete")
    require(all(a.has_visual_only_contract() for a in art_props), "P5 art prop owns collision or navigation")

    marker_ids = [str(a.get_editor_property("stable_id")) for a in markers]
    require(len(marker_ids) == len(set(marker_ids)), "Duplicate M01 marker stable IDs")
    expected_markers = {
        "M01",
        "M01.Route.Low.North", "M01.Route.Low.South",
        "M01.Route.Mid.Loop.North", "M01.Route.Mid.Loop.South",
        "M01.Route.High.Core", "M01.Route.Return",
        "M01.Encounter.Normal.01", "M01.Encounter.Normal.02", "M01.Encounter.Elite.01",
        "M01.Resource.TIER_1.Cluster.01", "M01.Resource.TIER_2.Cluster.01", "M01.Resource.TIER_3.Cluster.01",
        "M01.Boss.Main",
        "M01.Retreat.LowToSafe", "M01.Retreat.MidToLow", "M01.Retreat.HighToMid",
    }
    require(expected_markers.issubset(set(marker_ids)), "Required stable topology markers are missing")

    risk_ids = {str(a.get_editor_property("stable_id")) for a in risks}
    require(risk_ids == {"M01.Risk.LOW", "M01.Risk.MID", "M01.Risk.HIGH"}, "Risk IDs are incomplete")
    exit_tags = {str(tag) for actor in exits for tag in actor.get_editor_property("tags")}
    require({"M01.Exit.DiscardSpatial", "M01.Exit.Regular", "M01.Exit.Boss"}.issubset(exit_tags), "Exit IDs are incomplete")

    block_ids = [str(a.get_editor_property("stable_id")) for a in blocks]
    require(any("LowBarrier.Low" in value for value in block_ids), "LOW low barriers missing")
    require(any("LowBarrier.Mid" in value for value in block_ids), "MID low barriers missing")
    require(any("LowBarrier.High" in value for value in block_ids), "HIGH low barriers missing")
    require(any("SolidWall.Low" in value for value in block_ids), "LOW solid walls missing")
    require(any("SolidWall.Mid" in value for value in block_ids), "MID solid walls missing")
    require(any("SolidWall.High" in value for value in block_ids), "HIGH solid walls missing")

    art_tags = {str(tag) for actor in art_props for tag in actor.get_editor_property("tags")}
    require({
        "M01_ART_LOW", "M01_ART_MID", "M01_ART_HIGH", "M01_ART_BOSS",
        "M01_ART_PINE", "M01_ART_TIMBER", "M01_ART_RUIN", "M01_ART_ROCK",
        "M01_ART_CRYSTAL", "M01_ART_LANDMARK", "M01_ART_ALTAR", "M01_ART_EXIT",
    }.issubset(art_tags), "P5 regional art vocabulary is incomplete")

    forbidden_fragments = ("V3ProgressionMarker", "EncounterMarker", "TrainingTarget", "EnemyCharacter", "SpiritStonePickup", "demo_mapExitZone")
    forbidden = [a.get_actor_label() for a in actors if any(part in a.get_class().get_name() for part in forbidden_fragments)]
    require(not forbidden, "Legacy content exists in M01: " + ", ".join(forbidden))

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
    unreal.SystemLibrary.execute_console_command(world, "MAP CHECK")
    log("PLAYER_START=1 NAV_BOUNDS=1 BLOCKS=%d MARKERS=%d" % (len(blocks), len(markers)))
    log("RISKS=LOW,MID,HIGH EXITS=DiscardSpatial,Regular,Boss")
    log("THEME=DESOLATE_SPIRIT_MINE ART_PROPS=%d VISUAL_COLLISION=SEPARATED" % len(art_props))
    log("STABLE_IDS=PASS CONTENT_ISOLATION=PASS")
    log("PASS")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        unreal.log_error("M01_MAP_VALIDATION: FAIL: %s" % exc)
        raise
