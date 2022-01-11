#pragma once

#include "spline.hpp"
#include "scene/scene.hpp"
#include "net/generated_rpc_client.hpp"
#include "game_state.hpp"
#include "board_controller.hpp"

#include "main_menu.hpp"

// stupid shit
#define CAT_(a, b) a##b
#define CAT(a, b) CAT_(a, b)
#define VARNAME(Var) CAT(Var, __LINE__)
#define step(...)                      \
    static bool VARNAME(step) = false; \
    if (!VARNAME(step))                \
    {                                  \
        VARNAME(step) = true;          \
        __VA_ARGS__;                   \
    }

struct Scenes
{
    Scene *main;
    Scene *xs;

    Assets *assets;

    // hijacking this struct for controllers
    // TODO move this somewhere more appropriate
    PlayerController *player_controller;
    UiController *ui_controller;
};

struct Sequence
{
    virtual void update(float, Scenes, BoardController *board_controller, InputState *, RpcClient *) = 0;
    virtual bool ready() = 0;
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
    }

    void update(float timestep, Scenes scenes, BoardController *board_controller, InputState *input_state, RpcClient *rpc_client)
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
            scenes.main->active_camera_id = input.camera;
            camera->transform.position = catmull_rom(constant_distance_t(path->spline, t / length) - 1, path->spline);
        }

        step(
            scenes.ui_controller->popup_banner("hello", 5.f);)
    }

    bool ready()
    {
        return t >= length;
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
    }
    void update(float timestep, Scenes scenes, BoardController *board_controller, InputState *input_state, RpcClient *rpc_client)
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
    bool ready()
    {
        return t >= length;
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
    }
    void update(float timestep, Scenes scenes, BoardController *board_controller, InputState *input_state, RpcClient *rpc_client)
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

        scenes.player_controller->start_faceoff();
    }
    bool ready()
    {
        return t >= length;
    }
};

struct AskQuestionSequence : Sequence
{
    ScriptDefinition def()
    {
        return {
            "Ask Question Sequence",
            this,
            {
                {"faceoff_camera", &input.faceoff_camera, EntityType::CAMERA},
                {"board_closeup_camera", &input.board_closeup_camera, EntityType::CAMERA},
            },
        };
    }

    struct Input
    {
        int faceoff_camera;
        int board_closeup_camera;
    };
    Input input;

    AllocatedString<128> question;
    int num_answers;
    Button2 buzz_button;

    void init(Scenes scenes)
    {
    }

    void reset(String question, int num_answers)
    {
        this->question = string_to_allocated_string<128>(question);
        this->num_answers = num_answers;
    }
    void update(float timestep, Scenes scenes, BoardController *board_controller, InputState *input_state, RpcClient *rpc_client)
    {
        Font *font = scenes.assets->get_font(FONT_ROBOTO_CONDENSED_REGULAR, 64);

        static bool step_0_done = false;
        if (!step_0_done)
        {
            scenes.main->active_camera_id = input.board_closeup_camera;
        }
        step_0_done = true;

        static float wait_0 = .5f;
        wait_0 -= timestep;
        if (wait_0 > 0.f)
        {
            return;
        }

        static bool step_1_done = false;
        if (!step_1_done)
        {
            board_controller->activate(scenes.main, num_answers);
        }
        step_1_done = true;

        static float wait_1 = 3.f;
        wait_1 -= timestep;
        if (wait_1 > 0.f)
        {
            return;
        }

        static bool step_2_done = false;
        if (!step_2_done)
        {
            scenes.main->active_camera_id = input.faceoff_camera;
        }
        step_2_done = true;

        static float wait_2 = 1.f;
        wait_2 -= timestep;
        if (wait_2 > 0.f)
        {
            return;
        }

        glEnable(GL_BLEND);
        draw_text(*font, scenes.main->target, question, 10, 10, 2, 2);
        glDisable(GL_BLEND);

        buzz_button.rect = anchor_bottom_right(
            {0.2f * 1920, 0.2f * 1920 * 9.f / 16.f / 2.f},
            {25.f, 25.f});
        buzz_button.text = "BUZZ";

        if (buzz_button.update_and_draw(scenes.main->target, input_state, font))
        {
            rpc_client->InGameBuzz({});
        }
    }
    bool ready()
    {
        return false;
    }
};

struct PassOrPlaySequence : Sequence
{
    ScriptDefinition def()
    {
        return {
            "Pass Or Play Sequence",
            this,
            {},
        };
    }

    struct Input
    {
    };
    Input input;

    Button2 play_button;
    Button2 pass_button;

