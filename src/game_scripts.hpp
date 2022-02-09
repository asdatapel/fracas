#pragma once

#include "spline.hpp"
#include "scene/scene.hpp"
#include "net/generated_rpc_client.hpp"
#include "game_state.hpp"
#include "board_controller.hpp"

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
    return;                        \
  }

struct Scenes
{
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

  virtual void update(float, Scenes, ClientGameData *, InputState *, RpcClient *) = 0;
  virtual bool ready(Scenes scenes)                                               = 0;
};

struct ScriptDefinition
{
    struct InputDef
    {
        String name;
        int *value;
        EntityType entity_type;
    };

    String name;
    Sequence *seq;
    std::vector<InputDef> inputs;
};

struct IntroSequence : Sequence
{
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

    const float length = 5.f;
    float t;

    struct Input
    {
        int camera;
        int path;
    };
    Input input;

    void reset(Scenes scenes)
    {
        t = 0;
        scenes.main->set_sequence(scenes.assets->get_keyed_animation("resources/test/intro"));
        scenes.main->play_sequence();
        scenes.main->set_t(0);
    }

    void update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state, RpcClient *rpc_client)
    {
        t += timestep;
        if (t > length)
        {
            t = length;
        }

        Entity *camera = scenes.main->get(input.camera);
        if (camera)
        {
            scenes.main->active_camera_id = input.camera;
        }

        step(
            scenes.ui_controller->popup_banner("hello", 5.f);)
    }

    bool ready(Scenes scenes)
    {
        return scenes.main->is_sequence_finished();
    }
};

struct RoundStartSequence : Sequence
{
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

    struct Input
    {
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
    void update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state, RpcClient *rpc_client)
    {
        t += timestep;
        if (t > length)
        {
            t = length;
        }

        Entity *camera = scenes.main->get(input.camera);
        if (camera)
        {
            scenes.main->active_camera_id = input.camera;
        }
    }
    bool ready(Scenes scenes)
    {
        return scenes.main->is_sequence_finished();
    }
};

// shows players from both teams walking to table
struct FaceoffStartSequence : Sequence
{
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

    struct Input
    {
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
    void update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state, RpcClient *rpc_client)
    {
        t += timestep;
        if (t > length)
        {
            t = length;
        }

        Entity *camera = scenes.main->get(input.camera);
        if (camera)
        {
            scenes.main->active_camera_id = input.camera;
        }

        scenes.player_controller->start_faceoff();
    }
    bool ready(Scenes scenes)
    {
        return scenes.main->is_sequence_finished();
    }
};

struct AskQuestionSequence : Sequence
{
    ScriptDefinition def()
    {
        return {
            "Ask Question Sequence",
            this,
            {},
        };
    }

    struct Input {};
    Input input;

    f32 question_appearance_t = 0;
    AllocatedString<128> question;
    int num_answers;

    void init(Scenes scenes)
    {
        question_appearance_t = 0;
        scenes.main->stop_sequence();
    }

    void reset(String question, int num_answers)
    {
        this->question = string_to_allocated_string<128>(question);
        this->num_answers = num_answers;
    }
    void update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state, RpcClient *rpc_client)
    {
        step({
            scenes.main->set_sequence(scenes.assets->get_keyed_animation("resources/test/question_start"));
            scenes.main->play_sequence();
            scenes.main->set_t(0);
        })

        yield_wait(.5f);

        step({
          assert(num_answers < 10);
          scenes.board_controller->activate(scenes.main, num_answers);
          AllocatedString<128> dialogue = string_to_allocated_string<128>("We've got ");
          dialogue.append('0' + num_answers);
          dialogue.append(" answers on the board.");
          scenes.ui_controller->popup_banner(dialogue);
        })

        yield_wait(5.5f);
        
        scenes.ui_controller->show_buzz_button(true);
        if (scenes.ui_controller->buzz_button_pressed) {
            rpc_client->InGameBuzz({});
        }

        const f32 question_appearance_interval = 3.f;
        f32 question_appearance_ratio = std::min(question_appearance_t / question_appearance_interval, 1.f);
        u16 num_letters = question_appearance_ratio * question.len;
        scenes.ui_controller->question(true, {question.data, num_letters});
        question_appearance_t += timestep;
    }
    bool ready(Scenes scenes) { return false; }
};

struct PassOrPlaySequence : Sequence
{
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

    struct Input
    {
        i32 camera_faceoff_left;
        i32 camera_faceoff_right;
    };
    Input input;

    void update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state, RpcClient *rpc_client)
    {
        step({
            scenes.ui_controller->pass_or_play(true);
        })

        if (scenes.ui_controller->play_button_pressed)
        {
            rpc_client->InGameChoosePassOrPlay({true});
        }
        if (scenes.ui_controller->pass_button_pressed)
        {
            rpc_client->InGameChoosePassOrPlay({false});
        }
    }
    bool ready(Scenes scenes)
    {
        return false;
    }
};

