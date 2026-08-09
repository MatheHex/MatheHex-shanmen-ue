"""Read-only marker validation for V2/V3 isolation."""

import unreal


V2 = "/Game/DemoV2/Maps/L_V2_CombatDemo"
V3 = "/Game/DemoV3/Maps/L_V3_ProgressionDemo"


def require(value, message):
    if not value:
        raise RuntimeError(message)
    return value


def count_markers(levels, actors, package, load=True):
    if load:
        require(levels.load_level(package), "Could not load " + package)
    marker_class = require(unreal.load_class(None, "/Script/demo_map.demo_mapV3ProgressionMarker"), "Marker class unavailable")
    found = [actor for actor in actors.get_all_level_actors() if actor.get_class() == marker_class]
    details = []
    for actor in sorted(found, key=lambda value: value.get_actor_label()):
        details.append("%s TYPE=%s INDEX=%s ID=%s DEF=%s QTY=%s" % (
            actor.get_actor_label(), actor.get_editor_property("marker_type"), actor.get_editor_property("marker_index"),
            actor.get_editor_property("stable_id"), actor.get_editor_property("definition_id"), actor.get_editor_property("quantity")))
    return found, details


def main():
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    require(levels and actors, "Editor subsystems unavailable")
    v2_markers, _ = count_markers(levels, actors, V2, False)
    require(len(v2_markers) == 0, "V2 contains V3 markers")
    v3_markers, details = count_markers(levels, actors, V3)
    unreal.log("V3_MAP_VALIDATION: DISCOVERED_MARKERS=%d" % len(v3_markers))
    if len(v3_markers) != 9:
        for actor in actors.get_all_level_actors():
            if unreal.Name("V3_PROGRESSION_GENERATED") in actor.get_editor_property("tags"):
                unreal.log("V3_MAP_VALIDATION: GENERATED_ACTOR %s CLASS=%s" % (actor.get_actor_label(), actor.get_class().get_path_name()))
    require(len(v3_markers) == 9, "V3 marker count is not nine")
    for detail in details:
        unreal.log("V3_MAP_VALIDATION: " + detail)
    unreal.log("V3_MAP_VALIDATION: V2_MARKERS=0 V3_MARKERS=9 PASS")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        unreal.log_error("V3_MAP_VALIDATION: FAIL: %s" % exc)
        raise
