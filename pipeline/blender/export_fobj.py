bl_info = {
	"name": "ExportFobj",
	"description": "Saves info about objects in the scene.",
	"author": "based on Dropper by Joseph Hocking (https://jhocking.itch.io/dropper-for-blender)",
	"blender": (2, 80, 0),
	"location": "File > Export > FracasObj",
	"category": "Import-Export",
}

import bpy

import os
from array import array
from pathlib import Path
import yaml

from bpy_extras.io_utils import axis_conversion

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

def to_fmesh(obj):    
    mesh = obj.data
    mesh.calc_loop_triangles()
    mesh.calc_normals_split()
    
    data = array('f')
    data.append(1 if (len(mesh.uv_layers) > 1) else 0)
    for i, tri in enumerate(mesh.loop_triangles):
        for i, (vert_idx, loop_idx) in enumerate(zip(tri.vertices, tri.loops)):
            vert = mesh.vertices[vert_idx].co
            data.append(vert.x)
            data.append(vert.z)
            data.append(-vert.y)
            
            layer = mesh.uv_layers[0]
            uv = layer.data[loop_idx]
            data.append(uv.uv[0])
            data.append(uv.uv[1])
            
            normal = tri.split_normals[i]
            data.append(normal[0])
            data.append(normal[2])
            data.append(-normal[1])
            
            if (len(mesh.uv_layers) > 1):
                for layer in mesh.uv_layers[1:]:
                    uv = layer.data[loop_idx]
                    data.append(uv.uv[0])
                    data.append(uv.uv[1])
                    

    return data

def to_dict(obj, folder):
    entity = {}
    entity['name'] = obj.name
    entity['type'] = obj.type
        
    entity["position"] = pickle_vec(axis_rotated_vec(obj.location))
    entity["rotation"] = swizzled_vec(obj.rotation_euler)
    entity["scale"] = pickle_abs_vec(axis_rotated_vec(obj.scale))
    
    if obj.parent:
        entity['parent'] = obj.parent.name
        
    if obj.instance_collection and obj.instance_collection.library:
        entity['type'] = 'COLLECTION'
        
        prefix = '//'
        suffix = '.blend'
        ref_filename = obj.instance_collection.library.filepath
        ref_filename = ref_filename[len(prefix):] if ref_filename.startswith(prefix) else ref_filename
        ref_filename = ref_filename[:-len(suffix)] if ref_filename.endswith(suffix) else ref_filename
        entity["collection"] = ref_filename
    elif obj.type == 'MESH':
        entity['data'] = obj.name + '.fmesh'
        entity['material'] = obj.data.name
        
        f = open(os.path.join(folder, obj.name + '.fmesh'), 'wb')
        to_fmesh(obj).tofile(f)
        f.close()
    elif obj.type == 'LIGHT':
        energy = obj.data.energy
        entity['light'] = {
            'color'   : {"x": obj.data.color[0] * energy, "y": obj.data.color[1] * energy, "z": obj.data.color[2] * energy},
            'outer_angle': obj.data.spot_size / 2,
            'inner_angle': (obj.data.spot_size / 2) * (1 - obj.data.spot_blend),
        }
        
    return entity

def collection_to_dict(collection, folder):
    dicts = []
    for obj in collection.objects:
        dicts.append(to_dict(obj, folder))
    for inner in collection.children:
        dicts += collection_to_dict(inner, folder)
        
    return dicts

def write_data(context, filepath):
    folder = os.path.dirname(os.path.abspath(filepath))
    data = collection_to_dict(bpy.context.scene.collection, folder)
    in_yaml = yaml.dump({'objects': data}) 
    
    f = open(filepath, 'w+', encoding='utf-8')
    f.write(in_yaml)
    f.close()

    return {'FINISHED'}
        
            
#----- begin gui -----

# ExportHelper is a helper class, defines filename and
# invoke() function which calls the file selector.
from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator

class FracasObj(Operator, ExportHelper):
    """FracasObj - Exports scene as a Fracas Object."""
    bl_idname = "fracas_obj.scene_text"
    bl_label = "Export Fracas Object"

    # ExportHelper mixin class uses this
    filename_ext = ".fobj"

    filter_glob: StringProperty(
        default="*.fobj",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )

    def execute(self, context):
        return write_data(context, self.filepath)


def menu_func_export(self, context):
    self.layout.operator(FracasObj.bl_idname, text="FracasObj")


def register():
    bpy.utils.register_class(FracasObj)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(FracasObj)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()

    #test call
    #bpy.ops.fracas_layout.scene_text('INVOKE_DEFAULT')
