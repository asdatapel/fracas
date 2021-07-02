bl_info = {
	"name": "Layout",
	"description": "Saves info about objects in the scene.",
	"author": "based on Dropper by Joseph Hocking (https://jhocking.itch.io/dropper-for-blender)",
	"blender": (2, 80, 0),
	"location": "File > Export > Layout",
	"category": "Import-Export",
}

'''
install in Edit > Preferences > Add-ons
once installed: File > Export > Dropper
GPL license https://www.blender.org/support/faq/

Saves info about objects in the scene:
name, linked file, position, rotation, scale
Select either YAML or JSON for the data format.
'''


import bpy
import json
import yaml
from bpy_extras.io_utils import axis_conversion
import mathutils

def write_data(context, filepath, format):
    print("writing scene data...")
    
    data = {
        'OPT_JSON': build_json(),
        'OPT_YAML': build_yaml(),
    }[format]
    
    f = open(filepath, 'w', encoding='utf-8')
    f.write(data)
    f.close()

    return {'FINISHED'}

def build_json():
    dict = {}
    dict["entities"] = []
    
    for obj in bpy.data.objects:
        if obj.instance_collection and obj.instance_collection.library:
            entity = {"name": obj.name}
            entity["ref"] = clean_ref_name(obj.instance_collection.library.filepath)
            
            entity["position"] = {"x":obj.location.x, "y":obj.location.y, "z":obj.location.z}
            entity["rotation"] = {"x":obj.rotation_euler.x, "y":obj.rotation_euler.y, "z":obj.rotation_euler.z}
            entity["scale"] = {"x":obj.scale.x, "y":obj.scale.y, "z":obj.scale.z}
            dict["entities"].append(entity)
        
    return json.dumps(dict, indent=4, sort_keys=True)

def build_yaml():
    dict = {}
    dict["entities"] = []
    
    for obj in bpy.data.objects:
        if obj.instance_collection and obj.instance_collection.library:
            entity = {"name": obj.name}
            entity["ref"] = clean_ref_name(obj.instance_collection.library.filepath)
            
            entity["position"] = pickle_vec(axis_rotated_vec(obj.location))
            entity["rotation"] = swizzled_vec(obj.rotation_euler)
            entity["scale"] = pickle_abs_vec(axis_rotated_vec(obj.scale))
            
            for chld in obj.children:
                if chld.type == 'LIGHT':
                    energy = chld.data.energy
                    entity['light'] = {
                        'color'   : {"x":chld.data.color[0] * energy, "y": chld.data.color[1] * energy, "z":chld.data.color[2] * energy},
                        'position': swizzled_vec(chld.location),
                        'rotation': swizzled_vec(chld.rotation_euler),
                        'outer_angle': chld.data.spot_size / 2,
                        'inner_angle': (chld.data.spot_size / 2) * (1 - chld.data.spot_blend),
                    }
            
            dict["entities"].append(entity)        
            
        
    return yaml.dump(dict)

def clean_ref_name(name):
    prefix = '//'
    name = name[len(prefix):] if name.startswith(prefix) else name
    return name[:-6].upper()

def axis_rotated_vec(vec):
    m = axis_conversion(from_forward='Y', 
        from_up='Z',
        to_forward='-Z',
        to_up='Y')
    return m @ vec
def pickle_vec(vec):
    return  {"x":vec.x, "y": vec.y, "z": vec.z}
def pickle_abs_vec(vec):
    return  {"x":abs(vec.x), "y": abs(vec.y), "z": abs(vec.z)}
def swizzled_vec(vec):
    return {"x": vec.x, "y": vec.z, "z": -vec.y}
def swizzled_scale_vec(vec):
    return {"x": abs(vec.x), "y": abs(vec.z), "z": abs(vec.y)}

#----- begin gui -----

# ExportHelper is a helper class, defines filename and
# invoke() function which calls the file selector.
from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator

class Dropper(Operator, ExportHelper):
    """Dropper - Saves info about objects in the scene."""
    bl_idname = "dropper.scene_text"
    bl_label = "Export Scene Data"

    # ExportHelper mixin class uses this
    filename_ext = ".txt"

    filter_glob: StringProperty(
        default="*.txt",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )

    # options menu next to the file selector
    data_format: EnumProperty(
        name="Data Format",
        description="Choose the data format",
        items=(('OPT_JSON', "JSON", "JavaScript Object Notation"),
               ('OPT_YAML', "YAML", "YAML")),
        default='OPT_YAML',
    )

    def execute(self, context):
        return write_data(context, self.filepath, self.data_format)


def menu_func_export(self, context):
    self.layout.operator(Dropper.bl_idname, text="Dropper")


def register():
    bpy.utils.register_class(Dropper)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(Dropper)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()

    #test call
    bpy.ops.dropper.scene_text('INVOKE_DEFAULT')
