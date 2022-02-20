#pragma once

#include "board_controller.hpp"
#include "game_state.hpp"
#include "net/generated_rpc_client.hpp"
#include "scene/scene.hpp"
#include "spline.hpp"

// stupid shit
#define CAT_(a, b) a##b
#define CAT(a, b) CAT_(a, b)
#define VARNAME(Var) CAT(Var, __LINE__)
#define step(...)                       \
  static i32 VARNAME(step) = next_step; \
  if (next_step == VARNAME(step)) {     \
    next_step++;                        \
    __VA_ARGS__;                        \
  }
#define yield_wait(time)           \
  static f32 VARNAME(wait) = time; \
  step(VARNAME(wait) = time;);     \
  if (VARNAME(wait) > 0.f) {       \
    VARNAME(wait) -= timestep;     \
    return false;                  \
  }

struct Scenes {
  Scene *main;
  Scene *xs;

  Assets *assets;

  // hijacking this struct for controllers
  // TODO move this somewhere more appropriate
  BoardController *board_controller;
  PlayerController *player_controller;
  UiController *ui_controller;
};

struct Sequence {
  i32 next_step = 0;

  void reset_base() { next_step = 0; }

  virtual bool update(float, Scenes, ClientGameData *, InputState *, RpcClient *) = 0;
};

struct ScriptDefinition {
  struct InputDef {
    String name;
    int *value;
    EntityType entity_type;
  };

  String name;
  Sequence *seq;
  std::vector<InputDef> inputs;
};

struct IntroSequence : Sequence {
  ScriptDefinition def()
  {
    return {
        "Intro Sequence",
        this,
        {
            {"camera", &input.camera, EntityType::CAMERA},
            {"path", &input.path, EntityType::SPLINE},
        },
    };
  }

  struct Input {
    int camera;
    int path;
  };
  Input input;

  void reset(Scenes scenes)
  {
    scenes.main->set_sequence(scenes.assets->get_keyed_animation("resources/test/intro"));
    scenes.main->play_sequence();
    scenes.main->set_t(0);
  }

  bool update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state,
              RpcClient *rpc_client)
  {
    step({
      scenes.board_controller->set_family0_score(0, scenes.assets);
      scenes.board_controller->set_family1_score(0, scenes.assets);
    });

    Entity *camera = scenes.main->get(input.camera);
    if (camera) {
      scenes.main->active_camera_id = input.camera;
    }

    step(scenes.ui_controller->popup_banner("hello", 5.f);)

        return scenes.main->is_sequence_finished();
  }
};

struct RoundStartSequence : Sequence {
  ScriptDefinition def()
  {
    return {
        "Round Start Sequence",
        this,
        {
            {"camera", &input.camera, EntityType::CAMERA},
            {"path", &input.path, EntityType::SPLINE},
        },
    };
  }

  float t;
  const float length = 4.f;

  struct Input {
    int camera;
    int path;
  };
  Input input;

  void reset(Scenes scenes, int round)
  {
    t = 0;
    scenes.main->set_sequence(scenes.assets->get_keyed_animation("resources/test/round_start"));
    scenes.main->play_sequence();
    scenes.main->set_t(0);
  }
  bool update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state,
              RpcClient *rpc_client)
  {
    t += timestep;
    if (t > length) {
      t = length;
    }

    Entity *camera = scenes.main->get(input.camera);
    if (camera) {
      scenes.main->active_camera_id = input.camera;
    }

    return scenes.main->is_sequence_finished();
  }
};

// shows players from both teams walking to table
struct FaceoffStartSequence : Sequence {
  ScriptDefinition def()
  {
    return {
        "Faceoff Start Sequence",
        this,
        {
            {"camera", &input.camera, EntityType::CAMERA},
            {"path", &input.path, EntityType::SPLINE},
        },
    };
  }

  float t;
  const float length = 4.f;

  struct Input {
    int camera;
    int path;
  };
  Input input;

  void reset(Scenes scenes, int round)
  {
    t = 0;
    scenes.main->set_sequence(scenes.assets->get_keyed_animation("resources/test/faceoff_start"));
    scenes.main->play_sequence();
    scenes.main->set_t(0);
  }
  bool update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state,
              RpcClient *rpc_client)
  {
    t += timestep;
    if (t > length) {
      t = length;
    }

    // Entity *camera = scenes.main->get(input.camera);
    // if (camera)
    // {
    //     scenes.main->active_camera_id = input.camera;
    // }

    scenes.player_controller->start_faceoff();

    return scenes.main->is_sequence_finished();
  }
};

