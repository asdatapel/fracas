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
#include "scripts.hpp"
#include "main_menu.hpp"
#include "net/generated_rpc_client.hpp"
#include "net/net.hpp"
#include "platform.hpp"

#include "graphics/graphics_opengl.cpp"
#include "net/net.cpp"
#include "platform_windows.cpp"
#include "game/game_scripts.cpp"

Editor editor;

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

    main_memory.init(1024ull * 1024 * 1024 * 4);
    allocator.init(1024ull * 1024 * 1024 * 2);  // 2gb
    temp.init(1024 * 1024 * 100);               // 100 mb

    assets_allocator.init(&main_memory, 1024ull * 1024 * 1024 * 1);  // 1 gb
    assets_temp_allocator.init(&main_memory, 1024ull * 1024 * 1024);   // 1 gb
    scene_allocator.init(&main_memory, 1024ull * 1024 * 1024 * 1);   // 1 gb
    scene_temp_allocator.init(&main_memory, 1024ull * 1024 * 50);    // 50 mb

    editor.init(memory);
  }

  return true;
}

bool game_update(const float time_step, InputState *input_state, RenderTarget main_target)
{
  if (!init_if_not()) return false;

  editor.update_and_draw(main_target, input_state, memory);

  return true;
}

void deinit()
{
  // server.close();
  deinit_net();
}