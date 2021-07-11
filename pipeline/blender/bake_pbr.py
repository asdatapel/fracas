import bpy

import os
from pathlib import Path


def bake_pbr(obj, folder, img_size):
    if not obj.data.materials:
        return

    bpy.context.scene.render.engine = 'CYCLES'
    
    metal_input = 4
    roughness_input = 7
    
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    obj.data.uv_layers[0].active=True
    
    # You can choose your texture size (This will be the de bake image)
    image_name = obj.name + '_BakedTexture'
    if image_name not in bpy.data.images:
        img = bpy.data.images.new(image_name, img_size, img_size, alpha=True)
    
    img = bpy.data.images[image_name]
    img.generated_width = img_size
    img.generated_height = img_size
    bpy.context.scene.render.image_settings.color_mode = 'RGBA'
    bpy.context.scene.render.image_settings.file_format='PNG'

    storage = {}
    for mat in obj.data.materials:
        mat.use_nodes = True 
        nodes = mat.node_tree.nodes
        
        # add node + image
        texture_node = nodes.new('ShaderNodeTexImage')
        texture_node.name = 'Bake_node'
        texture_node.select = True
        nodes.active = texture_node
        texture_node.image = img # Assign the image to the node
        
        # get material
        roughness = nodes["Principled BSDF"].inputs[roughness_input].default_value
        metal = nodes["Principled BSDF"].inputs[metal_input].default_value
        storage[mat] = ({'r': roughness, 'm': metal})
        

    for mat in obj.data.materials:        
        nodes = mat.node_tree.nodes
        bsdf = nodes["Principled BSDF"]
        print('start', bsdf.inputs[metal_input].default_value)
        bsdf.inputs[metal_input].default_value = 0
        print('end', bsdf.inputs[metal_input].default_value)

    bpy.ops.object.bake(type='DIFFUSE', pass_filter={'COLOR'}, save_mode='EXTERNAL', use_clear = True)
    img.save_render(filepath=folder + 'diffuse.png')
        
    
    img = bpy.data.images.new(image_name, img_size, img_size, is_data=True)
    for mat in obj.data.materials:
        mat.use_nodes = True 
        nodes = mat.node_tree.nodes
        texture_node = nodes.new('ShaderNodeTexImage')
        texture_node.select = True
        nodes.active = texture_node
        texture_node.image = img
    
    bpy.ops.object.bake(type='ROUGHNESS', save_mode='EXTERNAL')
    img.save_render(filepath=folder + 'roughness.png')

    for mat in obj.data.materials:
        mat.use_nodes = True 
        nodes = mat.node_tree.nodes
        
        bsdf = nodes["Principled BSDF"]
        bsdf.inputs[roughness_input].default_value = storage[mat]['m']
    bpy.ops.object.bake(type='ROUGHNESS', save_mode='EXTERNAL')
    img.save_render(filepath=folder + 'metal.png')


    bpy.ops.object.bake(type='EMIT', save_mode='EXTERNAL')
    img.save_render(filepath=folder + 'emit.png')

    bpy.ops.object.bake(type='NORMAL', save_mode='EXTERNAL')
    img.save_render(filepath=folder + 'normal.png')

    bpy.ops.object.bake(type='AO', save_mode='EXTERNAL')
    img.save_render(filepath=folder + 'ao.png')

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

def do(parent_folder, img_size):    
    meshes = {}
    for obj in bpy.context.selected_objects: 
        if obj.type == 'MESH':
            meshes[obj.data.name] = obj
            
    for m in meshes:
        bpy.ops.object.select_all(action='DESELECT')
        obj = meshes[m]
        
        folder = os.path.join(parent_folder, m)
        Path(folder).mkdir(parents=True, exist_ok=True)
        
        bake_pbr(obj, folder + '/', img_size)
        os.system('python3 C:\\Users\\Asda\\Documents\\programming\\family_fracas\\pipeline\\png_to_bmp.py ' + folder)
                
    return {'FINISHED'}
                
                
class BakePbrProperties(bpy.types.PropertyGroup):
    directory : bpy.props.StringProperty(maxlen=1024, subtype='DIR_PATH', options={'HIDDEN', 'SKIP_SAVE'})
    tex_size : bpy.props.IntProperty(name='size', min=64, max=5096, default = 256)
    


class BakePbr(bpy.types.Operator):
    """Bake PBR textures"""      # Use this as a tooltip for menu items and buttons.
    bl_idname = "view3d.bake_pbr"        # Unique identifier for buttons and menu items to reference.
    bl_label = "Bake PBR"         # Display name in the interface.
    bl_options = {'REGISTER', 'UNDO'}  # Enable undo for the operator.

    def execute(self, context):        # execute() is called when running the operator.
        my_props = context.scene.bake_pbr_props
        
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
        
        my_props = context.scene.bake_pbr_props
        
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
    bpy.utils.register_class(BakePbrProperties)
    
    bpy.types.Scene.bake_pbr_props = bpy.props.PointerProperty(type=BakePbrProperties)


def unregister():
    bpy.utils.unregister_class(BakePbr)
    bpy.utils.unregister_class(Panel)
    bpy.utils.unregister_class(BakePbrProperties)
    
    del bpy.types.Scene.bake_pbr_props


# This allows you to run the script directly from Blender's Text editor
# to test the add-on without having to install it.
if __name__ == "__main__":
    register()