struct AskQuestionSequence : Sequence {
  ScriptDefinition def()
  {
    return {
        "Ask Question Sequence",
        this,
        {},
    };
  }

  struct Input {
  };
  Input input;

  f32 question_appearance_t = 0;

  void init(Scenes scenes) { question_appearance_t = 0; }

  bool update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state,
              RpcClient *rpc_client)
  {
    step({
      scenes.main->set_sequence(
          scenes.assets->get_keyed_animation("resources/test/question_start"));
      scenes.main->play_sequence();
      scenes.main->set_t(0);
    })

        yield_wait(.5f);

    step({
      assert(game_data->num_answers < 10);
      scenes.board_controller->activate(scenes.main, game_data->num_answers);
      AllocatedString<128> dialogue = string_to_allocated_string<128>("We've got ");
      dialogue.append('0' + game_data->num_answers);
      dialogue.append(" answers on the board.");
      scenes.ui_controller->popup_banner(dialogue);
    })

        yield_wait(5.5f);

    scenes.ui_controller->show_buzz_button(true);
    if (scenes.ui_controller->buzz_button_pressed) {
      rpc_client->InGameBuzz({});
      scenes.ui_controller->show_buzz_button(false);
    }

    step(scenes.ui_controller->question(true));
    step(scenes.ui_controller->set_question(game_data->question));

    const f32 question_appearance_interval = 3.f;
    f32 question_appearance_ratio =
        std::min(question_appearance_t / question_appearance_interval, 1.f);
    scenes.ui_controller->set_question_visible_pct(question_appearance_ratio);
    question_appearance_t += timestep;

    return false;
  }
};

struct PlayerBuzzedSequence : Sequence {
  ScriptDefinition def()
  {
    return {
        "PlayerBuzzedSequence",
        this,
        {},
    };
  }

  struct Input {
  };
  Input input;

  float t;
  const float length = 3.f;

  PlayerName player_name;

  void reset(PlayerName player_name)
  {
    t                 = 0;
    this->player_name = player_name;
  }
  bool update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state,
              RpcClient *rpc_client)
  {
    t += timestep;

    step({
      scenes.ui_controller->show_buzz_button(false);
      scenes.ui_controller->question(true);
    });

    step(AllocatedString<256> combined = player_name; combined.append(" buzzed.");
         scenes.ui_controller->popup_banner(combined););

    return t > length;
  }
};

struct PassOrPlaySequence : Sequence {
  ScriptDefinition def()
  {
    return {
        "Pass Or Play Sequence",
        this,
        {
            {"camera_faceoff_left", &input.camera_faceoff_left, EntityType::CAMERA},
            {"camera_faceoff_right", &input.camera_faceoff_right, EntityType::CAMERA},
        },
    };
  }

  struct Input {
    i32 camera_faceoff_left;
    i32 camera_faceoff_right;
  };
  Input input;

  bool update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state,
              RpcClient *rpc_client)
  {
    step({
      scenes.ui_controller->pass_or_play(true);
      scenes.ui_controller->question(true);
    })

        if (scenes.ui_controller->play_button_pressed)
    {
      rpc_client->InGameChoosePassOrPlay({true});
      scenes.ui_controller->pass_or_play(false);
    }
    if (scenes.ui_controller->pass_button_pressed) {
      rpc_client->InGameChoosePassOrPlay({false});
      scenes.ui_controller->pass_or_play(false);
    }

    return false;
  }
};

struct PlayerChosePassOrPlaySequence : Sequence {
  ScriptDefinition def()
  {
    return {
        "PlayerChosePassOrPlaySequence",
        this,
        {},
    };
  }

  struct Input {
  };
  Input input;

  const f32 length = 3.f;
  f32 t;

  b8 play;

  void reset(b8 play)
  {
    t = 0;

    this->play = play;
  }
  bool update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state,
              RpcClient *rpc_client)
  {
    t += timestep;

    step({ scenes.ui_controller->popup_banner(play ? "\"Play\"" : "\"Pass\""); })

            return t >= length;
  }
};