    void update(float timestep, Scenes scenes, BoardController *board_controller, InputState *input_state, RpcClient *rpc_client)
    {
        Font *font = scenes.assets->get_font(FONT_ROBOTO_CONDENSED_REGULAR, 64);

        glEnable(GL_BLEND);
        play_button.rect = {
            25.f,
            1080 - 25.f,
            0.2f * 1920,
            0.2f * 1920 * 9.f / 16.f / 2.f,
        };
        play_button.text = "PLAY";
        pass_button.rect = anchor_bottom_right(
            {0.2f * 1920, 0.2f * 1920 * 9.f / 16.f / 2.f},
            {25.f, 25.f});
        pass_button.text = "PASS";

        if (play_button.update_and_draw(scenes.main->target, input_state, font))
        {
            rpc_client->InGameChoosePassOrPlay({true});
        }
        if (pass_button.update_and_draw(scenes.main->target, input_state, font))
        {
            rpc_client->InGameChoosePassOrPlay({false});
        }
        glDisable(GL_BLEND);
    }
    bool ready()
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
    void update(float timestep, Scenes scenes, BoardController *board_controller, InputState *input_state, RpcClient *rpc_client)
    {
        Font *font = scenes.assets->get_font(FONT_ROBOTO_CONDENSED_REGULAR, 64);

        t += timestep;

        step(
            AllocatedString<256> combined = player_name;
            combined.append(" buzzed.");
            scenes.ui_controller->popup_banner(combined);)
    }
    bool ready()
    {
        return t > length;
    }
};

