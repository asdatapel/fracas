#include <stdio.h>
#include <mutex>
#include <thread>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "assets.hpp"
#include "camera.hpp"
#include "debug_ui.hpp"
#include "font.hpp"
#include "game_scripts.hpp"
#include "mesh.hpp"
#include "main_menu.hpp"
#include "net/net.hpp"
#include "net/generated_rpc_client.hpp"
#include "platform.hpp"
#include "scene/scene.hpp"
#include "editor.hpp"
#include "ui.hpp"

#include "graphics/graphics_opengl.cpp"
#include "platform_windows.cpp"
#include "net/net.cpp"

Peer server;

Assets assets;
Scene scene;
Editor editor;
Font ui_font;
Assets2 assets2;
Scene2 x_scene;

StackAllocator allocator;
StackAllocator temp;
Memory memory{&allocator, &temp};

float animation_wait = 0.f;

bool init_if_not()
{
    static bool initted = false;
    if (!initted)
    {
        initted = true;

        init_net();
        server.open("127.0.0.1", 6519, false);

        allocator.init(1024ull * 1024 * 1024 * 2); // 2gb
        temp.init(1024 * 1024 * 100);              // 50 mb
        assets.init(memory);
        scene.init(memory);
        scene.load(&assets, memory);
        ui_font = load_font(assets.font_files[(int)FontId::RESOURCES_FONTS_ROBOTOCONDENSED_LIGHT_TTF], 32, memory.temp);

        assets2.load("resources/test/assets.yaml", memory);
        x_scene.init(memory, TextureFormat::RGBA16F);
        x_scene.load("resources/test/eeegghhh_scene.yaml", &assets2, memory);

        imm_init(&assets, memory);

        // MainMenu::init(&assets);
    }

    return true;
}

struct ClientData
{
    MainMenu main_menu;
    MainPage main;
    SettingsPage settings;
    CreateGamePage create;
    JoinGamePage join;
    LobbyPage lobby;

    ClientData(Assets *assets, RpcClient *client, Memory memory)
        : main(assets, memory),
          settings(assets, client, memory),
          create(assets, client, memory),
          join(assets, client, memory),
          lobby(assets, client, memory)
    {
        main_menu.main = &main;
        main_menu.settings = &settings;
        main_menu.create = &create;
        main_menu.join = &join;
        main_menu.lobby = &lobby;
        main_menu.current = main_menu.main;
    }
};

bool game_update(const float time_step, InputState *input_state, RenderTarget main_target)
{
    if (!init_if_not())
        return false;

    static RpcClient client("127.0.0.1", 6666);
    static ClientData client_data(&assets, &client, memory);
    static Game game;

    static bool inited = false;
    if (!inited)
    {
        inited = true;

        client.client_data = &client_data;
        game.init({&scene, &x_scene});
    }

    int msg_len;
    char msg[MAX_MSG_SIZE];
    if ((msg_len = client.peer.recieve_msg(msg)) > 0)
    {
        if (client.handle_rpc(msg, msg_len))
        {
            client.peer.pop_message();
        }
    }

    if (client_data.main_menu.current)
    {
        main_target.bind();

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        { // background
            static float t = 0;
            t += time_step;
            bind_shader(blurred_colors_shader);
            bind_1f(blurred_colors_shader, UniformId::T, t);
            bind_2i(blurred_colors_shader, UniformId::RESOLUTION, main_target.width, main_target.height);
            bind_2f(blurred_colors_shader, UniformId::POS, 0, 0);
            bind_2f(blurred_colors_shader, UniformId::SCALE, main_target.width, main_target.height);
            draw_rect();
        }

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        client_data.main_menu.current->update_and_draw(main_target, input_state, &client_data.main_menu, &game.game_data);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }
    else
    {
        editor.update_and_draw(&scene, &x_scene, &game, &client, main_target, input_state, memory);
        x_scene.visible = true;
        BoardController board_controller;
        board_controller.flip(&assets2, 0, "", 0);
        if (x_scene.visible)
        {
            x_scene.update_and_draw(nullptr);
            main_target.bind();
            glEnable(GL_BLEND);
            glDisable(GL_DEPTH_TEST);
            draw_textured_rect(main_target, {0, 0, 1920, 1080}, {}, x_scene.target.color_tex);
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }
    }

    return true;
}

void deinit()
{
    server.close();
    deinit_net();
}