struct PrepForPromptForAnswerSequence : Sequence {
  ScriptDefinition def()
  {
    return {
        "PrepForPromptForAnswerSequence",
        this,
        {
            {"camera_faceoff_main", &input.camera_faceoff_main, EntityType::CAMERA},

            {"camera_family_0_player_0", &input.camera_family_0_player_0, EntityType::CAMERA},
            {"camera_family_0_player_1", &input.camera_family_0_player_1, EntityType::CAMERA},
            {"camera_family_0_player_2", &input.camera_family_0_player_2, EntityType::CAMERA},
            {"camera_family_0_player_3", &input.camera_family_0_player_3, EntityType::CAMERA},
            {"camera_family_0_player_4", &input.camera_family_0_player_4, EntityType::CAMERA},

            {"camera_family_1_player_0", &input.camera_family_1_player_0, EntityType::CAMERA},
            {"camera_family_1_player_1", &input.camera_family_1_player_1, EntityType::CAMERA},
            {"camera_family_1_player_2", &input.camera_family_1_player_2, EntityType::CAMERA},
            {"camera_family_1_player_3", &input.camera_family_1_player_3, EntityType::CAMERA},
            {"camera_family_1_player_4", &input.camera_family_1_player_4, EntityType::CAMERA},
        },
    };
  }

  struct Input {
    i32 camera_faceoff_main;

    i32 camera_family_0_player_0;
    i32 camera_family_0_player_1;
    i32 camera_family_0_player_2;
    i32 camera_family_0_player_3;
    i32 camera_family_0_player_4;

    i32 camera_family_1_player_0;
    i32 camera_family_1_player_1;
    i32 camera_family_1_player_2;
    i32 camera_family_1_player_3;
    i32 camera_family_1_player_4;
  };
  Input input;

  float t;
  const float length = 3.f;

  i32 family;
  i32 player_position;

  i32 get_player_camera()
  {
    assert(player_position < 5);
    i32 family0[] = {
        input.camera_family_0_player_0, input.camera_family_0_player_1,
        input.camera_family_0_player_2, input.camera_family_0_player_3,
        input.camera_family_0_player_4,
    };
    i32 family1[] = {
        input.camera_family_1_player_0, input.camera_family_1_player_1,
        input.camera_family_1_player_2, input.camera_family_1_player_3,
        input.camera_family_1_player_4,
    };
    return family == 0 ? family0[player_position] : family1[player_position];
  }

  void reset(i32 family, i32 player_position)
  {
    t = 0;

    this->family          = family;
    this->player_position = player_position;
  }
  bool update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state,
              RpcClient *rpc_client)
  {
    t += timestep;

    step(scenes.ui_controller->question(true));

    if (game_data->round_stage == RoundStage::FACEOFF) {
      scenes.main->active_camera_id = input.camera_faceoff_main;
    } else {
      scenes.main->active_camera_id = get_player_camera();
    }

    step(scenes.ui_controller->popup_banner("\"I'm prompting you for an answer.\"");)

            return t > length;
  }
};

struct PromptForAnswerSequence : Sequence {
  ScriptDefinition def()
  {
    return {
        "PromptForAnswerSequence",
        this,
        {
            {"camera_faceoff_left", &input.camera_faceoff_left, EntityType::CAMERA},
            {"camera_faceoff_right", &input.camera_faceoff_right, EntityType::CAMERA},
        },
    };
  }

  struct Input {
    i32 camera_faceoff_left;
    i32 camera_faceoff_right;
  };
  Input input;

  float time_remaining;

  PlayerData *answerer;
  TextBox2<32> textbox;

  void reset(PlayerData *answerer)
  {
    this->answerer       = answerer;
    this->time_remaining = 15.f;
  }
  bool update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state,
              RpcClient *rpc_client)
  {
    step(scenes.ui_controller->question(true));

    time_remaining -= timestep;
    scenes.ui_controller->answer_timer(true, time_remaining);

    if (game_data->round_stage == RoundStage::FACEOFF) {
      scenes.main->active_camera_id = answerer->id == game_data->faceoffers.first
                                          ? input.camera_faceoff_left
                                          : input.camera_faceoff_right;
    }

    if (answerer->id == game_data->my_id) {
      scenes.ui_controller->show_answer_textbox(true);
      if (scenes.ui_controller->answer_submitted) {
        rpc_client->InGameAnswer({scenes.ui_controller->answer_textbox.text});
        scenes.ui_controller->show_answer_textbox(false);
      }
    } else {
      step(AllocatedString<256> combined = answerer->name; combined.append(" is answering.");
           scenes.ui_controller->popup_banner(combined);)
    }

    return false;
  }
};