struct PromptForAnswerSequence : Sequence
{
    ScriptDefinition def()
    {
        return {
            "PromptForAnswerSequence",
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
    float time_remaining;

    bool me_is_answering;
    TextBox2<32> textbox;
    PlayerName answerer;

    void reset(bool me_is_answering, PlayerName answerer)
    {
        this->me_is_answering = me_is_answering;
        this->answerer = answerer;
        this->time_remaining = 10.f;
    }
    void update(float timestep, Scenes scenes, BoardController *board_controller, InputState *input_state, RpcClient *rpc_client)
    {
        Font *font = scenes.assets->get_font(FONT_ROBOTO_CONDENSED_REGULAR, 64);

        time_remaining -= timestep;
        scenes.ui_controller->answer_timer(true, time_remaining);
        
        if (me_is_answering)
        {
            textbox.rect = {1000, 1000, 300, 100};
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            textbox.update_and_draw(scenes.main->target, input_state, font);
            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);

            for (int i = 0; i < input_state->key_input.len; i++)
            {
                if (input_state->key_input[i] == Keys::ENTER)
                {
                    rpc_client->InGameAnswer({textbox.text});
                }
            }
        }
        else
        {
            step(
                AllocatedString<256> combined = answerer;
                combined.append(" is answering.");
                scenes.ui_controller->popup_banner(combined);)

        }
    }
    bool ready()
    {
        return false;
    }
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

    void update(float timestep, Scenes scenes, BoardController *board_controller, InputState *input_state, RpcClient *rpc_client)
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

    bool ready()
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

    void update(float timestep, Scenes scenes, BoardController *board_controller, InputState *input_state, RpcClient *rpc_client)
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

    bool ready()
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
    void update(float timestep, Scenes scenes, BoardController *board_controller, InputState *input_state, RpcClient *rpc_client)
    {
        Font *font = scenes.assets->get_font(FONT_ROBOTO_CONDENSED_REGULAR, 64);

        t += timestep;
        float pos_x = 200 + sinf(t * 1000) * 20;
        float pos_y = 500 + sinf(t * 1000) * 20;

        step(scenes.ui_controller->popup_banner(answer, 5.f))
    }
    bool ready()
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
                {"bar 1", &input.bars[0], EntityType::MESH},
                {"bar 2", &input.bars[1], EntityType::MESH},
                {"bar 3", &input.bars[2], EntityType::MESH},
                {"bar 4", &input.bars[3], EntityType::MESH},
                {"bar 5", &input.bars[4], EntityType::MESH},
                {"bar 6", &input.bars[5], EntityType::MESH},
                {"bar 7", &input.bars[6], EntityType::MESH},
                {"bar 8", &input.bars[7], EntityType::MESH},
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
    void update(float timestep, Scenes scenes, BoardController *board_controller, InputState *input_state, RpcClient *rpc_client)
    {
        scenes.main->active_camera_id = input.camera;
        if (t == 0)
            board_controller->flip(scenes.main, scenes.assets, answer_i, answer, score);

        t += timestep;
        // set scene camera to board_closeup_camera
        // wait x seconds
        // flip animation
        // wait x seconds
        // ready
    }
    bool ready()
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

    void update(float timestep, Scenes scenes, BoardController *board_controller, InputState *input_state, RpcClient *rpc_client)
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
    bool ready()
    {
        return false;
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

    void update(float timestep, Scenes scenes, BoardController *board_controller, InputState *input_state, RpcClient *rpc_client)
    {
        // sweeping camera
        // move host back to faceoff table
        // round N graphic on screen
    }
    bool ready()
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

    void update(float timestep, Scenes scenes, BoardController *board_controller, InputState *input_state, RpcClient *rpc_client)
    {
        // show winner
    }
    bool ready()
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

        // start intro cinematic
        intro_sequence.reset(scenes);
        current_sequence = &intro_sequence;
    }
    void update(float, Scenes, RpcClient *, InputState *);
    bool handle_rpcs(Scenes, RpcClient *); // returns true if game is over

    // TODO there really should be a way to create a static array at compile time or something why is it so hard
    std::vector<ScriptDefinition> get_script_defs()
    {
        return {
            intro_sequence.def(),
            round_start_sequence.def(),
            faceoff_start_sequence.def(),
            ask_question_sequence.def(),
            player_buzzed_sequence.def(),
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
    scenes.player_controller = &player_controller;
    scenes.ui_controller = &ui_controller;

    handle_rpcs(scenes, rpc_client);

    current_sequence->update(timestep, scenes, &board_controller, input_state, rpc_client);
    if (current_sequence->ready() && !sent_ready)
    {
        rpc_client->InGameReady({});
        sent_ready = true;
    }

    board_controller.update(timestep, scenes.main, scenes.assets);
    player_controller.update(timestep, scenes.main, scenes.assets);
    ui_controller.update(timestep, scenes.main, scenes.assets);
}

// returns true if game is over
bool Game::handle_rpcs(Scenes scenes, RpcClient *rpc_client)
{
    if (auto msg = rpc_client->get_InGameStartRound_msg())
    {
        printf("get_InGameStartRound_msg\n");
        if (current_sequence != &round_start_sequence)
        {
            game_data.game_state.round++;
            game_data.game_state.round_stage = RoundStage::START;

            round_start_sequence.reset(scenes, 0);
            current_sequence = &round_start_sequence;
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGameStartFaceoff_msg())
    {
        printf("get_InGameStartFaceoff_msg\n");
        if (current_sequence != &faceoff_start_sequence)
        {
            game_data.game_state.round_stage = RoundStage::FACEOFF;

            faceoff_start_sequence.reset(scenes, 0);
            current_sequence = &faceoff_start_sequence;
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGameAskQuestion_msg())
    {
        printf("get_InGameAskQuestion_msg\n");
        if (current_sequence != &ask_question_sequence)
        {
            game_data.game_state.question = msg->question;

            ask_question_sequence.reset(msg->question, msg->num_answers);
            current_sequence = &ask_question_sequence;
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGamePlayerBuzzed_msg())
    {
        printf("get_InGamePlayerBuzzed_msg\n");
        if (current_sequence != &player_buzzed_sequence)
        {
            player_buzzed_sequence.reset(game_data.game_state.get_player_data(msg->user_id)->name);
            current_sequence = &player_buzzed_sequence;
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGamePromptForAnswer_msg())
    {
        printf("get_InGamePromptForAnswer_msg\n");
        if (current_sequence != &prompt_for_answer_sequence)
        {
            prompt_for_answer_sequence.reset(msg->user_id == game_data.my_id, game_data.game_state.get_player_data(msg->user_id)->name);
            current_sequence = &prompt_for_answer_sequence;
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGamePlayerAnswered_msg())
    {
        printf("get_InGamePlayerAnswered_msg\n");
        if (current_sequence != &player_answered_sequence)
        {
            player_answered_sequence.reset(msg->answer);
            current_sequence = &player_answered_sequence;
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGamePromptPassOrPlay_msg())
    {
        printf("get_InGamePromptPassOrPlay_msg\n");
        if (current_sequence != &pass_or_play_sequence)
        {
            current_sequence = &pass_or_play_sequence;
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
        if (current_sequence != &start_play_sequence)
        {
            start_play_sequence.reset();
            current_sequence = &start_play_sequence;
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGameStartSteal_msg())
    {
        printf("get_InGameStartSteal_msg\n");
        if (current_sequence != &start_steal_sequence)
        {
            start_steal_sequence.reset();
            current_sequence = &start_steal_sequence;
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGameFlipAnswer_msg())
    {
        printf("get_InGameFlipAnswer_msg\n");
        if (current_sequence != &flip_answer_sequence)
        {
            flip_answer_sequence.reset(msg->answer, msg->answer_index, msg->score);
            current_sequence = &flip_answer_sequence;
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGameEggghhhh_msg())
    {
        printf("get_InGameEggghhhh_msg\n");
        if (current_sequence != &eeeeggghhh_sequence)
        {
            current_sequence = &eeeeggghhh_sequence;
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGameEndRound_msg())
    {
        printf("get_InGameEndRound_msg\n");
        if (current_sequence != &end_round_sequence)
        {
            current_sequence = &end_round_sequence;
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGameEndGame_msg())
    {
        printf("get_InGameEndGame_msg\n");
        if (current_sequence != &end_game_sequence)
        {
            current_sequence = &end_game_sequence;
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