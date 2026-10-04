"""Create a new formal Demo20 map. Never duplicate or overwrite a legacy/practice map."""
import unreal

PACKAGE = "/Game/Demo20/Maps/L_Demo20_JadePass"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(PACKAGE):
    raise RuntimeError("Formal Demo20 map exists; refusing to overwrite authored content.")
mode = unreal.load_class(None, "/Script/demo_map.ShanmenDemo20ExpeditionGameMode")
if not mode:
    raise RuntimeError("Build Editor target before generating the formal map.")
if not levels.new_level(PACKAGE, False):
    raise RuntimeError("Could not create formal Demo20 map.")
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", mode)
start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-760, 0, 100))
if not start:
    raise RuntimeError("Could not create safe entry point.")
start.set_actor_label("Demo20_JadePass_SafeEntry")
if not levels.save_current_level():
    raise RuntimeError("Could not save formal Demo20 map.")
unreal.log("DEMO20_EXPEDITION_MAP_CREATED " + PACKAGE)