struct StartPlaySequence : Sequence {
  ScriptDefinition def()
  {
    return {
        "StartPlaySequence",
        this,
        {
            {"camera", &input.camera, EntityType::CAMERA},
            {"path", &input.path, EntityType::SPLINE},
        },
    };
  }

  const float length = 5.f;
  float t;

  struct Input {
    int camera;
    int path;
  };
  Input input;

  void reset() { t = 0; }

  bool update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state,
              RpcClient *rpc_client)
  {
    t += timestep;
    if (t > length) {
      t = length;
    }

    Entity *camera = scenes.main->get(input.camera);
    Entity *path   = scenes.main->get(input.path);
    if (camera && path && path->type == EntityType::SPLINE) {
      camera->transform.position =
          catmull_rom(constant_distance_t(path->spline, t / length) - 1, path->spline);
    }

    // step (player_controller->end_faceoff) // walking away animations
    // camera on current player, move host to that position
    // repeat question
    //

    return t >= length;
  }
};
struct StartStealSequence : Sequence {
  ScriptDefinition def()
  {
    return {
        "StartStealSequence",
        this,
        {
            {"camera", &input.camera, EntityType::CAMERA},
            {"path", &input.path, EntityType::SPLINE},
        },
    };
  }

  const float length = 5.f;
  float t;

  struct Input {
    int camera;
    int path;
  };
  Input input;

  void reset() { t = 0; }

  bool update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state,
              RpcClient *rpc_client)
  {
    t += timestep;
    if (t > length) {
      t = length;
    }

    Entity *camera = scenes.main->get(input.camera);
    Entity *path   = scenes.main->get(input.path);
    if (camera && path && path->type == EntityType::SPLINE) {
      camera->transform.position =
          catmull_rom(constant_distance_t(path->spline, t / length) - 1, path->spline);
    }

    return t >= length;
  }
};

struct PlayerAnsweredSequence : Sequence {
  ScriptDefinition def()
  {
    return {
        "PlayerAnsweredSequence",
        this,
        {},
    };
  }

  struct Input {
  };
  Input input;

  float t;
  const float length = 3.f;

  AllocatedString<64> answer;

  void reset(AllocatedString<64> answer)
  {
    t = 0;

    this->answer = answer;
  }
  bool update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state,
              RpcClient *rpc_client)
  {
    t += timestep;
    float pos_x = 200 + sinf(t * 1000) * 20;
    float pos_y = 500 + sinf(t * 1000) * 20;

    step(scenes.ui_controller->popup_banner(answer, 5.f))

            return t > length;
  }
};

struct FlipAnswerSequence : Sequence {
  ScriptDefinition def()
  {
    return {
        "FlipAnswerSequence",
        this,
        {
            {"camera", &input.camera, EntityType::CAMERA},
        },
    };
  }

  struct Input {
    int bars[8];
    int camera;
  };
  Input input;

  AllocatedString<64> answer;
  int answer_i;
  int score;

  void reset(AllocatedString<64> answer, int answer_i, int score)
  {
    this->answer   = answer;
    this->answer_i = answer_i;
    this->score    = score;
  }
  bool update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state,
              RpcClient *rpc_client)
  {
    step({
      scenes.ui_controller->question(true);
      scenes.ui_controller->set_question_visible_pct(1.f);
    });

    scenes.main->active_camera_id = input.camera;

    yield_wait(2.f);

    step({
      scenes.board_controller->flip(scenes.main, scenes.assets, answer_i, answer, score);
      scenes.board_controller->set_round_score(game_data->this_round_score, scenes.assets);
    });

    yield_wait(2.f);

    return true;
  }
};

struct EeeeeggghhhhSequence : Sequence {
  ScriptDefinition def()
  {
    return {
        "EeeeeggghhhhSequence",
        this,
        {
            {"camera", &input.camera, EntityType::CAMERA},
        },
    };
  }

  struct Input {
    int camera;
  };
  Input input;

  f32 t;
  f32 popout_t;

  void reset()
  {
    t        = 0;
    popout_t = 0;
  }

