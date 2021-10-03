#pragma once

#include "spline.hpp"
#include "scene/scene.hpp"
#include "net/generated_rpc_client.hpp"

struct ScriptDefinition
{
    struct InputDef
    {
        String name;
        int *value;
        EntityType entity_type;
    };

    String name;
    std::vector<InputDef> inputs;
};

struct Sequence
{
    virtual void update(float, Scene *) = 0;
    virtual bool ready() = 0;
};

struct IntroSequence : Sequence
{
    ScriptDefinition def()
    {
        return {
            "Intro Sequence",
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

    void update(float timestep, Scene *scene)
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
    void update(float timestep, Scene *scene)
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
    void update(float timestep, Scene *scene)
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
            {
                {"score screen", &input.screen, EntityType::MESH},
            },
        };
    }

    struct Input
    {
        int screen;
    };
    Input input;

    AllocatedString<128> question;
    RenderTarget screen_render_target;

    void init(Scene *scene)
    {
        screen_render_target = RenderTarget(1024, 1024, TextureFormat::RGBA8, TextureFormat::NONE);

        Entity *screen = scene->get(input.screen);
        if (screen)
        {
            screen->material->textures[screen->material->num_textures - 1] = screen_render_target.color_tex;
        }
    }

    void reset(String question)
    {
        this->question = string_to_allocated_string<128>(question);
    }
    void update(float timestep, Scene *scene)
    {
        Entity *screen = scene->get(input.screen);
        if (screen)
        {
            screen_render_target.bind();
            screen_render_target.clear();
            float aspect_ratio = 2.f;
            float text_scale = 4.f;
            float target_border = 0.05f;
            Rect sub_target = {0, 0,
                               .8f * screen_render_target.width,
                               .5f * screen_render_target.height};
            draw_centered_text(scene->font, screen_render_target, question, sub_target, target_border, text_scale, aspect_ratio);
            screen_render_target.color_tex.gen_mipmaps();
        }
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
    Sequence *current_sequence;

    bool sent_ready = false;

    void init(Scene *scene)
    {
        // TODO ask server for player avatars?

        ask_question_sequence.init(scene);

        intro_sequence.reset(scene);
        current_sequence = &intro_sequence;
        // start intro cinematic
        // wait for
    }
    void update(float, Scene *, RpcClient *);
    bool handle_rpcs(Scene *, RpcClient *); // returns true if game is over

    // TODO there really should be a way to create a static array at compile time or something why is it so hard
    std::vector<ScriptDefinition> get_script_defs()
    {
        return {
            intro_sequence.def(),
            round_start_sequence.def(),
            faceoff_start_sequence.def(),
            ask_question_sequence.def(),
        };
    }
};

void Game::update(float timestep, Scene *scene, RpcClient *rpc_client)
{
    handle_rpcs(scene, rpc_client);

    current_sequence->update(timestep, scene);
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
    else if (auto msg = rpc_client->get_InGamePromptPassOrPlay_msg())
    {
        // round_start_sequence.reset();
        // current_sequence = &round_start_sequence;
    }
    else if (auto msg = rpc_client->get_InGamePlayerBuzzed_msg())
    {
        // round_start_sequence.reset();
        // current_sequence = &round_start_sequence;
    }
    else if (auto msg = rpc_client->get_InGamePromptForAnswer_msg())
    {
        // round_start_sequence.reset();
        // current_sequence = &round_start_sequence;
    }
    else if (auto msg = rpc_client->get_InGameStartPlay_msg())
    {
        // round_start_sequence.reset();
        // current_sequence = &round_start_sequence;
    }
    else if (auto msg = rpc_client->get_InGameAnswer_msg())
    {
        // round_start_sequence.reset();
        // current_sequence = &round_start_sequence;
    }
    else if (auto msg = rpc_client->get_InGameFlipAnswer_msg())
    {
        // round_start_sequence.reset();
        // current_sequence = &round_start_sequence;
    }
    else if (auto msg = rpc_client->get_InGameEggghhhh_msg())
    {
        // round_start_sequence.reset();
        // current_sequence = &round_start_sequence;
    }
    else if (auto msg = rpc_client->get_InGameEndRound_msg())
    {
        // round_start_sequence.reset();
        // current_sequence = &round_start_sequence;
    }
    else if (auto msg = rpc_client->get_InGameEndGame_msg())
    {
        // round_start_sequence.reset();
        // current_sequence = &round_start_sequence;

        return true;
    }
    return false;
}