struct PlayerBuzzedSequence : Sequence
{
    ScriptDefinition def()
    {
        return {
            "PlayerBuzzedSequence",
            this,
            {},
        };
    }

    struct Input
    {
    };
    Input input;

    float t;
    const float length = 3.f;

    PlayerName player_name;

    void reset(PlayerName player_name)
    {
        t = 0;
        this->player_name = player_name;
    }
    void update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state, RpcClient *rpc_client)
    {
        t += timestep;

        step(
            AllocatedString<256> combined = player_name;
            combined.append(" buzzed.");
            scenes.ui_controller->popup_banner(combined);)
    }
    bool ready(Scenes scenes)
    {
        return t > length;
    }
};

struct PrepForPromptForAnswerSequence : Sequence
{
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

    struct Input
    {
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

    i32 get_player_camera() {
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

        this->family = family;
        this->player_position = player_position;
    }
    void update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state, RpcClient *rpc_client)
    {
        t += timestep;

        if (game_data->round_stage == RoundStage::FACEOFF) {
            scenes.main->active_camera_id = input.camera_faceoff_main;
        } else {
            scenes.main->active_camera_id = get_player_camera();
        }

        step(scenes.ui_controller->popup_banner("\"I'm prompting you for an answer.\"");)
    }
    bool ready(Scenes scenes) { return t > length; }
};

struct PromptForAnswerSequence : Sequence
{
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

    struct Input
    {
      i32 camera_faceoff_left;
      i32 camera_faceoff_right;
    };
    Input input;

    float time_remaining;

    PlayerData *answerer;
    TextBox2<32> textbox;

    void reset(PlayerData *answerer)
    {
        this->answerer = answerer;
        this->time_remaining = 15.f;
    }
    void update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state, RpcClient *rpc_client)
    {
        time_remaining -= timestep;
        scenes.ui_controller->answer_timer(true, time_remaining);

        if (game_data->round_stage == RoundStage::FACEOFF) {
          scenes.main->active_camera_id = answerer->id == game_data->faceoffers.first
                                              ? input.camera_faceoff_left
                                              : input.camera_faceoff_right;
        }

        if (answerer->id == game_data->my_id)
        {
            scenes.ui_controller->show_answer_textbox(true);
            if (scenes.ui_controller->answer_submitted) {
                rpc_client->InGameAnswer({scenes.ui_controller->answer_textbox.text});
            }
        }
        else
        {
            step(
                AllocatedString<256> combined = answerer->name;
                combined.append(" is answering.");
                scenes.ui_controller->popup_banner(combined);)

        }
    }
    bool ready(Scenes scenes) { return false; }
};

struct StartPlaySequence : Sequence
{
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

    struct Input
    {
        int camera;
        int path;
    };
    Input input;

    void reset()
    {
        t = 0;
    }

    void update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state, RpcClient *rpc_client)
    {
        t += timestep;
        if (t > length)
        {
            t = length;
        }

        Entity *camera = scenes.main->get(input.camera);
        Entity *path = scenes.main->get(input.path);
        if (camera && path && path->type == EntityType::SPLINE)
        {
            camera->transform.position = catmull_rom(constant_distance_t(path->spline, t / length) - 1, path->spline);
        }

        // step (player_controller->end_faceoff) // walking away animations
        // camera on current player, move host to that position
        // repeat question
        //
    }

    bool ready(Scenes scenes)
    {
        return t >= length;
    }
};
struct StartStealSequence : Sequence
{
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

    struct Input
    {
        int camera;
        int path;
    };
    Input input;

    void reset()
    {
        t = 0;
    }

    void update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state, RpcClient *rpc_client)
    {
        t += timestep;
        if (t > length)
        {
            t = length;
        }

        Entity *camera = scenes.main->get(input.camera);
        Entity *path = scenes.main->get(input.path);
        if (camera && path && path->type == EntityType::SPLINE)
        {
            camera->transform.position = catmull_rom(constant_distance_t(path->spline, t / length) - 1, path->spline);
        }
    }

    bool ready(Scenes scenes)
    {
        return t >= length;
    }
};

struct PlayerAnsweredSequence : Sequence
{
    ScriptDefinition def()
    {
        return {
            "PlayerAnsweredSequence",
            this,
            {},
        };
    }

    struct Input
    {
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
    void update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state, RpcClient *rpc_client)
    {
        t += timestep;
        float pos_x = 200 + sinf(t * 1000) * 20;
        float pos_y = 500 + sinf(t * 1000) * 20;

        step(scenes.ui_controller->popup_banner(answer, 5.f))
    }
    bool ready(Scenes scenes)
    {
        return t > length;
    }
};