  bool update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state,
              RpcClient *rpc_client)
  {
    step({
      scenes.ui_controller->question(true);
      scenes.ui_controller->set_question_visible_pct(1.f);
    });

    step(scenes.main->active_camera_id = input.camera;);

    yield_wait(2.f);

    scenes.xs->visible = true;

    auto bump_function = [](f64 t) -> f64 {
      if (t < 0 || t > 1) return 0;
      return 2.71828182846 * std::exp(-1 / ((-4 * t * t) + (4 * t)));
    };
    auto clamped_bump_function = [&](f64 t, f64 top) -> f32 {
      return std::min(bump_function(t), top);
    };
    auto easeOutCubic   = [](f32 t) { return 1 - std::pow(1 - t, 3); };
    auto x_pop_function = [&](f32 t) {
      return 2 * clamped_bump_function(easeOutCubic(t * .4), 0.15);
    };

    t += timestep;

    // FYI this is relying on the first three entities in the second secene being Xs
    if (game_data->incorrects == 1 || game_data->round_stage == RoundStage::FACEOFF) {
      scenes.xs->entities.data[1].value.transform.position = {0, 0, x_pop_function(t)};
      scenes.xs->entities.data[1].value.transform.rotation = {PI / 2, 0, 0};
      scenes.xs->entities.data[1].value.transform.scale    = {
          1 + x_pop_function(t) * .1f,
          1 + x_pop_function(t) * .1f,
          1 + x_pop_function(t) * .1f,
      };

      scenes.xs->entities.data[0].value.transform.position.z = 10;
      scenes.xs->entities.data[2].value.transform.position.z = 10;
    } else if (game_data->incorrects == 2) {
      scenes.xs->entities.data[0].value.transform.position = {-.5f, 0, 0};
      scenes.xs->entities.data[0].value.transform.rotation = {PI / 2, 0, 0};
      scenes.xs->entities.data[1].value.transform.position = {.5f, 0, x_pop_function(t)};
      scenes.xs->entities.data[1].value.transform.rotation = {PI / 2, 0, 0};
      scenes.xs->entities.data[1].value.transform.scale    = {
          1 + x_pop_function(t) * .1f,
          1 + x_pop_function(t) * .1f,
          1 + x_pop_function(t) * .1f,
      };

      scenes.xs->entities.data[2].value.transform.position.z = 10;
    } else {
      scenes.xs->entities.data[0].value.transform.position = {-1, 0, 0};
      scenes.xs->entities.data[0].value.transform.rotation = {PI / 2, 0, 0};
      scenes.xs->entities.data[1].value.transform.position = {0, 0, 0};
      scenes.xs->entities.data[1].value.transform.rotation = {PI / 2, 0, 0};

      scenes.xs->entities.data[2].value.transform.position = {1, 0, x_pop_function(t)};
      scenes.xs->entities.data[2].value.transform.rotation = {PI / 2, 0, 0};
      scenes.xs->entities.data[2].value.transform.scale    = {
          1 + x_pop_function(t) * .1f,
          1 + x_pop_function(t) * .1f,
          1 + x_pop_function(t) * .1f,
      };
    }

    // moving light pizzazzzz
    scenes.xs->entities.data[7].value.transform.position.x = 1.8f - t;

    yield_wait(4.f);

    const f32 speed_denoms[3]      = {2, 3., 2.15};
    const glm::quat target_rots[3] = {glm::quat({.4f, 2, 0}), glm::quat({.5f, 0, -.9f}),
                                      glm::quat({1.f, 1.3f, .75f})};
    glm::vec3 initial_positions[3] = {glm::vec3{-1, 0, 0}, glm::vec3{0, 0, 0}, glm::vec3{1, 0, 0}};

    if ((game_data->round_stage == RoundStage::FACEOFF && game_data->incorrects == 2) ||
        game_data->incorrects == 3) {
      if (game_data->round_stage == RoundStage::FACEOFF) {
        initial_positions[0].z = 20;
        initial_positions[1].z = 20;
      }

      popout_t += timestep;

      i32 start = 0;
      i32 end   = 3;
      if (game_data->round_stage == RoundStage::FACEOFF) {
        start = 1;
        end   = 2;
      }

      for (i32 i = start; i < end; i++) {
        glm::vec3 initial_pos = {i - 1, 0.f, 0.f};
        glm::quat initial_rot({glm::radians(90.f), 0.f, 0.f});

        glm::vec3 target_pos = {i - 1.5, 0, 5};
        glm::quat target_rot = target_rots[i];

        f32 actual_t = powf(glm::clamp(popout_t / 3, 0.f, 1.f), speed_denoms[i]);

        glm::vec3 actual_pos = initial_pos + ((target_pos - initial_pos) * actual_t);
        glm::quat actual_rot =
            glm::normalize(initial_rot + ((target_rot - initial_rot) * actual_t));
        glm::vec3 actual_rot_euler = glm::eulerAngles(actual_rot);

        scenes.xs->entities.data[i].value.transform.position = {actual_pos.x, actual_pos.y,
                                                                actual_pos.z};
        scenes.xs->entities.data[i].value.transform.rotation = {
            actual_rot_euler.x, actual_rot_euler.y, actual_rot_euler.z};
      }

      yield_wait(3.5f);
    }

    scenes.xs->visible = false;

    yield_wait(2.f);

    return true;
  }
};

