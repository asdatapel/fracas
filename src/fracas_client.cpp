#include <stdio.h>
#include <mutex>
#include <thread>

#include <winsock2.h>
// stupid bullshit
#undef min
#undef max

#include <ws2tcpip.h>

#include "assets.hpp"
#include "editor.hpp"
#include "game_scripts.hpp"
#include "main_menu.hpp"
#include "net/generated_rpc_client.hpp"
#include "net/net.hpp"
#include "platform.hpp"

#include "graphics/graphics_opengl.cpp"
#include "net/net.cpp"
#include "platform_windows.cpp"

Peer server;

Editor editor;
Assets assets;
SceneManager scenes;

StackAllocator main_memory;
StackAllocator allocator;
StackAllocator temp;
Memory memory{&allocator, &temp};

float animation_wait = 0.f;

bool init_if_not()
{
  static bool initted = false;
  if (!initted) {
    initted = true;

    init_net();
    server.open("127.0.0.1", 6519, false);

    main_memory.init(1024ull * 1024 * 1024 * 4);
    allocator.init(1024ull * 1024 * 1024 * 2);  // 2gb
    temp.init(1024 * 1024 * 100);               // 100 mb

    assets_allocator.init(&main_memory, 1024ull * 1024 * 1024 * 1);  // 1 gb
    assets_temp_allocator.init(&main_memory, 1024ull * 1024 * 50);   // 50 mb
    scene_allocator.init(&main_memory, 1024ull * 1024 * 1024 * 1);   // 1 gb
    scene_temp_allocator.init(&main_memory, 1024ull * 1024 * 50);    // 50 mb

    assets.init();
    assets.load("resources/test/main_assets.yaml", &main_memory);
    assets.load("resources/test/assets_out.yaml");

    scenes.init(&assets, memory);
    editor.init(&scenes, &assets, memory);

    Imm::init();
  }

  return true;
}

struct ClientData {
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
    main_menu.main     = &main;
    main_menu.settings = &settings;
    main_menu.create   = &create;
    main_menu.join     = &join;
    main_menu.lobby    = &lobby;
    main_menu.current  = main_menu.main;
  }
};

bool game_update(const float time_step, InputState *input_state, RenderTarget main_target)
{
  if (!init_if_not()) return false;

  static RpcClient client("127.0.0.1", 6666);
  static ClientData client_data(&assets, &client, memory);
  static Game game;

  static bool inited = false;
  if (!inited) {
    inited = true;

    client.client_data = &client_data;
  }

  int msg_len;
  char msg[MAX_MSG_SIZE];
  if ((msg_len = client.peer.recieve_msg(msg)) > 0) {
    if (client.handle_rpc(msg, msg_len)) {
      client.peer.pop_message();
    }
  }

  if (client_data.main_menu.current) {
    main_target.bind();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    {
      // background
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
    client_data.main_menu.current->update_and_draw(main_target, input_state, &client_data.main_menu,
                                                   &game.game_data);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
  } else {
    editor.update_and_draw(&assets, &client, main_target, input_state, memory);
  }

  return true;
}

void deinit()
{
  server.close();
  deinit_net();
}