#pragma once

#include "spline.hpp"
#include "scene/scene.hpp"
#include "net/generated_rpc_client.hpp"

#include "main_menu.hpp"

struct PlayerData
{
    ClientId id;
    PlayerName name;
    int family;
};

struct ClientGameState
{
    ClientId my_id;

    Array<PlayerName, 2> family_names = {{}, {}};
    Array<PlayerData, MAX_PLAYERS_PER_GAME / 2> players = {};

    ClientId faceoffer_0, faceoffer_1;
};

struct Sequence
{
    virtual void update(float, Scene *, InputState *, RpcClient *) = 0;
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

    void reset(Scene *scene)
    {
        t = 0;
    }

    void update(float timestep, Scene *scene, InputState *input_state, RpcClient *rpc_client)
    {
        t += timestep;
        if (t > length)
        {
            t = length;
        }

        Entity *camera = scene->get(input.camera);
        Entity *path = scene->get(input.path);
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

    void reset(Scene *scene, int round)
    {
        t = 0;
    }
    void update(float timestep, Scene *scene, InputState *input_state, RpcClient *rpc_client)
    {
        t += timestep;
        if (t > length)
        {
            t = length;
        }

        Entity *camera = scene->get(input.camera);
        Entity *path = scene->get(input.path);
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

    void reset(Scene *scene, int round)
    {
        t = 0;
    }
    void update(float timestep, Scene *scene, InputState *input_state, RpcClient *rpc_client)
    {
        t += timestep;
        if (t > length)
        {
            t = length;
        }

        Entity *camera = scene->get(input.camera);
        Entity *path = scene->get(input.path);
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

    struct Input
    {
    };
    Input input;

    AllocatedString<128> question;
    Button2 buzz_button;

    void init(Scene *scene)
    {
        // screen_render_target = RenderTarget(1024, 1024, TextureFormat::RGBA8, TextureFormat::NONE);

        // Entity *screen = scene->get(input.screen);
        // if (screen)
        // {
        //     screen->material->textures[screen->material->num_textures - 1] = screen_render_target.color_tex;
        // }
    }

    void reset(String question)
    {
        this->question = string_to_allocated_string<128>(question);
    }
    void update(float timestep, Scene *scene, InputState *input_state, RpcClient *rpc_client)
    {
        glEnable(GL_BLEND);
        draw_text(scene->font, scene->hdr_target, question, 10, 10, 2, 2);
        glDisable(GL_BLEND);

        buzz_button.rect = anchor_bottom_right(
            {0.2f * 1920, 0.2f * 1920 * 9.f / 16.f / 2.f},
            {25.f, 25.f});
        buzz_button.text = "BUZZ";

        if (buzz_button.update_and_draw(scene->hdr_target, input_state, &scene->font))
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

    void update(float timestep, Scene *scene, InputState *input_state, RpcClient *rpc_client)
    {
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

        if (play_button.update_and_draw(scene->hdr_target, input_state, &scene->font))
        {
            rpc_client->InGameChoosePassOrPlay({true});
        }
        if (pass_button.update_and_draw(scene->hdr_target, input_state, &scene->font))
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
        this->player_name = player_name;
    }
    void update(float timestep, Scene *scene, InputState *input_state, RpcClient *rpc_client)
    {
        float pos_x = 200 + sinf(t * 1000) * 20;
        float pos_y = 500 + sinf(t * 1000) * 20;

        glEnable(GL_BLEND);
        draw_text(scene->font, scene->hdr_target, player_name, 10, 10, 2, 2);
        glDisable(GL_BLEND);
    }
    bool ready()
    {
        return false;
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

    TextBox2<32> textbox;

    void update(float timestep, Scene *scene, InputState *input_state, RpcClient *rpc_client)
    {
        textbox.rect = {1000, 1000, 300, 100};
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        textbox.update_and_draw(scene->hdr_target, input_state, &scene->font);
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

    void update(float timestep, Scene *scene, InputState *input_state, RpcClient *rpc_client)
    {
        t += timestep;
        if (t > length)
        {
            t = length;
        }

        Entity *camera = scene->get(input.camera);
        Entity *path = scene->get(input.path);
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

    void update(float timestep, Scene *scene, InputState *input_state, RpcClient *rpc_client)
    {
        t += timestep;
        if (t > length)
        {
            t = length;
        }

        Entity *camera = scene->get(input.camera);
        Entity *path = scene->get(input.path);
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
        this->answer = answer;
    }
    void update(float timestep, Scene *scene, InputState *input_state, RpcClient *rpc_client)
    {
        float pos_x = 200 + sinf(t * 1000) * 20;
        float pos_y = 500 + sinf(t * 1000) * 20;

        glEnable(GL_BLEND);
        draw_text(scene->font, scene->hdr_target, answer, pos_x, pos_y, 2, 2);
        glDisable(GL_BLEND);
    }
    bool ready()
    {
        return false;
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
    const float length = 3.f;

    AllocatedString<64> answer;
    int answer_i;
    int score;

    void reset(AllocatedString<64> answer, int answer_i, int score)
    {
        this->answer = answer;
        this->answer_i = answer_i;
        this->score = score;
    }
    void update(float timestep, Scene *scene, InputState *input_state, RpcClient *rpc_client)
    {
        t += timestep;

        scene->selected_camera_id = input.camera;

        float rotation = 180 * (t / length);
        if (rotation > 180.f)
            rotation = 180.f;
        Entity *bar = scene->get(input.bars[answer_i]);
        bar->transform.rotation.x = glm::radians(rotation);

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
    const float length = 3.f;

    void update(float timestep, Scene *scene, InputState *input_state, RpcClient *rpc_client)
    {
        // show Xs
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

    void update(float timestep, Scene *scene, InputState *input_state, RpcClient *rpc_client)
    {
        // end round
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

    void update(float timestep, Scene *scene, InputState *input_state, RpcClient *rpc_client)
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

    ClientGameState game_state;

    void init(Scene *scene)
    {
        // TODO ask server for player avatars?

        ask_question_sequence.init(scene);

        // start intro cinematic
        intro_sequence.reset(scene);
        current_sequence = &intro_sequence;
    }
    void update(float, Scene *, RpcClient *, InputState *);
    bool handle_rpcs(Scene *, RpcClient *); // returns true if game is over

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

void Game::update(float timestep, Scene *scene, RpcClient *rpc_client, InputState *input_state)
{
    handle_rpcs(scene, rpc_client);

    current_sequence->update(timestep, scene, input_state, rpc_client);
    if (current_sequence->ready() && !sent_ready)
    {
        rpc_client->InGameReady({});
        sent_ready = true;
    }
}

// returns true if game is over
bool Game::handle_rpcs(Scene *scene, RpcClient *rpc_client)
{
    if (auto msg = rpc_client->get_InGameStartRound_msg())
    {
        if (current_sequence != &round_start_sequence)
        {
            round_start_sequence.reset(scene, 0);
            current_sequence = &round_start_sequence;
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGameStartFaceoff_msg())
    {
        if (current_sequence != &faceoff_start_sequence)
        {
            game_state.faceoffer_0 = msg->faceoffer_0_id;
            game_state.faceoffer_1 = msg->faceoffer_1_id;
            
            faceoff_start_sequence.reset(scene, 0);
            current_sequence = &faceoff_start_sequence;
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGameAskQuestion_msg())
    {
        if (current_sequence != &ask_question_sequence)
        {
            ask_question_sequence.reset(msg->question);
            current_sequence = &ask_question_sequence;
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGamePlayerBuzzed_msg())
    {
        printf("get_InGamePlayerBuzzed_msg\n");
        if (current_sequence != &player_buzzed_sequence)
        {
            PlayerName player_name;
            player_name.data[0] = msg->user_id + 48;
            player_name.len = 1;
            player_buzzed_sequence.reset(player_name);
            current_sequence = &player_buzzed_sequence;
            sent_ready = false;
        }
    }
    else if (auto msg = rpc_client->get_InGamePromptForAnswer_msg())
    {
        printf("get_InGamePromptForAnswer_msg\n");
        if (current_sequence != &prompt_for_answer_sequence)
        {
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