struct FlipAnswerSequence : Sequence
{
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

    struct Input
    {
        int bars[8];
        int camera;
    };
    Input input;

    float t;
    const float length = 2.f;

    AllocatedString<64> answer;
    int answer_i;
    int score;

    void reset(AllocatedString<64> answer, int answer_i, int score)
    {
        t = 0;

        this->answer = answer;
        this->answer_i = answer_i;
        this->score = score;
    }
    void update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state, RpcClient *rpc_client)
    {
        scenes.main->active_camera_id = input.camera;
        
        yield_wait(2.f);
        
        if (t == 0)
            scenes.board_controller->flip(scenes.main, scenes.assets, answer_i, answer, score);

        t += timestep;
        // set scene camera to board_closeup_camera
        // wait x seconds
        // flip animation
        // wait x seconds
        // ready
    }
    bool ready(Scenes scenes)
    {
        return t >= length;
    }
};

struct EeeeeggghhhhSequence : Sequence
{
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

    struct Input
    {
        int camera;
    };
    Input input;

    float t;
    const float length = 5.f;

    void reset()
    {
        t = 0;
    }

    void update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state, RpcClient *rpc_client)
    {
        t += timestep;

        scenes.xs->visible = true;

        float speed_denoms[3] = {2, 2.75, 2.15};
        for (int i = 0; i < 3; i++)
        {
            glm::vec3 initial_pos = {-1 + i, 0, 5};
            glm::quat initial_rot({(i * .5f), .1f + (i * .1f), -0.5f + (i * 1.4f)});
            glm::vec3 target_pos = {i - 1, 0.f, 0.f};
            glm::quat target_rot({glm::radians(90.f), 0.f, 0.f});
            float actual_t = 1.f - powf(glm::clamp(t / 2, 0.f, 1.f), speed_denoms[i]);
            glm::vec3 actual_pos = initial_pos + ((target_pos - initial_pos) * actual_t);
            glm::quat actual_rot = glm::normalize(initial_rot + ((target_rot - initial_rot) * actual_t));
            glm::vec3 actual_rot_euler = glm::eulerAngles(actual_rot);

            scenes.xs->entities.data[i].value.transform.position = {actual_pos.x, actual_pos.y, actual_pos.z};
            scenes.xs->entities.data[i].value.transform.rotation = {actual_rot_euler.x, actual_rot_euler.y, actual_rot_euler.z};
        }
        // set scene camera to board_closeup_camera
        // wait x seconds
        // show Xs
        // wait x seconds
        // ready
    }
    bool ready(Scenes scenes)
    {
        return t > length;
    }
};

struct EndRoundSequence : Sequence
{
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

    struct Input
    {
        int camera;
    };
    Input input;

    float t;
    const float length = 3.f;

    void update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state, RpcClient *rpc_client)
    {
        // sweeping camera
        // move host back to faceoff table
        // round N graphic on screen
    }
    bool ready(Scenes scenes)
    {
        return false;
    }
};

struct EndGameSequence : Sequence
{
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

    struct Input
    {
        int camera;
    };
    Input input;

    float t;
    const float length = 3.f;

    void update(float timestep, Scenes scenes, ClientGameData *game_data, InputState *input_state, RpcClient *rpc_client)
    {
        // show winner
    }
    bool ready(Scenes scenes)
    {
        return false;
    }
};

struct Game
{
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
    void reset(Scenes scenes) {
        // start intro cinematic
        intro_sequence.reset(scenes);
        set_current_sequence(&intro_sequence);
    }
    void update(float, Scenes, RpcClient *, InputState *);
    bool handle_rpcs(Scenes, RpcClient *); // returns true if game is over

    void set_current_sequence(Sequence *seq) {
        seq->reset_base();
        current_sequence = seq;
    }

    // TODO there really should be a way to create a static array at compile time or something why is it so hard
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
    scenes.board_controller = &board_controller;
    scenes.player_controller = &player_controller;
    scenes.ui_controller = &ui_controller;

    handle_rpcs(scenes, rpc_client);

    if (current_sequence) {
        current_sequence->update(timestep, scenes, &game_data, input_state, rpc_client);
        if (current_sequence->ready(scenes) && !sent_ready)
        {
            rpc_client->InGameReady({});
            sent_ready = true;
        }
    }
}

