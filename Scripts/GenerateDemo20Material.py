"""New Demo20 material only; no legacy asset reads or overwrites."""
import unreal

package = "/Game/Demo20/Materials/M_Demo20_Color"
if unreal.EditorAssetLibrary.does_asset_exist(package):
    raise RuntimeError("Demo20 color material already exists; refusing overwrite.")
material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
    "M_Demo20_Color", "/Game/Demo20/Materials", unreal.Material, unreal.MaterialFactoryNew())
if not material:
    raise RuntimeError("Could not create color material.")
library = unreal.MaterialEditingLibrary
color = library.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -400, 0)
color.set_editor_property("parameter_name", "Color")
color.set_editor_property("default_value", unreal.LinearColor(0.25, 0.3, 0.29, 1))
roughness = library.create_material_expression(material, unreal.MaterialExpressionConstant, -400, 180)
roughness.set_editor_property("r", 0.85)
glow = library.create_material_expression(material, unreal.MaterialExpressionMultiply, -150, 300)
glow.set_editor_property("const_b", 0.22)
assert library.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
assert library.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
assert library.connect_material_expressions(color, "", glow, "A")
assert library.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
library.recompile_material(material)
if not unreal.EditorAssetLibrary.save_loaded_asset(material):
    raise RuntimeError("Could not save color material.")
unreal.log("DEMO20_MATERIAL_CREATED " + package)
