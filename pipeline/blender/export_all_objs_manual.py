import bpy

import os
from pathlib import Path

def obj_string(obj, uv_layer):    
    mesh = obj.data
    mesh.calc_loop_triangles()
    mesh.calc_normals_split()
    
    uvs = mesh.uv_layers[uv_layer]
    
    vert_positions = ''
    vert_tex = ''
    vert_normals = ''
    faces = ''
    for vert in mesh.vertices:
        vert_positions += f'v {vert.co.x} {vert.co.z} {-vert.co.y}\n'
    for i, tri in enumerate(mesh.loop_triangles):
        for loop in tri.loops:
            uv = uvs.data[loop]
            vert_tex += f'vt {uv.uv[0]} {uv.uv[1]}\n'
        for normal in tri.split_normals:
            vert_normals += f'vn {normal[0]} {normal[2]} {-normal[1]}\n'
        
        faces += f'f {tri.vertices[0] + 1}/{i * 3 + 1}/{i * 3 + 1} '
        faces += f'{tri.vertices[1] + 1}/{i * 3 + 1 + 1}/{i * 3 + 1 + 1} '
        faces += f'{tri.vertices[2] + 1}/{i * 3 + 2 + 1}/{i * 3 + 2 + 1}\n'
        
    return vert_positions + vert_tex + vert_normals + faces


def do(parent_folder):
    meshes = {}
    for obj in bpy.context.selected_objects:
        if obj.type == 'MESH':
            meshes[obj.data.name] = obj
            
    for m in meshes:
        bpy.ops.object.select_all(action='DESELECT')
        
        obj = meshes[m]
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        
        folder = os.path.join(parent_folder, m)
        Path(folder).mkdir(parents=True, exist_ok=True)
            
        for i, uvs in enumerate(obj.data.uv_layers):
            output = obj_string(obj, i)            
            filepath = os.path.join(folder, f'model_{i}.obj')
            file = open(filepath, 'w+')
            file.write(output)
            file.close()
            
            
class ExportObjsProperties(bpy.types.PropertyGroup):
    directory : bpy.props.StringProperty(maxlen=1024, subtype='DIR_PATH', options={'HIDDEN', 'SKIP_SAVE'})

class View3dExportAllObjs(bpy.types.Operator):
    """Export OBJs with all UVs"""      # Use this as a tooltip for menu items and buttons.
    bl_idname = "view3d.export_all_objs"        # Unique identifier for buttons and menu items to reference.
    bl_label = "Export All OBJs"         # Display name in the interface.
    bl_options = {'REGISTER', 'UNDO'}  # Enable undo for the operator.

    def execute(self, context):        # execute() is called when running the operator.
        my_props = context.scene.export_objs_props
        
        filepath_full = bpy.path.abspath(my_props.directory)
        do(filepath_full)

        return {'FINISHED'}            # Lets Blender know the operator finished successfully.
    

class View3dExportAllObjs_Panel(bpy.types.Panel):
    bl_idname = "view3d.export_all_objs_panel"        # Unique identifier for buttons and menu items to reference.
    bl_label = "Export All OBJs Panel"         # Display name in the interface.
    bl_category = "Export"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    
    def draw(self, context):
        layout = self.layout
        
        my_props = context.scene.export_objs_props
        
        layout.prop(my_props, "directory")
        layout.operator("view3d.export_all_objs", text="Export all OBJs")
        
bl_info = {
    "name": "Export All OBJs",
    "blender": (2, 80, 0),
    "location": "View3D",
    "category": "Generic"
}

def register():
    bpy.utils.register_class(View3dExportAllObjs)
    bpy.utils.register_class(View3dExportAllObjs_Panel)
    bpy.utils.register_class(ExportObjsProperties)

    bpy.types.Scene.export_objs_props = bpy.props.PointerProperty(type=ExportObjsProperties)

def unregister():
    bpy.utils.unregister_class(View3dExportAllObjs)
    bpy.utils.unregister_class(View3dExportAllObjs_Panel)
    bpy.utils.unregister_class(ExportObjsProperties)

    del bpy.types.Scene.export_objs_props

# This allows you to run the script directly from Blender's Text editor
# to test the add-on without having to install it.
if __name__ == "__main__":
    register()
    
    do('C://Users/Asda/Documents/programming/family_fracas/resources/test')