// returns true if game is over
bool Game::handle_rpcs(Scenes scenes, RpcClient *rpc_client)
{
    if (auto msg = rpc_client->get_StartGame_msg())
    {
        printf("get_StartGame_msg\n");
        reset(scenes);
    }
    else if (auto msg = rpc_client->get_GameStatePing_msg())
    {
        game_data.my_id = msg->my_id;

        game_data.players.clear();
        for (auto p : msg->players) {
            game_data.players.append({p.user_id, p.name, p.family});
        }
    }
    else if (auto msg = rpc_client->get_InGameStartRound_msg())
    {
        printf("get_InGameStartRound_msg\n");
        if (current_sequence != &round_start_sequence)
        {
            game_data.round++;
            game_data.round_stage = RoundStage::START;

            round_start_sequence.reset(scenes, 0);
            set_current_sequence(&round_start_sequence);
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGameStartFaceoff_msg())
    {
        printf("get_InGameStartFaceoff_msg\n");

        game_data.faceoffers.first = msg->faceoffer_0_id;
        game_data.faceoffers.second = msg->faceoffer_1_id;

        if (current_sequence != &faceoff_start_sequence)
        {
            game_data.round_stage = RoundStage::FACEOFF;

            faceoff_start_sequence.reset(scenes, 0);
            set_current_sequence(&faceoff_start_sequence);
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGameAskQuestion_msg())
    {
        printf("get_InGameAskQuestion_msg\n");
        if (current_sequence != &ask_question_sequence)
        {
            game_data.question = msg->question;

            ask_question_sequence.reset(msg->question, msg->num_answers);
            set_current_sequence(&ask_question_sequence);
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGamePlayerBuzzed_msg())
    {
        printf("get_InGamePlayerBuzzed_msg\n");
        game_data.buzzing_family = msg->buzzing_family;
        if (current_sequence != &player_buzzed_sequence)
        {
            player_buzzed_sequence.reset(game_data.get_player_data(msg->user_id)->name);
            set_current_sequence(&player_buzzed_sequence);
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGamePrepForPromptForAnswer_msg())
    {
        printf("get_InGamePrepForPromptForAnswer_msg\n");
        if (current_sequence != &prep_for_prompt_for_answer_sequence)
        {
            prep_for_prompt_for_answer_sequence.reset(msg->family, msg->player_position);
            set_current_sequence(&prep_for_prompt_for_answer_sequence);
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGamePromptForAnswer_msg())
    {
        printf("get_InGamePromptForAnswer_msg\n");
        if (current_sequence != &prompt_for_answer_sequence)
        {
            prompt_for_answer_sequence.reset(game_data.get_player_data(msg->user_id));
            set_current_sequence(&prompt_for_answer_sequence);
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGamePlayerAnswered_msg())
    {
        printf("get_InGamePlayerAnswered_msg\n");
        if (current_sequence != &player_answered_sequence)
        {
            player_answered_sequence.reset(msg->answer);
            set_current_sequence(&player_answered_sequence);
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGamePromptPassOrPlay_msg())
    {
        printf("get_InGamePromptPassOrPlay_msg\n");
        if (current_sequence != &pass_or_play_sequence)
        {
            set_current_sequence(&pass_or_play_sequence);
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGamePlayerChosePassOrPlay_msg())
    {
        printf("get_InGamePlayerChosePassOrPlay_msg\n");
        ui_controller.popup_banner(msg->play ? "\"Play\"" : "\"PASS\"");
    }
    else if (auto msg = rpc_client->get_InGameStartPlay_msg())
    {
        printf("get_InGameStartPlay_msg\n");
        game_data.round_stage = RoundStage::PLAY;

        if (current_sequence != &start_play_sequence)
        {
            start_play_sequence.reset();
            set_current_sequence(&start_play_sequence);
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGameStartSteal_msg())
    {
        printf("get_InGameStartSteal_msg\n");
        game_data.round_stage = RoundStage::STEAL;
        
        if (current_sequence != &start_steal_sequence)
        {
            start_steal_sequence.reset();
            set_current_sequence(&start_steal_sequence);
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGameFlipAnswer_msg())
    {
        printf("get_InGameFlipAnswer_msg\n");
        if (current_sequence != &flip_answer_sequence)
        {
            flip_answer_sequence.reset(msg->answer, msg->answer_index, msg->score);
            set_current_sequence(&flip_answer_sequence);
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGameEggghhhh_msg())
    {
        printf("get_InGameEggghhhh_msg\n");
        if (current_sequence != &eeeeggghhh_sequence)
        {
            set_current_sequence(&eeeeggghhh_sequence);
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGameEndRound_msg())
    {
        printf("get_InGameEndRound_msg\n");
        if (current_sequence != &end_round_sequence)
        {
            set_current_sequence(&end_round_sequence);
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGameEndGame_msg())
    {
        printf("get_InGameEndGame_msg\n");
        if (current_sequence != &end_game_sequence)
        {
            set_current_sequence(&end_game_sequence);
            sent_ready = false;
        }
    }
    return false;
}

struct WaitStep
{
    float duration;
    float elapsed;
    WaitStep(float duration) : duration(duration) {}
    void init()
    {
        elapsed = 0.f;
    }
    bool update(float timestep)
    {
        elapsed += timestep;
        return elapsed >= duration;
    }
};