struct EndRoundSequence : Sequence {
  ScriptDefinition def()
  {
    return {
        "EndRoundSequence",
        this,
        {
            {"camera", &input.camera, EntityType::CAMERA},
        },
    };
  }

  struct Input {
    int camera;
  };
  Input input;

  float t;
  const float length = 3.f;
  i32 round_winner;

  void reset(i32 round_winner) { this->round_winner = round_winner; }

  bool update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state,
              RpcClient *rpc_client)
  {
    step({
      scenes.main->active_camera_id = input.camera;
      scenes.board_controller->reset(scenes.main);
      scenes.board_controller->set_family0_score(game_data->scores[0], scenes.assets);
      scenes.board_controller->set_family1_score(game_data->scores[1], scenes.assets);
    })

        if (round_winner == 0)
    {
      step({
        scenes.main->set_sequence(
            scenes.assets->get_keyed_animation("resources/test/family0_round_win"));
        scenes.main->play_sequence();
        scenes.main->set_t(0);
      })
    }
    else if (round_winner == 1)
    {
      step({
        scenes.main->set_sequence(
            scenes.assets->get_keyed_animation("resources/test/family1_round_win"));
        scenes.main->play_sequence();
        scenes.main->set_t(0);
      })
    }
    else
    {
      // TODO: faceoff failure, show faceoffers going back
    }

    return false;
  }
};

struct EndGameSequence : Sequence {
  ScriptDefinition def()
  {
    return {
        "EndGameSequence",
        this,
        {
            {"camera", &input.camera, EntityType::CAMERA},
        },
    };
  }

  struct Input {
    int camera;
  };
  Input input;

  float t;
  const float length = 3.f;
  i32 game_winner;

  void reset(i32 game_winner) { this->game_winner = game_winner; }
  bool update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state,
              RpcClient *rpc_client)
  {
    if (game_winner == 0) {
      step({
        scenes.main->set_sequence(
            scenes.assets->get_keyed_animation("resources/test/family0_game_win"));
        scenes.main->play_sequence();
        scenes.main->set_t(0);
      })
    } else {
      step({
        scenes.main->set_sequence(
            scenes.assets->get_keyed_animation("resources/test/family1_game_win"));
        scenes.main->play_sequence();
        scenes.main->set_t(0);
      })
    }

    return false;
  }
};

struct Game {
  BoardController board_controller;
  PlayerController player_controller;
  UiController ui_controller;

  IntroSequence intro_sequence;
  RoundStartSequence round_start_sequence;
  FaceoffStartSequence faceoff_start_sequence;
  AskQuestionSequence ask_question_sequence;
  PlayerBuzzedSequence player_buzzed_sequence;
  PrepForPromptForAnswerSequence prep_for_prompt_for_answer_sequence;
  PromptForAnswerSequence prompt_for_answer_sequence;
  PassOrPlaySequence pass_or_play_sequence;
  PlayerChosePassOrPlaySequence player_chose_pass_or_play_sequence;
  StartPlaySequence start_play_sequence;
  StartStealSequence start_steal_sequence;
  PlayerAnsweredSequence player_answered_sequence;
  FlipAnswerSequence flip_answer_sequence;
  EeeeeggghhhhSequence eeeeggghhh_sequence;
  EndRoundSequence end_round_sequence;
  EndGameSequence end_game_sequence;
  Sequence *current_sequence;

