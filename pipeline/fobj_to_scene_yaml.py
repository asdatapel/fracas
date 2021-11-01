from os import path
import yaml
import numpy as np
from scipy.spatial.transform import Rotation as R


scene_root_path = 'resources/scenes/test'

meshes = {}
textures = {}
materials = {}
objects = []

def add_mesh(folder, file):
    global meshes

    p = path.join(folder, file).replace("\\","/")
    if p in meshes:
        return meshes[p]['id']

    id = len(meshes)
    meshes[p] = {
        'id': id,
        'path': p,
    }
    return id

def add_texture(folder, file, format):
    global textures

    p = path.join(folder, file).replace("\\","/")
    if p in textures:
        return textures[p]['id']

    id = len(textures)
    textures[p] = {
        'id': id,
        'path': p,
        'format': format,
    }
    return id

def add_material(folder):
    global materials

    folder = folder.replace("\\","/")

    if folder in materials:
        return materials[folder]['id']

    textures = [
       add_texture(folder, 'diffuse.bmp', 'SRGB8_ALPHA8'),
       add_texture(folder, 'normal.bmp', 'RGBA8'),
       add_texture(folder, 'metal.bmp', 'RGBA8'),
       add_texture(folder, 'roughness.bmp', 'RGBA8'),
       add_texture(folder, 'emit.bmp', 'SRGB8_ALPHA8'),
       add_texture(folder, 'ao.bmp', 'RGBA8'),
    ]

    id = len(materials)
    materials[folder] = {
        'id': id,
        'name': folder,
        'textures': textures,
    }
    return id

def read_object(folder, o):
    id = len(objects)

    result = {}
    result['id'] = id
    result['name'] = o['name']
    result['position'] = {'x': o['position']['x'], 'y': o['position']['y'], 'z': o['position']['z']}
    result['rotation'] = {'x': o['rotation']['x'], 'y': o['rotation']['y'], 'z': o['position']['z']}
    result['scale'] = {'x': o['scale']['x'], 'y': o['scale']['y'], 'z': o['scale']['z']}
    result['type'] = o['type']

    if result['type'] == 'MESH':
        result['mesh'] = {
            'mesh': add_mesh(folder, o['data']),
            'material': add_material(path.join(folder, o['material'])),
        }
    elif result['type'] == 'LIGHT':
        result['spotlight'] = {
            'color':  {'x': o['light']['color']['x'], 'y': o['light']['color']['y'], 'z': o['light']['color']['z']},
            'inner_angle': o['light']['inner_angle'],
            'outer_angle': o['light']['outer_angle'],
        }
    
    return result

def load_collection(fobj, root):
    global objects

    root_pos_vec = np.array([root['position']['x'], root['position']['y'], root['position']['z']])
    root_rot_vec = np.array([root['rotation']['x'], root['rotation']['y'], root['rotation']['z']])
    root_scale_vec = np.array([root['scale']['x'], root['scale']['y'], root['scale']['z']])

    f = path.join(scene_root_path, fobj, fobj + '.fobj')
    with open(f, 'r') as stream:
        data = yaml.safe_load(stream)
        for o in data['objects']:
            obj = read_object(path.join(scene_root_path, fobj), o)

            if len(data['objects']) == 1:
                obj['name'] = root['name']
            else:
                obj['name'] = root['name']+'.'+obj['name']

            pos_vec = np.array([obj['position']['x'], obj['position']['y'], obj['position']['z']])
            final_pos_vec = root_pos_vec + R.from_euler('xyz', root_rot_vec, degrees=False).apply(pos_vec)
            obj['position'] = {'x': final_pos_vec[0].item(), 'y': final_pos_vec[1].item(), 'z': final_pos_vec[2].item()}

            rot_vec = np.array([obj['rotation']['x'], obj['rotation']['y'], obj['rotation']['z']])
            final_rot_vec = root_rot_vec + rot_vec
            obj['rotation'] = {'x': final_rot_vec[0].item(), 'y': final_rot_vec[1].item(), 'z': final_rot_vec[2].item()}

            scale_vec = np.array([obj['scale']['x'], obj['scale']['y'], obj['scale']['z']])
            final_scale_vec = root_scale_vec * scale_vec
            obj['scale'] = {'x': final_scale_vec[0].item(), 'y': final_scale_vec[1].item(), 'z': final_scale_vec[2].item()}

            objects.append(obj)

with open('resources/scenes/test/set.fobj', 'r') as stream:
    data = yaml.safe_load(stream)
    for o in data['objects']:
        obj = read_object(scene_root_path, o)
        if (obj['type'] == 'COLLECTION'):
            load_collection(o['collection'], obj)
        else:
            objects.append(obj)


with open('resources/test/test.yaml', 'r') as stream:
    data = yaml.safe_load(stream)
    for i, obj in enumerate(objects):
        data['entities'][i]['name'] = obj['name']
        data['entities'][i]['type'] = obj['type']
        # data['entities'][i]['transform'] = {
        #     'position': data['entities'][i]['position'],
        #     'rotation': data['entities'][i]['rotation'],
        #     'scale': data['entities'][i]['scale'],
        # }
        # del data['entities'][i]['position']
        # del data['entities'][i]['rotation']
        # del data['entities'][i]['scale']

        if obj['type'] == 'MESH':
            data['entities'][i]['mesh'] = obj['mesh']
        elif obj['type'] == 'LIGHT':
            data['entities'][i]['spotlight'] = obj['spotlight']
            del data['entities'][i]['light']
    
    output = open('resources/test/main.yaml', 'w+')
    output.write(yaml.dump(data))
    output.close()


output = open('resources/test/main_assets.yaml', 'w+')
output.write(yaml.dump({ 'assets': {
    'meshes': [meshes[k] for k in meshes], 
    'textures': [textures[k] for k in textures], 
    'materials': [materials[k] for k in materials],
}}))
output.close()