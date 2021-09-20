#pragma once

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
            },
        };
    }

    const Vec3f camera_pos_start = {0, 1, 20};
    const Vec3f camera_pos_end = {0, 10, -2};
    const float length = 5.f;

    Vec3f camera_pos;
    float t;

    struct Input
    {
        int camera;
    };
    Input input;

    void init(Scene *scene)
    {
        t = 0;
        camera_pos = camera_pos_start;
    }

    void update(float timestep, Scene *scene)
    {
        t += timestep;
        if (t > length)
        {
            t = length;
        }

        camera_pos = lerp(camera_pos_start, camera_pos_end, t / length);

        Entity *camera = scene->get(input.camera);
        if (camera)
            camera->transform.position = camera_pos;
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
            },
        };
    }

    const Vec3f camera_pos_start = {0, 1, 10};
    const Vec3f camera_pos_end = {0, 2, -2};
    const float length = 10.f;

    Vec3f camera_pos;
    float t;

    struct Input
    {
        int camera;
    };
    Input input;

    void init(Scene *scene, int round)
    {
        // select camera positions
        t = 0;
        camera_pos = camera_pos_start;
    }
    void update(float timestep, Scene *scene)
    {
        t += timestep;
        if (t > length)
        {
            t = length;
        }

        camera_pos = lerp(camera_pos_start, camera_pos_end, t / length);
    }
    bool ready()
    {
        return t >= length;
    }
};

struct Game
{
    IntroSequence intro_sequence;
    FaceoffStartSequence faceoff_start_sequence;
    Sequence *current_sequence;

    void init(Scene *scene)
    {
        // TODO ask server for player avatars?

        intro_sequence.init(scene);
        current_sequence = &intro_sequence;
        // start intro cinematic
        // wait for
    }
    void update(float, Scene *, RpcClient *);
    void handle_rpcs(RpcClient *);

    // TODO there really should be a way to create a static array at compile time or something why is it so hard
    std::vector<ScriptDefinition> get_script_defs()
    {
        return {
            intro_sequence.def(),
            faceoff_start_sequence.def(),
        };
    }
};

void Game::update(float timestep, Scene *scene, RpcClient *rpc_client)
{
    handle_rpcs(rpc_client);

    current_sequence->update(timestep, scene);
    // if (current_sequence.finished())
    // {
    //  rpc_client.send_ready();
}

void Game::handle_rpcs(RpcClient *rpc_client)
{
    if (auto msg = rpc_client->get_InGameStartRound_msg())
    {
        // round_start_sequence.init();
        // current_sequence = &round_start_sequence;
    }
    else if (auto msg = rpc_client->get_InGameStartFaceoff_msg())
    {
        // round_start_sequence.init();
        // current_sequence = &round_start_sequence;
    }
    else if (auto msg = rpc_client->get_InGameAskQuestion_msg())
    {
        // round_start_sequence.init();
        // current_sequence = &round_start_sequence;
    }
    else if (auto msg = rpc_client->get_InGamePromptPassOrPlay_msg())
    {
        // round_start_sequence.init();
        // current_sequence = &round_start_sequence;
    }
    else if (auto msg = rpc_client->get_InGamePlayerBuzzed_msg())
    {
        // round_start_sequence.init();
        // current_sequence = &round_start_sequence;
    }
    else if (auto msg = rpc_client->get_InGamePromptForAnswer_msg())
    {
        // round_start_sequence.init();
        // current_sequence = &round_start_sequence;
    }
    else if (auto msg = rpc_client->get_InGameStartPlay_msg())
    {
        // round_start_sequence.init();
        // current_sequence = &round_start_sequence;
    }
    else if (auto msg = rpc_client->get_InGameAnswer_msg())
    {
        // round_start_sequence.init();
        // current_sequence = &round_start_sequence;
    }
    else if (auto msg = rpc_client->get_InGameFlipAnswer_msg())
    {
        // round_start_sequence.init();
        // current_sequence = &round_start_sequence;
    }
    else if (auto msg = rpc_client->get_InGameEggghhhh_msg())
    {
        // round_start_sequence.init();
        // current_sequence = &round_start_sequence;
    }
    else if (auto msg = rpc_client->get_InGameEndRound_msg())
    {
        // round_start_sequence.init();
        // current_sequence = &round_start_sequence;
    }
    else if (auto msg = rpc_client->get_InGameEndGame_msg())
    {
        // round_start_sequence.init();
        // current_sequence = &round_start_sequence;
    }
}