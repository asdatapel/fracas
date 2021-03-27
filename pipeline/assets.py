import os
import sys


shader_filepaths = ["/".join([root, f]) for root,dirs,files in os.walk("resources/shaders") for f in files if f.endswith(".gl")]
shader_filepaths += ["/".join([root, f]) for root,dirs,files in os.walk("./shaders") for f in files if f.endswith(".lib")]
font_filepaths = ["/".join([root, f]) for root,dirs,files in os.walk("resources/fonts") for f in files if f.endswith(".ttf")]

bitmap_filepaths = ["resources/default.bmp"]
materials = {}
obj_filepaths = []
meshes = {}

class Material:
    diffuse = bitmap_filepaths[0]
    normal = bitmap_filepaths[0]
    metal = bitmap_filepaths[0]
    roughness = bitmap_filepaths[0]
    emit = bitmap_filepaths[0]
    ao = bitmap_filepaths[0]

for model in os.listdir("resources/models"):
    dir = "resources/models/" + model
    files = [f for f in os.listdir(dir)]

    bmps = ["/".join([dir, f]) for f in files if f.endswith(".bmp")]
    bitmap_filepaths += bmps
    m = Material()
    if "diffuse.bmp" in files:
        m.diffuse = "/".join([dir, "diffuse.bmp"]) 
    if "normal.bmp" in files:
        m.normal ="/".join([dir, "normal.bmp"])
    if "metal.bmp" in files:
        m.metal = "/".join([dir, "metal.bmp"])
    if "roughness.bmp" in files:
        m.roughness = "/".join([dir,"roughness.bmp"])
    if "emit.bmp" in files:
        m.emit = "/".join([dir, "emit.bmp"])
    if "ao.bmp" in files:
        m.ao = "/".join([dir,"ao.bmp"])
    materials[dir] = m

    objs = ["/".join([dir, f]) for f in files if f.endswith(".obj")]
    obj_filepaths += objs
    meshes[dir] = objs


    
def filename_to_enum(v):
    return v.upper().replace('/', '_').replace('.', '_')
def texture_ref_to_enum(v):
    return 'TextureAssetId::' + v.upper().replace('/', '_').replace('.', '_')
def obj_ref_to_enum(v):
    return 'ObjId::' + v.upper().replace('/', '_').replace('.', '_')

textures_enum = ""
for bmp in bitmap_filepaths:
    textures_enum += filename_to_enum(bmp) + ",\n"
textures_list = ""
for bmp in bitmap_filepaths:
    textures_list += '"' + bmp + '"' + ',\n'
materials_enum = ""
for k in materials:
    materials_enum += filename_to_enum(k) + ",\n"
materials_list = ""
for k in materials:
    m = materials[k]
    materials_list += f"{{{texture_ref_to_enum(m.diffuse)}, {texture_ref_to_enum(m.normal)}, {texture_ref_to_enum(m.metal)}, {texture_ref_to_enum(m.roughness)}, {texture_ref_to_enum(m.emit)}, {texture_ref_to_enum(m.ao)}}},\n"



objs_enum = ""
for obj in obj_filepaths:
    objs_enum += filename_to_enum(obj) + ",\n"
objs_list = ""
for obj in obj_filepaths:
    objs_list += '"' + obj + '"' + ',\n'
meshes_enum = ""
for k in meshes:
    meshes_enum += filename_to_enum(k) + ",\n"
meshes_list = ""
for k in meshes:
    m = meshes[k]
    multiple_uvs = "true" if len(m) > 1 else "false"
    meshes_list += f"{{{multiple_uvs}, {{{','.join([obj_ref_to_enum(obj) for obj in m])}}}}},\n"

fonts_enum = ''
for font in font_filepaths:
    fonts_enum += filename_to_enum(font) + ",\n"
fonts_list = ""
for font in font_filepaths:
    fonts_list += '"' + font + '"' + ',\n'
    

out = f"""
#pragma once

enum struct TextureAssetId
{{
    {textures_enum}
}};
const static int TEXTURE_ASSET_COUNT = {len(bitmap_filepaths)};
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
const static int OBJ_ASSET_COUNT = {len(obj_filepaths)};
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

print(out)
outfile = open("generated/asset_definitions.hpp", 'w+')
outfile.write(out)