  bool sent_ready = false;

  ClientGameData game_data;

  void init(Scenes scenes)
  {
    player_controller.init();

    // TODO ask server for player avatars?
    ask_question_sequence.init(scenes);
  }
  void reset(Scenes scenes)
  {
    // start intro cinematic
    intro_sequence.reset(scenes);
    set_current_sequence(&intro_sequence);
  }
  void update(float, Scenes, RpcClient *, InputState *);
  bool handle_rpcs(Scenes, RpcClient *);  // returns true if game is over

  void set_current_sequence(Sequence *seq)
  {
    ui_controller.hide_all();

    seq->reset_base();
    current_sequence = seq;
  }

  // TODO there really should be a way to create a static array at compile time or something why is
  // it so hard
  std::vector<ScriptDefinition> get_script_defs()
  {
    return {
        intro_sequence.def(),
        round_start_sequence.def(),
        faceoff_start_sequence.def(),
        ask_question_sequence.def(),
        player_buzzed_sequence.def(),
        prep_for_prompt_for_answer_sequence.def(),
        prompt_for_answer_sequence.def(),
        pass_or_play_sequence.def(),
        player_chose_pass_or_play_sequence.def(),
        start_play_sequence.def(),
        start_steal_sequence.def(),
        player_answered_sequence.def(),
        flip_answer_sequence.def(),
        eeeeggghhh_sequence.def(),
        end_round_sequence.def(),
        end_game_sequence.def(),
    };
  }
};

void Game::update(float timestep, Scenes scenes, RpcClient *rpc_client, InputState *input_state)
{
  scenes.board_controller  = &board_controller;
  scenes.player_controller = &player_controller;
  scenes.ui_controller     = &ui_controller;

  handle_rpcs(scenes, rpc_client);

  if (current_sequence) {
    b8 finished = current_sequence->update(timestep, scenes, &game_data, input_state, rpc_client);
    if (finished && !sent_ready) {
      rpc_client->InGameReady({});
      sent_ready = true;
    }
  }
}

