#pragma once

#include <vector>

#include "../scene/entity.hpp"
#include "../common.hpp"
#include "../util.hpp"

struct ScriptDefinition {
  struct InputDef {
    String name;
    int *value;
    EntityType entity_type;
  };

  String name;
  std::vector<InputDef> inputs;
};

struct ScriptData;
struct Scripts {
  ScriptData *script_data;

  void init();
  void reset();
  void update(float timestep);
  std::vector<ScriptDefinition> get_script_defs();
};

#include "game/game_scripts.cpp"