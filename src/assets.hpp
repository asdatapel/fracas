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

    StackAllocator allocator;
    StackAllocator temp_allocator;
};

Assets load_assets()
{
    Assets assets;
    assets.allocator.init(1024 * 1024 * 1024 * 2); // 2gb
    assets.temp_allocator.init(1024 * 1024 * 50);  // 50 mb

    for (int i = 0; i < TEXTURE_ASSET_COUNT; i++)
    {
        FileData tex_file = read_entire_file(TEXTURE_FILES[i], &assets.temp_allocator);
        Bitmap bmp = parse_bitmap(tex_file, &assets.allocator);

        Texture2D tex(bmp.width, bmp.height, TextureFormat::SRGB8_ALPHA8, true);
        tex.upload((uint8_t *)bmp.data, true);
        assets.textures[i] = tex;

        assets.temp_allocator.free(tex_file.data);
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
            FileData mesh_file1 = read_entire_file(OBJ_FILES[(int)def.objs[0]], &assets.allocator);
            FileData mesh_file2 = read_entire_file(OBJ_FILES[(int)def.objs[1]], &assets.allocator);
            assets.meshes[i] = load_obj_extra_uvs(mesh_file1, mesh_file2, &assets.allocator, &assets.temp_allocator);
        }
        else
        {
            FileData mesh_file = read_entire_file(OBJ_FILES[(int)def.objs[0]], &assets.allocator);
            assets.meshes[i] = load_obj(mesh_file, &assets.allocator, &assets.temp_allocator);
        }
        assets.vertex_buffers[i] = upload_vertex_buffer(assets.meshes[i]);
    }

    for (int i = 0; i < FONT_ASSET_COUNT; i++)
    {
        assets.font_files[i] = read_entire_file(FONT_FILES[i], &assets.allocator);
    }

    assets.temp_allocator.reset();
    return assets;
}