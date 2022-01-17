#pragma once

#include "framebuffer.hpp"
#include "graphics.hpp"

struct Bloomer
{
    static const int N_LAYERS = 4;
    RenderTarget horizontal_blur_targets[N_LAYERS];
    RenderTarget vertical_blur_targets[N_LAYERS];
    // RenderTarget up_targets[N_LAYERS];

    Bloomer() {}
    Bloomer(uint32_t width, uint32_t height)
    {
        for (int layer = 0; layer < N_LAYERS; layer++)
        {
            horizontal_blur_targets[layer] = RenderTarget(width / pow(2, layer + 1), height / pow(2, layer + 1), TextureFormat::RGB16F, TextureFormat::NONE);
            vertical_blur_targets[layer] = RenderTarget(width / pow(2, layer + 1), height / pow(2, layer + 1), TextureFormat::RGB16F, TextureFormat::NONE);
            // up_targets[layer] = RenderTarget(width / pow(2, layer + 1), height / pow(2, layer + 1), TextureFormat::RGB16F, TextureFormat::NONE);
        }
    }

    void do_bloom(RenderTarget hdr)
    {
        // downsample and blur
        for (int layer = 0; layer < N_LAYERS; layer++)
        {
            RenderTarget &source = layer == 0 ? hdr : vertical_blur_targets[layer - 1];
            source.color_tex.gen_mipmaps();
            
            bind_shader(blur_shader);

            RenderTarget &hor = horizontal_blur_targets[layer];
            RenderTarget &ver = vertical_blur_targets[layer];

            hor.bind();
            hor.clear();
            bind_1i(blur_shader, UniformId::HORIZONTAL, 1);
            bind_texture(blur_shader, UniformId::SOURCE, source.color_tex);
            draw_rect();

            ver.bind();
            ver.clear();
            bind_1i(blur_shader, UniformId::HORIZONTAL, 0);
            bind_texture(blur_shader, UniformId::SOURCE, hor.color_tex);
            draw_rect();
        }
        // upsample and add
        // for (int layer = N_LAYERS - 2; layer >= 0; layer--)
        // {
        //     RenderTarget &dest = up_targets[layer];
        //     RenderTarget &source_a = vertical_blur_targets[layer];
        //     RenderTarget &source_b =
        //         layer == N_LAYERS - 2 ? vertical_blur_targets[layer + 1] : up_targets[layer + 1];
        //     source_a.color_tex.gen_mipmaps();
        //     source_b.color_tex.gen_mipmaps();

        //     bind_shader(add_shader);

        //     dest.bind();
        //     dest.clear();
        //     bind_texture(add_shader, UniformId::SOURCE_A, source_a.color_tex);
        //     bind_texture(add_shader, UniformId::SOURCE_B, source_b.color_tex);
        //     draw_rect();
        // }
    }

    RenderTarget get_final()
    {
        return vertical_blur_targets[N_LAYERS - 1];
    }
};