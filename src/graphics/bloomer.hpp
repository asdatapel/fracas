#pragma once

#include "framebuffer.hpp"
#include "../graphics.hpp"

struct Bloomer
{
    static const int LAYERS = 4;
    RenderTarget horizontal_blur_targets[LAYERS];
    RenderTarget vertical_blur_targets[LAYERS];

    Bloomer(uint32_t width, uint32_t height)
    {
        for (int layer = 0; layer < LAYERS; layer++)
        {
            horizontal_blur_targets[layer] = RenderTarget(width / pow(2, layer + 1), height / pow(2, layer + 1), TextureFormat::RGB16F, TextureFormat::NONE);
            vertical_blur_targets[layer] = RenderTarget(width / pow(2, layer + 1), height / pow(2, layer + 1), TextureFormat::RGB16F, TextureFormat::NONE);
        }
    }

    void do_bloom(RenderTarget hdr)
    {
        // downsample and blur
        for (int layer = 0; layer < LAYERS; layer++)
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
        // upsample and blur
        for (int layer = LAYERS - 2; layer >= 0; layer--)
        {
            RenderTarget &source = vertical_blur_targets[layer + 1];
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
    }

    RenderTarget get_final()
    {
        return vertical_blur_targets[0];
    }
};