// returns true if game is over
bool Game::handle_rpcs(Scenes scenes, RpcClient *rpc_client)
{
  if (auto msg = rpc_client->get_StartGame_msg()) {
    printf("get_StartGame_msg\n");
    reset(scenes);
  } else if (auto msg = rpc_client->get_GameStatePing_msg()) {
    game_data.my_id = msg->my_id;

    game_data.players.clear();
    for (auto p : msg->players) {
      game_data.players.append({p.user_id, p.name, p.family});
    }
  } else if (auto msg = rpc_client->get_InGameStartRound_msg()) {
    printf("get_InGameStartRound_msg\n");
    if (current_sequence != &round_start_sequence) {
      game_data.round++;
      game_data.round_stage = RoundStage::START;

      round_start_sequence.reset(scenes, 0);
      set_current_sequence(&round_start_sequence);
      sent_ready = false;
    }
  } else if (auto msg = rpc_client->get_InGameStartFaceoff_msg()) {
    printf("get_InGameStartFaceoff_msg\n");

    game_data.faceoffers.first  = msg->faceoffer_0_id;
    game_data.faceoffers.second = msg->faceoffer_1_id;

    if (current_sequence != &faceoff_start_sequence) {
      game_data.round_stage = RoundStage::FACEOFF;

      faceoff_start_sequence.reset(scenes, 0);
      set_current_sequence(&faceoff_start_sequence);
      sent_ready = false;
    }
  } else if (auto msg = rpc_client->get_InGameAskQuestion_msg()) {
    printf("get_InGameAskQuestion_msg\n");

    game_data.question    = msg->question;
    game_data.num_answers = msg->num_answers;
    if (current_sequence != &ask_question_sequence) {
      game_data.question = msg->question;

      set_current_sequence(&ask_question_sequence);
      sent_ready = false;
    }
  } else if (auto msg = rpc_client->get_InGamePlayerBuzzed_msg()) {
    printf("get_InGamePlayerBuzzed_msg\n");
    game_data.buzzing_family = msg->buzzing_family;
    if (current_sequence != &player_buzzed_sequence) {
      player_buzzed_sequence.reset(game_data.get_player_data(msg->user_id)->name);
      set_current_sequence(&player_buzzed_sequence);
      sent_ready = false;
    }
  } else if (auto msg = rpc_client->get_InGamePrepForPromptForAnswer_msg()) {
    printf("get_InGamePrepForPromptForAnswer_msg\n");
    if (current_sequence != &prep_for_prompt_for_answer_sequence) {
      prep_for_prompt_for_answer_sequence.reset(msg->family, msg->player_position);
      set_current_sequence(&prep_for_prompt_for_answer_sequence);
      sent_ready = false;
    }
  } else if (auto msg = rpc_client->get_InGamePromptForAnswer_msg()) {
    printf("get_InGamePromptForAnswer_msg\n");
    if (current_sequence != &prompt_for_answer_sequence) {
      prompt_for_answer_sequence.reset(game_data.get_player_data(msg->user_id));
      set_current_sequence(&prompt_for_answer_sequence);
      sent_ready = false;
    }
  } else if (auto msg = rpc_client->get_InGamePlayerAnswered_msg()) {
    printf("get_InGamePlayerAnswered_msg\n");
    if (current_sequence != &player_answered_sequence) {
      player_answered_sequence.reset(msg->answer);
      set_current_sequence(&player_answered_sequence);
      sent_ready = false;
    }
  } else if (auto msg = rpc_client->get_InGamePromptPassOrPlay_msg()) {
    printf("get_InGamePromptPassOrPlay_msg\n");
    if (current_sequence != &pass_or_play_sequence) {
      set_current_sequence(&pass_or_play_sequence);
      sent_ready = false;
    }
  } else if (auto msg = rpc_client->get_InGamePlayerChosePassOrPlay_msg()) {
    printf("get_InGamePlayerChosePassOrPlay_msg\n");

    if (current_sequence != &player_chose_pass_or_play_sequence) {
      player_chose_pass_or_play_sequence.reset(msg->play);
      set_current_sequence(&player_chose_pass_or_play_sequence);
      sent_ready = false;
    }
  } else if (auto msg = rpc_client->get_InGameStartPlay_msg()) {
    printf("get_InGameStartPlay_msg\n");
    game_data.round_stage = RoundStage::PLAY;

    game_data.incorrects = 0;
    if (current_sequence != &start_play_sequence) {
      start_play_sequence.reset();
      set_current_sequence(&start_play_sequence);
      sent_ready = false;
    }
  } else if (auto msg = rpc_client->get_InGameStartSteal_msg()) {
    printf("get_InGameStartSteal_msg\n");
    game_data.round_stage = RoundStage::STEAL;

    if (current_sequence != &start_steal_sequence) {
      start_steal_sequence.reset();
      set_current_sequence(&start_steal_sequence);
      sent_ready = false;
    }
  } else if (auto msg = rpc_client->get_InGameFlipAnswer_msg()) {
    printf("get_InGameFlipAnswer_msg\n");
    game_data.this_round_score = msg->round_score;
    if (current_sequence != &flip_answer_sequence) {
      flip_answer_sequence.reset(msg->answer, msg->answer_index, msg->score);
      set_current_sequence(&flip_answer_sequence);
      sent_ready = false;
    }
  } else if (auto msg = rpc_client->get_InGameEggghhhh_msg()) {
    printf("get_InGameEggghhhh_msg\n");

    game_data.incorrects = msg->n_incorrects;
    if (current_sequence != &eeeeggghhh_sequence) {
      eeeeggghhh_sequence.reset();
      set_current_sequence(&eeeeggghhh_sequence);
      sent_ready = false;
    }
  } else if (auto msg = rpc_client->get_InGameEndRound_msg()) {
    printf("get_InGameEndRound_msg\n");
    game_data.scores[0] = msg->family0_score;
    game_data.scores[1] = msg->family1_score;
    if (current_sequence != &end_round_sequence) {
      end_round_sequence.reset(msg->round_winner);
      set_current_sequence(&end_round_sequence);
      sent_ready = false;
    }
  } else if (auto msg = rpc_client->get_InGameEndGame_msg()) {
    printf("get_InGameEndGame_msg\n");
    if (current_sequence != &end_game_sequence) {
      end_game_sequence.reset(msg->game_winner);
      set_current_sequence(&end_game_sequence);
      sent_ready = false;
    }
  }
  return false;
}
