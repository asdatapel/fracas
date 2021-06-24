import os
import sys


shader_filepaths = ["/".join([root, f]) for root,dirs,files in os.walk("resources/shaders") for f in files if f.endswith(".gl")]
shader_filepaths += ["/".join([root, f]) for root,dirs,files in os.walk("./shaders") for f in files if f.endswith(".lib")]
font_filepaths = ["/".join([root, f]) for root,dirs,files in os.walk("resources/fonts") for f in files if f.endswith(".ttf")]

bitmaps = {"DEFAULT": "resources/default.bmp"}
materials = {}
objs = {}
meshes = {}

def to_enum(v):
    return v.upper().replace('/', '_').replace('.', '_')

class Material:
    diffuse = "DEFAULT"
    normal = "DEFAULT"
    metal = "DEFAULT"
    roughness = "DEFAULT"
    emit = "DEFAULT"
    ao = "DEFAULT"

class Mesh:
    objs = []

    def __init__(self, objs):
        self.objs = objs

for dir in os.listdir("resources/models"):
    full_dir = "resources/models/" + dir
    files = [f for f in os.listdir(full_dir)]

    m = Material()
    for f in files:
        if f.endswith(".bmp"):
            bmp_id = to_enum(dir + '/' + f)
            bitmaps[bmp_id] = full_dir + '/' + f
            
            if f == "diffuse.bmp":
                m.diffuse = bmp_id
            if f == "normal.bmp":
                m.normal = bmp_id
            if f == "metal.bmp":
                m.metal = bmp_id
            if f == "roughness.bmp":
                m.roughness = bmp_id
            if f == "emit.bmp":
                m.emit = bmp_id
            if f == "ao.bmp":
                m.ao = bmp_id
    material_id = to_enum(dir)
    materials[material_id] = m

    this_objs = []
    for f in files:
        if f.endswith(".obj"):
            obj_id = to_enum(dir + '/' + f)
            this_objs.append(obj_id)
            objs[obj_id] = full_dir + '/' + f

    mesh_id = to_enum(dir)
    meshes[mesh_id] = Mesh(this_objs)


    
def texture_ref_to_enum(v):
    return 'TextureAssetId::' + v.upper().replace('/', '_').replace('.', '_')
def obj_ref_to_enum(v):
    return 'ObjId::' + v.upper().replace('/', '_').replace('.', '_')

textures_enum = ""
for bmp in bitmaps:
    textures_enum += bmp + ",\n"
textures_list = ""
for bmp in bitmaps:
    textures_list += '"' + bitmaps[bmp] + '"' + ',\n'
materials_enum = ""
for k in materials:
    materials_enum += k + ",\n"
materials_list = ""
for k in materials:
    m = materials[k]
    materials_list += f"{{{'TextureAssetId::' + m.diffuse}, {'TextureAssetId::' + m.normal}, {'TextureAssetId::' + m.metal}, {'TextureAssetId::' + m.roughness}, {'TextureAssetId::' + m.emit}, {'TextureAssetId::' + m.ao}}},\n"


objs_enum = ""
for obj in objs:
    objs_enum += obj + ",\n"
objs_list = ""
for obj in objs:
    objs_list += '"' + objs[obj] + '"' + ',\n'
meshes_enum = ""
for mesh in meshes:
    meshes_enum += mesh + ",\n"
meshes_list = ""
for k in meshes:
    m = meshes[k]
    multiple_uvs = "true" if len(m.objs) > 1 else "false"
    meshes_list += f"{{{multiple_uvs}, {{{','.join([obj_ref_to_enum(obj) for obj in m.objs])}}}}},\n"

fonts_enum = ''
for font in font_filepaths:
    fonts_enum += to_enum(font) + ",\n"
fonts_list = ""
for font in font_filepaths:
    fonts_list += '"' + font + '"' + ',\n'
    

out = f"""
#pragma once

enum struct TextureAssetId
{{
    {textures_enum}
}};
const static int TEXTURE_ASSET_COUNT = {len(bitmaps)};
constexpr static char TEXTURE_FILES[TEXTURE_ASSET_COUNT][128] = {{
    {textures_list}
}};
enum struct MaterialId
{{
    {materials_enum}
}};
const static int MATERIAL_COUNT = {len(materials)};
constexpr static TextureAssetId MATERIAL_DEFINITIONS[MATERIAL_COUNT][6] = {{
    {materials_list}
}};

enum struct ObjId
{{
    {objs_enum}
}};
const static int OBJ_ASSET_COUNT = {len(objs)};
constexpr static char OBJ_FILES[OBJ_ASSET_COUNT][128] = {{
    {objs_list}
}};
enum struct MeshId
{{
    {meshes_enum}
}};
const static int MESH_COUNT = {len(meshes)};
struct MeshDefinition
{{
    bool multiple_uvs;
    ObjId objs[2];
}};
constexpr static MeshDefinition MESH_DEFINITIONS[MESH_COUNT] = {{
    {meshes_list}
}};

enum struct FontId
{{
    {fonts_enum}
}};
const static int FONT_ASSET_COUNT = {len(font_filepaths)};
constexpr static char FONT_FILES[FONT_ASSET_COUNT][128] = {{
    {fonts_list}
}};
"""

outfile = open("generated/asset_definitions.hpp", 'w+')
outfile.write(out)