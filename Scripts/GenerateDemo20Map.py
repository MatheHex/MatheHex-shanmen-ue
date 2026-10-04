"""Create a NEW Demo 2.0 entry map; never load/duplicate or rewrite legacy maps."""
import unreal

PACKAGE = "/Game/Demo20/Maps/L_Demo20_StoneCourt"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(PACKAGE):
    raise RuntimeError("Demo20 map already exists. Refusing to overwrite authored content.")
mode = unreal.load_class(None, "/Script/demo_map.ShanmenDemo20GameMode")
if not mode:
    raise RuntimeError("Build the Editor target before generating the Demo20 map.")
if not levels.new_level(PACKAGE, False):
    raise RuntimeError("Could not create Demo20 entry map.")
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", mode)
start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-760, 0, 100))
if not start:
    raise RuntimeError("Could not author the Demo20 spawn point.")
start.set_actor_label("Demo20_PlayerStart")
if not levels.save_current_level():
    raise RuntimeError("Could not save Demo20 entry map.")
unreal.log("DEMO20_MAP_CREATED " + PACKAGE)
