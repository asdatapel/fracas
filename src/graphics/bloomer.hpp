#pragma once

#include "framebuffer.hpp"
#include "graphics.hpp"

struct Bloomer
{
    static const int N_LAYERS = 6;
    RenderTarget horizontal_blur_targets[N_LAYERS];
    RenderTarget vertical_blur_targets[N_LAYERS];
    RenderTarget up_targets[N_LAYERS];

    Bloomer() {}
    Bloomer(uint32_t width, uint32_t height)
    {
        for (int layer = 0; layer < N_LAYERS; layer++)
        {
            horizontal_blur_targets[layer] = RenderTarget(width / pow(2, layer + 1), height / pow(2, layer), TextureFormat::RGB16F, TextureFormat::NONE);
            vertical_blur_targets[layer] = RenderTarget(width / pow(2, layer + 1), height / pow(2, layer + 1), TextureFormat::RGB16F, TextureFormat::NONE);
            up_targets[layer] = RenderTarget(width / pow(2, layer + 1), height / pow(2, layer + 1), TextureFormat::RGB16F, TextureFormat::NONE);
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
            hor.color_tex.gen_mipmaps();

            // FIXME theres one vertical one extra happening
            ver.bind();
            ver.clear();
            bind_1i(blur_shader, UniformId::HORIZONTAL, 0);
            bind_texture(blur_shader, UniformId::SOURCE, hor.color_tex);
            draw_rect();
            ver.color_tex.gen_mipmaps();
        }
        // upsample and add
        for (int layer = N_LAYERS - 2; layer >= 0; layer--)
        {
            RenderTarget &dest = up_targets[layer];
            RenderTarget &source_a =
                layer == N_LAYERS - 2 ? vertical_blur_targets[layer + 1] : up_targets[layer + 1];
            RenderTarget &source_b = vertical_blur_targets[layer];

            bind_shader(add_shader);

            dest.bind();
            dest.clear();
            bind_texture(add_shader, UniformId::SOURCE_A, source_a.color_tex);
            bind_texture(add_shader, UniformId::SOURCE_B, source_b.color_tex);
            draw_rect();
            dest.color_tex.gen_mipmaps();
        }
    }

    RenderTarget get_final()
    {
        return up_targets[0];
    }
};