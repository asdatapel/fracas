#pragma once

#include <asset_definitions.hpp>
#include "font.hpp"
#include "graphics.hpp"
#include "material.hpp"

struct Assets
{
    Texture textures[TEXTURE_ASSET_COUNT];
    StandardPbrMaterial materials[MATERIAL_COUNT];

    Mesh meshes[MESH_COUNT];
    VertexBuffer vertex_buffers[MESH_COUNT];

    FileData font_files[FONT_ASSET_COUNT];
};

Assets load_assets()
{
    Assets assets;
    for (int i = 0; i < TEXTURE_ASSET_COUNT; i++)
    {
        assets.textures[i] = to_texture(parse_bitmap(read_entire_file(TEXTURE_FILES[i])), true);
    }
    for (int i = 0; i < MATERIAL_COUNT; i++)
    {
        assets.materials[i].texture_array[0] = assets.textures[(int)MATERIAL_DEFINITIONS[i][0]];
        assets.materials[i].texture_array[1] = assets.textures[(int)MATERIAL_DEFINITIONS[i][1]];
        assets.materials[i].texture_array[2] = assets.textures[(int)MATERIAL_DEFINITIONS[i][2]];
        assets.materials[i].texture_array[3] = assets.textures[(int)MATERIAL_DEFINITIONS[i][3]];
        assets.materials[i].texture_array[4] = assets.textures[(int)MATERIAL_DEFINITIONS[i][4]];
        assets.materials[i].texture_array[5] = assets.textures[(int)MATERIAL_DEFINITIONS[i][5]];
    }

    for (int i = 0; i < MESH_COUNT; i++)
    {
        MeshDefinition def = MESH_DEFINITIONS[i];
        if (def.multiple_uvs)
        {
            assets.meshes[i] = load_obj_extra_uvs(OBJ_FILES[(int)def.objs[0]], OBJ_FILES[(int)def.objs[1]]);
        }
        else
        {
            assets.meshes[i] = load_obj(OBJ_FILES[(int)def.objs[0]]);
        }
        assets.vertex_buffers[i] = upload_vertex_buffer(assets.meshes[i]);
    }

    for (int i = 0; i < FONT_ASSET_COUNT; i++)
    {
        assets.font_files[i] = read_entire_file(FONT_FILES[i]);
    }

    return assets;
}