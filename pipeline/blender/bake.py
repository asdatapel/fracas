import bpy

def do(filepath, img_size):
    bpy.context.scene.render.engine = 'CYCLES'
    
    metal_input = 4
    roughness_input = 7

    if len(bpy.context.selected_objects) == 0:
        return {'CANCELLED'}
    obj = bpy.context.selected_objects[0]
    bpy.context.active_object.data.uv_layers[0].active=True

    # You can choose your texture size (This will be the de bake image)
    image_name = obj.name + '_BakedTexture'
    img = bpy.data.images.new(image_name, img_size, img_size, alpha=True)
    bpy.context.scene.render.image_settings.color_mode = 'RGBA'
    bpy.context.scene.render.image_settings.file_format='PNG'

    bpy.context.view_layer.objects.active = obj

    storage = {}
       
    #Due to the presence of any multiple materials, it seems necessary to iterate on all the materials, and assign them a node + the image to bake.
    for mat in obj.data.materials:
        # Here it is assumed that the materials have been created with nodes,
        # otherwise it would not be possible to assign a node for the Bake, so this step is a bit useless
        mat.use_nodes = True 
        nodes = mat.node_tree.nodes
        
        # add node + image
        texture_node =nodes.new('ShaderNodeTexImage')
        texture_node.name = 'Bake_node'
        texture_node.select = True
        nodes.active = texture_node
        texture_node.image = img #Assign the image to the node
        
        #get material
        roughness = nodes["Principled BSDF"].inputs[roughness_input].default_value
        metal = nodes["Principled BSDF"].inputs[metal_input].default_value
        storage[mat] = ({'r': roughness, 'm': metal})
        

    for mat in obj.data.materials:
        mat.use_nodes = True 
        nodes = mat.node_tree.nodes
        
        bsdf = nodes["Principled BSDF"]
        bsdf.inputs[metal_input].default_value = 0
        

    bpy.ops.object.bake(type='DIFFUSE', pass_filter={'COLOR'}, save_mode='EXTERNAL', use_clear = True)
    img.save_render(filepath=filepath + 'diffuse.png')
        
    
    img = bpy.data.images.new(image_name, img_size, img_size, is_data=True)
    for mat in obj.data.materials:
        mat.use_nodes = True 
        nodes = mat.node_tree.nodes
        texture_node = nodes.new('ShaderNodeTexImage')
        texture_node.select = True
        nodes.active = texture_node
        texture_node.image = img
    
    bpy.ops.object.bake(type='ROUGHNESS', save_mode='EXTERNAL')
    img.save_render(filepath=filepath + 'roughness.png')

    for mat in obj.data.materials:
        mat.use_nodes = True 
        nodes = mat.node_tree.nodes
        
        bsdf = nodes["Principled BSDF"]
        bsdf.inputs[roughness_input].default_value = storage[mat]['m']
    bpy.ops.object.bake(type='ROUGHNESS', save_mode='EXTERNAL')
    img.save_render(filepath=filepath + 'metal.png')


    bpy.ops.object.bake(type='EMIT', save_mode='EXTERNAL')
    img.save_render(filepath=filepath + 'emit.png')

    bpy.ops.object.bake(type='NORMAL', save_mode='EXTERNAL')
    img.save_render(filepath=filepath + 'normal.png')

    bpy.ops.object.bake(type='AO', save_mode='EXTERNAL')
    img.save_render(filepath=filepath + 'ao.png')

    # reset materials
    for mat in obj.data.materials:
        mat.use_nodes = True 
        nodes = mat.node_tree.nodes
        
        bsdf = nodes["Principled BSDF"]
        bsdf.inputs[metal_input].default_value = storage[mat]['m']
        bsdf.inputs[roughness_input].default_value = storage[mat]['r']
        
    #In the last step, we are going to delete the nodes we created earlier
    for mat in obj.data.materials:
        for n in mat.node_tree.nodes:
            if n.name == 'Bake_node':
                mat.node_tree.nodes.remove(n)
                
    return {'FINISHED'}
                
                
class MyProperties(bpy.types.PropertyGroup):
    directory : bpy.props.StringProperty(maxlen=1024, subtype='DIR_PATH', options={'HIDDEN', 'SKIP_SAVE'})
    tex_size : bpy.props.IntProperty(name='size', min=64, max=5096, default = 256)
    


class BakePbr(bpy.types.Operator):
    """Bake PBR textures"""      # Use this as a tooltip for menu items and buttons.
    bl_idname = "view3d.bake_pbr"        # Unique identifier for buttons and menu items to reference.
    bl_label = "Bake PBR"         # Display name in the interface.
    bl_options = {'REGISTER', 'UNDO'}  # Enable undo for the operator.

    def execute(self, context):        # execute() is called when running the operator.
        my_props = context.scene.my_props
        
        filepath_full = bpy.path.abspath(my_props.directory)
        return do(filepath_full, my_props.tex_size)
    
class Panel(bpy.types.Panel):
    bl_idname = "view3d.bake_pbr_panel"
    bl_label = "Bake PBR"
    bl_category = "Export"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    
    def draw(self, context):
        layout = self.layout
        
        my_props = context.scene.my_props
        
        layout.prop(my_props, "directory")
        layout.prop(my_props, "tex_size")
        layout.operator("view3d.bake_pbr", text="Bake")
        
            
bl_info = {
    "name": "Bake PBR",
    "blender": (2, 80, 0),
    "category": "View3D",
}

def register():
    bpy.utils.register_class(BakePbr)
    bpy.utils.register_class(Panel)
    bpy.utils.register_class(MyProperties)
    
    bpy.types.Scene.my_props = bpy.props.PointerProperty(type=MyProperties)


def unregister():
    bpy.utils.unregister_class(BakePbr)
    bpy.utils.unregister_class(Panel)
    bpy.utils.unregister_class(MyProperties)
    
    del bpy.types.Scene.my_props


# This allows you to run the script directly from Blender's Text editor
# to test the add-on without having to install it.
if __name__ == "__main__":
    register()