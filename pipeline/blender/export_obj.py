import bpy

def do(folder_path):
    for i, uvs in enumerate(bpy.context.active_object.data.uv_layers):
        uvs.active=True
        
        bpy.ops.export_scene.obj(filepath = folder_path + 'model_' + str(i) + '.obj', 
            check_existing=False, filter_glob='*.obj;*.mtl', 
            use_selection=True, use_animation=False, 
            use_mesh_modifiers=True, use_edges=True, 
            use_smooth_groups=False, use_smooth_groups_bitflags=False, 
            use_normals=True, use_uvs=True, use_materials=False, 
            use_triangles=True, use_nurbs=False, use_vertex_groups=False, 
            use_blen_objects=True, group_by_object=False, 
            group_by_material=False, keep_vertex_order=False, 
            global_scale=1.0, path_mode='AUTO', axis_forward='-Z', axis_up='Y')
            
            
class MyProperties(bpy.types.PropertyGroup):
    directory : bpy.props.StringProperty(maxlen=1024, subtype='DIR_PATH', options={'HIDDEN', 'SKIP_SAVE'})

class View3dExportAllObjs(bpy.types.Operator):
    """Export OBJs with all UVs"""      # Use this as a tooltip for menu items and buttons.
    bl_idname = "view3d.export_all_objs"        # Unique identifier for buttons and menu items to reference.
    bl_label = "Export All OBJs"         # Display name in the interface.
    bl_options = {'REGISTER', 'UNDO'}  # Enable undo for the operator.

    def execute(self, context):        # execute() is called when running the operator.
        my_props = context.scene.my_props
        
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
        
        my_props = context.scene.my_props
        
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
    bpy.utils.register_class(MyProperties)

    bpy.types.Scene.my_props = bpy.props.PointerProperty(type=MyProperties)

def unregister():
    bpy.utils.unregister_class(View3dExportAllObjs)
    bpy.utils.unregister_class(View3dExportAllObjs_Panel)
    bpy.utils.unregister_class(MyProperties)

    del bpy.types.Scene.my_props

# This allows you to run the script directly from Blender's Text editor
# to test the add-on without having to install it.
if __name__ == "__main__":
    register()