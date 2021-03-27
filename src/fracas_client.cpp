#include <stdio.h>
#include <mutex>
#include <thread>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "stb/stb_image.hpp"
#include "stb/stb_truetype.hpp"

#include "assets.hpp"
#include "camera.hpp"
#include "font.hpp"
#include "mesh.hpp"
#include "net.hpp"
#include "platform.hpp"
#include "ui.hpp"

#include "graphics_opengl.cpp"
#include "platform_windows.cpp"

#pragma comment(lib, "ws2_32.lib")

WSADATA wsa;
SOCKET server_socket;
Peer server;

Assets assets;

const int MAX_PLAYERS = 12;
AllocatedString<32> players[MAX_PLAYERS];

float animation_wait = 0.f;

// game data
int my_id = -1;
ServerMessageType ui_state;

char round_num;
char faceoffer0_i, faceoffer1_i;
AllocatedString<128> question;
uint64_t answer_end_time;
char answering_player_i;
char buzzing_family;
char faceoff_winner;
char playing_family;
AllocatedString<128> what_player_said;
char flip_answer_rank;
AllocatedString<128> flip_answer_text;
uint16_t flip_answer_score;
char incorrects;
char round_winner;
uint16_t family0_score, family1_score;

void handleJOIN_RESPONSE(char *data)
{
    char id;
    read_byte(data, &id);
    my_id = id;

    printf("received %s\n", __func__);
    printf("my_id: %d\n", my_id);
}

void handleDESCRIBE_LOBBY(char *data)
{
    char count;
    data = read_byte(data, &count);

    printf("received %s\n", __func__);
    printf("//////////////////////////////////////\n");
    for (int i = 0; i < count && i < MAX_PLAYERS; i++)
    {
        data = read_string(data, &players[i]);
        printf("\tIn lobby: %.*s\n", players[i].len, players[i].data);
    }
    printf("//////////////////////////////////////\n");
}

void handleSTART_GAME(char *data)
{
    ui_state = ServerMessageType::START_GAME;
    animation_wait = 4.f;

    printf("received %s\n", __func__);
}

void handleSTART_ROUND(char *data)
{
    read_byte(data, &round_num);

    ui_state = ServerMessageType::START_ROUND;
    animation_wait = 3.f;

    printf("received %s\n", __func__);
}

void handleSTART_FACEOFF(char *data)
{
    data = read_byte(data, &faceoffer0_i);
    data = read_byte(data, &faceoffer1_i);

    ui_state = ServerMessageType::START_FACEOFF;
    animation_wait = 4.f;

    printf("received %s\n", __func__);
    printf("faceoffers: %d, %d\n", faceoffer0_i, faceoffer1_i);
}

void handleASK_QUESTION(char *data)
{
    read_string(data, &question);

    ui_state = ServerMessageType::ASK_QUESTION;

    printf("received %s\n", __func__);
    printf("question:  %.*s\n", question.len, question.data);
}

void handlePROMPT_FOR_ANSWER(char *data)
{
    data = read_byte(data, &answering_player_i);
    data = read_long(data, &answer_end_time);
    printf("Sending timestamp: %llu\n", answer_end_time);
    ui_state = ServerMessageType::PROMPT_FOR_ANSWER;

    printf("received %s\n", __func__);
    printf("player: %d\n", answering_player_i);
}

void handlePLAYER_BUZZED(char *data)
{
    data = read_byte(data, &buzzing_family);

    ui_state = ServerMessageType::PLAYER_BUZZED;
    animation_wait = 4.f;

    printf("received %s\n", __func__);
    printf("buzzing_family: %d\n", buzzing_family);
}

void handlePROMPT_PASS_OR_PLAY(char *data)
{
    read_byte(data, &faceoff_winner);

    ui_state = ServerMessageType::PROMPT_PASS_OR_PLAY;

    printf("received %s\n", __func__);
    printf("player: %d\n", faceoff_winner);
}

void handleSTART_PLAY(char *data)
{
    data = read_byte(data, &playing_family);

    ui_state = ServerMessageType::START_PLAY;
    animation_wait = 4.f;

    printf("received %s\n", __func__);
    printf("playing_family: %d\n", playing_family);
}

void handleSTART_STEAL(char *data)
{
    data = read_byte(data, &playing_family);

    ui_state = ServerMessageType::START_STEAL;
    animation_wait = 4.f;

    printf("received %s\n", __func__);
    printf("playing_family: %d\n", playing_family);
}

void handlePLAYER_SAID_SOMETHING(char *data)
{
    read_string(data, &what_player_said);

    ui_state = ServerMessageType::PLAYER_SAID_SOMETHING;
    animation_wait = 4.f;

    printf("received %s\n", __func__);
    printf("what_player_said: %.*s\n", what_player_said.len, what_player_said.data);
}

void handleDO_A_FLIP(char *data)
{
    data = read_byte(data, &flip_answer_rank);
    data = read_string(data, &flip_answer_text);
    data = read_short(data, &flip_answer_score);

    ui_state = ServerMessageType::DO_A_FLIP;
    animation_wait = 4.f;

    printf("received %s\n", __func__);
    printf("answer rank: %d, text: %.*s, score: %d\n", flip_answer_rank, flip_answer_text.len, flip_answer_text.data, flip_answer_score);
}

void handleDO_AN_EEEEEGGGHHHH(char *data)
{
    read_byte(data, &incorrects);

    ui_state = ServerMessageType::DO_AN_EEEEEGGGHHHH;
    animation_wait = 4.f;

    printf("received %s\n", __func__);
    printf("incorrects: %d\n", incorrects);
}

void handleEND_ROUND(char *data)
{
    data = read_byte(data, &round_winner); // -1 if no one won
    data = read_short(data, &family0_score);
    data = read_short(data, &family1_score);

    ui_state = ServerMessageType::END_ROUND;
    animation_wait = 4.f;

    printf("received %s\n", __func__);
    printf("winner: %d, family0 score: %d, family1 score: %d\n", round_winner, family0_score, family1_score);
}

typedef void (*HandleFunc)(char *);
HandleFunc handle_funcs[(int)ServerMessageType::INVALID] = {
    handleJOIN_RESPONSE,         // JOIN_RESPONSE
    handleDESCRIBE_LOBBY,        // DESCRIBE_LOBBY
    handleSTART_GAME,            // START_GAME
    handleSTART_ROUND,           // START_ROUND
    handleSTART_FACEOFF,         // START_FACEOFF
    handleASK_QUESTION,          // ASK_QUESTION
    handlePROMPT_PASS_OR_PLAY,   // PROMPT_PASS_OR_PLAY
    handlePROMPT_FOR_ANSWER,     // PROMPT_FOR_ANSWER
    handlePLAYER_BUZZED,         // PLAYER_BUZZED
    handleSTART_PLAY,            // START_PLAY
    handleSTART_STEAL,           // START_STEAL
    handlePLAYER_SAID_SOMETHING, // PLAYER_SAID_SOMETHING
    handleDO_A_FLIP,             // DO_A_FLIP
    handleDO_AN_EEEEEGGGHHHH,    // DO_AN_EEEEEGGGHHHH
    handleEND_ROUND,             // END_ROUND
};

bool init_if_not()
{
    static bool initted = false;
    if (!initted)
    {
        initted = true;

        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        {
            printf("Failed. Error Code : %d", WSAGetLastError());
            return false;
        }
        if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
        {
            printf("Could not create socket : %d", WSAGetLastError());
        }
        sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr.S_un.S_addr);
        server_address.sin_port = htons(6519);
        // if (connect(server_socket, (sockaddr *)&server_address, sizeof(server_address)) != 0)
        // {
        //     printf("Could not connect to server : %d", WSAGetLastError());
        // }
        {
            u_long mode = 1;
            ioctlsocket(server_socket, FIONBIO, (unsigned long *)&mode);
        }
        server = {server_socket};

        assets = load_assets();
        ui_state = ServerMessageType::DESCRIBE_LOBBY;
    }

    return true;
}

void font_stuff(RenderTarget target, Assets *assets, InputState *input, int index, String answer, int score)
{
    Font font = assets->fonts[(int)FontId::RESOURCES_FONTS_ANTON_REGULAR_TTF];
    Texture *num_texs[8] = {
        &assets->textures[(int)TextureAssetId::RESOURCES_MODELS_BAR3_NUM_1_BMP],
        &assets->textures[(int)TextureAssetId::RESOURCES_MODELS_BAR3_NUM_2_BMP],
        &assets->textures[(int)TextureAssetId::RESOURCES_MODELS_BAR3_NUM_3_BMP],
        &assets->textures[(int)TextureAssetId::RESOURCES_MODELS_BAR3_NUM_4_BMP],
        &assets->textures[(int)TextureAssetId::RESOURCES_MODELS_BAR3_NUM_5_BMP],
        &assets->textures[(int)TextureAssetId::RESOURCES_MODELS_BAR3_NUM_6_BMP],
        &assets->textures[(int)TextureAssetId::RESOURCES_MODELS_BAR3_NUM_7_BMP],
        &assets->textures[(int)TextureAssetId::RESOURCES_MODELS_BAR3_NUM_8_BMP],
    };

    float aspect_ratio = 2.f;
    float text_scale = 4.f;
    {
        float target_border = 0.05f;
        Rect sub_target = {target_border, target_border, .8f - (target_border * 2), .5f - (target_border * 2)};
        draw_centered_text(font, target, answer, sub_target, text_scale, aspect_ratio);
    }

    {
        assert(score > 0 && score < 100);
        char buf[3];
        _itoa_s(score, buf, 10);
        String text;
        text.data = buf;
        text.len = strlen(buf);

        float target_border = 0.025f;
        Rect sub_target = {1 - .19f + target_border, target_border, .19f - target_border - target_border, .5f - target_border};
        draw_centered_text(font, target, text, sub_target, text_scale, aspect_ratio);
    }

    {
        Texture num_tex = *num_texs[index];
        float img_width = (float)num_tex.width / target.width;
        float img_height = (float)num_tex.height / target.height;

        float border = .04f;
        float scale = (.5f - (border * 2.f)) / (img_height * aspect_ratio);
        float height = img_height * scale;
        float width = img_width * scale;

        float top = .5f + border;
        float left = (1.f - width) / 2.f;

        Rect rect = {left * target.width, top * target.height, target.width * width, target.height * height * aspect_ratio};
        draw_textured_rect(target, rect, {}, num_tex);
    }
}

void graphics_test_stuff(RenderTarget target, Assets *assets, InputState *input)
{
    static Texture hdri_tex;
    static Texture unfiltered_cubemap;
    static StandardPbrEnvMaterial env_mat;

    static RenderTarget answer_textures[8];
    static RenderTarget score_textures[2];

    static bool init = false;
    if (!init)
    {
        init = true;

        {
            int width, height, components;
            stbi_set_flip_vertically_on_load(true);
            float *hdri = stbi_loadf("resources/hdri/Newport_Loft_Ref.hdr", &width, &height, &components, 0);
            hdri_tex = to_texture(hdri, width, height);
            stbi_image_free(hdri);

            unfiltered_cubemap = hdri_to_cubemap(hdri_tex, 1024);

            Texture irradiance_map = convolve_irradiance_map(unfiltered_cubemap, 32);
            Texture env_map = filter_env_map(unfiltered_cubemap, 512);
            Texture brdf_lut = generate_brdf_lut(512);
            env_mat.texture_array[0] = irradiance_map;
            env_mat.texture_array[1] = env_map;
            env_mat.texture_array[2] = brdf_lut;
        }

        for (int i = 0; i < 8; i++)
        {
            answer_textures[i] = new_render_target(2048, 2048, false);
        }
        for (int i = 0; i < 2; i++)
        {
            score_textures[i] = new_render_target(1024, 1024, false);
        }
    }

    static String answers[8] = {
        String::from("RED ASJKDD ASKJHDQQW"),
        String::from("BLUE"),
        String::from("ORANGE"),
        String::from("GREEN"),
        String::from("PURPLE"),
        String::from("VIOLET"),
        String::from("PINK"),
        String::from("CYAN"),
    };
    for (int i = 0; i < 8; i++)
    {
        bind(answer_textures[i]);
        clear_backbuffer();
        font_stuff(answer_textures[i], assets, input, i, answers[i], i + 8);
        gen_mips(answer_textures[i].color_tex);
    }

    bind(target);
    clear_backbuffer();

    static Camera camera;
    camera.update(target, input);

    VertexBuffer x_verts = assets->vertex_buffers[(int)MeshId::RESOURCES_MODELS_X2];
    StandardPbrMaterial x_mat = assets->materials[(int)MaterialId::RESOURCES_MODELS_X2];
    VertexBuffer bar_verts = assets->vertex_buffers[(int)MeshId::RESOURCES_MODELS_BAR];
    StandardPbrMaterial bar_mat = assets->materials[(int)MaterialId::RESOURCES_MODELS_BAR];
    VertexBuffer test_verts = assets->vertex_buffers[(int)MeshId::RESOURCES_MODELS_LONG_TABLE];
    StandardPbrMaterial test_mat = assets->materials[(int)MaterialId::RESOURCES_MODELS_LONG_TABLE];

    static float bar_ts[8] = {};
    static bool animating[8] = {};
    for (int i = 0; i < input->key_input.len; i++)
    {
        if (input->key_input[i] >= Keys::NUM_1 && input->key_input[i] <= Keys::NUM_8)
        {
            animating[(int)input->key_input[i] - (int)Keys::NUM_1] = !animating[(int)input->key_input[i] - (int)Keys::NUM_1];
        }
    }
    for (int i = 0; i < 8; i++)
    {
        float x = (i % 2) * 4.f;
        float y = (i / 2) * -0.8f;

        if (animating[i])
        {
            bar_ts[i] += 0.01f;
        }
        float bar_rot = (powf(bar_ts[i] * 4, 2) * 90.f) - 90.f;
        if (bar_rot < -90.f)
        {
            bar_rot = -90.f;
        }
        if (bar_rot > 90.f)
        {
            bar_rot = 90.f;
        }

        glm::mat4 model = glm::rotate(glm::translate(glm::mat4(1.0f), {x, y, 0.f}), glm::radians(bar_rot), glm::vec3{1.f, 0.f, 0.f});

        bind_shader(bar_shader);
        bind_camera(bar_shader, camera);
        bind_mat4(bar_shader, UniformId::MODEL, model);
        bind_material(bar_shader, env_mat);
        bind_material(bar_shader, bar_mat);
        bind_texture(bar_shader, UniformId::OVERLAY_TEXTURE, answer_textures[i].color_tex);
        draw(target, bar_shader, bar_verts);
    }

    float speed_denoms[3] = {2, 2.75, 2.15};
    for (int i = 0; i < 3; i++)
    {
        float t = 0.f;
        t += 0.01f;
        glm::vec3 initial_pos = {-1 + i, 0, 5};
        glm::quat initial_rot({(i * .5f), .1f + (i * .1f), -0.5f + (i * 1.4f)});
        glm::vec3 target_pos = {i - 1, 0.f, 0.f};
        glm::quat target_rot({glm::radians(90.f), 0.f, 0.f});
        float actual_t = 1.f - powf(glm::clamp(t / 2, 0.f, 1.f), speed_denoms[i]);
        glm::vec3 actual_pos = initial_pos + ((target_pos - initial_pos) * actual_t);
        glm::quat actual_rot = glm::normalize(initial_rot + ((target_rot - initial_rot) * actual_t));
        glm::mat4 model = glm::translate(glm::mat4(1.0f), actual_pos) * glm::toMat4(actual_rot);

        bind_shader(threed_shader);
        bind_camera(threed_shader, camera);
        bind_mat4(threed_shader, UniformId::MODEL, model);
        bind_material(threed_shader, x_mat);
        bind_material(threed_shader, env_mat);
        draw(target, threed_shader, x_verts);
    }

    static float t = 0.f;
    t += 0.01f;
    glm::mat4 model = glm::rotate(glm::translate(glm::mat4(1.0f), {0.f, 2.f, 0.f}), glm::radians(t), glm::vec3{0.f, 1.f, 0.f});
    bind_shader(threed_shader);
    bind_camera(threed_shader, camera);
    bind_mat4(threed_shader, UniformId::MODEL, model);
    bind_material(threed_shader, env_mat);
    bind_material(threed_shader, test_mat);
    draw(target, threed_shader, test_verts);

    draw_cubemap(unfiltered_cubemap, camera);
}

bool game_update(const float time_step, InputState *input_state, RenderTarget target)
{
    if (!init_if_not())
        return false;

    graphics_test_stuff(target, &assets, input_state);

    if (animation_wait)
    {
        animation_wait -= time_step;
        if (animation_wait < 0)
        {
            animation_wait = 0;
            int this_msg_size = sizeof(uint16_t) + sizeof(ClientMessageType);
            char msg_data[MAX_MSG_SIZE];
            char *buf_pos = msg_data;
            buf_pos = append_short(buf_pos, this_msg_size);
            buf_pos = append_byte(buf_pos, (char)ClientMessageType::READY);

            server.send_all(msg_data, this_msg_size);
            printf("Sent READY\n");
        }
    }

    // for (int i = 0; i < input_state->key_input.len; i++)
    // {
    //     switch (input_state->key_input[i])
    //     {
    //     case Keys::S:
    //     {
    //         int this_msg_size = sizeof(uint16_t) + sizeof(ClientMessageType);
    //         char msg_data[MAX_MSG_SIZE];
    //         char *buf_pos = msg_data;
    //         buf_pos = append_short(buf_pos, this_msg_size);
    //         buf_pos = append_byte(buf_pos, (char)ClientMessageType::READY);

    //         server.send_all(msg_data, this_msg_size);
    //         printf("Sent READY\n");
    //     }
    //     break;
    //     default:
    //         break;
    //     }
    // }

    int msg_len;
    char msg[MAX_MSG_SIZE];
    // while ((msg_len = server.recieve_msg(msg)) > 0)
    // {
    //     ServerMessageType msg_type;
    //     char *data = read_byte(msg, (char *)&msg_type);

    //     if (msg_type >= ServerMessageType::INVALID)
    //         DEBUG_PRINT("There has been an error\n");

    //     HandleFunc handle_func = handle_funcs[(uint8_t)msg_type];
    //     handle_func(data);
    // }

    static UiContext ui_context;
    Font font = assets.fonts[(int)FontId::RESOURCES_FONTS_ANTON_REGULAR_TTF];

    switch (ui_state)
    {
    case ServerMessageType::DESCRIBE_LOBBY:
    {
        for (int i = 0; i < 12; i++)
        {
            float rect_width = 400;
            float rect_height = 100;
            float left = (target.width / 2) - rect_width - 10;
            float top = (target.height / 2) - (rect_height * 2) - (10 * 2);

            float this_left = (i % 2) * (rect_width + 10) + left;
            float this_top = (i / 2) * (rect_height + 10) + top;
            draw_rect(target, {this_left, this_top, rect_width, rect_height}, {i * .1f, (i % 4) * .25f, (i % 3) * .25f, 255});
            if (players[i].len != 0)
            {
                draw_text(font, target, players[i], this_left, this_top + rect_height / 2, 1.f, 1.f);
            }
        }

        static AllocatedString<32> username;
        do_text_box(&username, &ui_context, target, input_state, font, &username, {500, 900, 300, 50}, 8, {1, 0, 0, .8});

        String buttonstr = String::from("Set Name");
        bool set_name = do_button(&buttonstr, &ui_context, target, input_state, font, buttonstr, {850, 900, 0, 50}, 8, {0, 0, 1, .8});
        if (set_name)
        {
            int this_msg_size = sizeof(uint16_t) + sizeof(ClientMessageType) + sizeof(uint16_t) + username.len;
            if (this_msg_size > MAX_MSG_SIZE)
            {
                printf("Msg too big: %d", this_msg_size);
                return true;
            }

            char msg_data[MAX_MSG_SIZE];
            char *buf_pos = msg_data;
            buf_pos = append_short(buf_pos, this_msg_size);
            buf_pos = append_byte(buf_pos, (char)ClientMessageType::JOIN);
            buf_pos = append_string(buf_pos, username);

            server.send_all(msg_data, this_msg_size);
        }

        if (my_id == 0)
        {
            String buttonstr = String::from("Start Game");
            bool start_game = do_button(&buttonstr, &ui_context, target, input_state, font, buttonstr, {25, 25, 0, 75}, 15, {0, 1, 1, .8});
            if (start_game)
            {
                int this_msg_size = sizeof(uint16_t) + sizeof(ClientMessageType);
                char msg_data[MAX_MSG_SIZE];
                char *buf_pos = msg_data;
                buf_pos = append_short(buf_pos, this_msg_size);
                buf_pos = append_byte(buf_pos, (char)ClientMessageType::START);

                server.send_all(msg_data, this_msg_size);
            }
        }
    }
    break;
    case ServerMessageType::START_GAME:
    {
        String str = String::from("Welcome to Family Feud!");
        do_label(&str, &ui_context, target, input_state, font, str, {500, 900, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::START_ROUND:
    {
        char round_char = (round_num % 10) + '0';
        AllocatedString<32> str;
        str = String::from("Round ");
        str.append(round_char);

        do_label(&str, &ui_context, target, input_state, font, str, {500, 900, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::START_FACEOFF:
    {
        String str = String::from("Now Facing Off:");
        do_label(nullptr, &ui_context, target, input_state, font, str, {10, 10, 0, 50}, {1, 0, 0, .8});
        do_label(nullptr, &ui_context, target, input_state, font, players[faceoffer0_i], {10, 70, 0, 50}, {1, 0, 0, .8});
        do_label(nullptr, &ui_context, target, input_state, font, players[faceoffer1_i], {10, 130, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::ASK_QUESTION:
    {
        String str = String::from("Answer this question:");
        do_label(nullptr, &ui_context, target, input_state, font, str, {10, 10, 0, 50}, {1, 0, 0, .8});
        do_label(nullptr, &ui_context, target, input_state, font, question, {10, 130, 0, 50}, {1, 0, 0, .8});

        String buttonstr = String::from("BUZZ");
        bool buzz = do_button(&buttonstr, &ui_context, target, input_state, font, buttonstr, {850, 900, 0, 50}, 8, {0, 0, 1, .8});
        if (buzz)
        {
            int this_msg_size = sizeof(uint16_t) + sizeof(ClientMessageType);
            char msg_data[MAX_MSG_SIZE];
            char *buf_pos = msg_data;
            buf_pos = append_short(buf_pos, this_msg_size);
            buf_pos = append_byte(buf_pos, (char)ClientMessageType::BUZZ);

            server.send_all(msg_data, this_msg_size);
            printf("Sent BUZZ\n");
        }
    }
    break;
    case ServerMessageType::PROMPT_FOR_ANSWER:
    {
        do_label(nullptr, &ui_context, target, input_state, font, question, {10, 130, 0, 50}, {1, 0, 0, .8});

        if (answering_player_i == my_id)
        {
            static AllocatedString<32> answer;
            do_text_box(&answer, &ui_context, target, input_state, font, &answer, {500, 900, 300, 50}, 8, {1, 0, 0, .8});

            String buttonstr = String::from("Submit");
            bool submit = do_button(&buttonstr, &ui_context, target, input_state, font, buttonstr, {850, 900, 0, 50}, 8, {0, 0, 1, .8});
            if (submit)
            {
                char msg[MAX_MSG_SIZE];
                char *buf_pos = append_byte(msg + 2, (char)ClientMessageType::ANSWER);
                buf_pos = append_string(buf_pos, answer);
                uint16_t msg_len = buf_pos - msg;
                append_short(msg, msg_len);

                server.send_all(msg, msg_len);
                printf("Sent ANSWER\n");
            }
        }
        else
        {
            AllocatedString<64> str;
            str = players[answering_player_i];
            str.append(String::from(" is answering"));
            do_label(nullptr, &ui_context, target, input_state, font, str, {10, 190, 0, 50}, {1, 0, 0, .8});
        }
    }
    break;
    case ServerMessageType::PLAYER_BUZZED:
    {
        AllocatedString<64> str;
        str = players[answering_player_i];
        str.append(String::from(" buzzed"));
        do_label(nullptr, &ui_context, target, input_state, font, str, {10, 190, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::PLAYER_SAID_SOMETHING:
    {
        AllocatedString<64> str;
        str = players[answering_player_i];
        str.append(String::from(" said:"));
        do_label(nullptr, &ui_context, target, input_state, font, str, {10, 130, 0, 50}, {1, 0, 0, .8});
        do_label(nullptr, &ui_context, target, input_state, font, what_player_said, {10, 190, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::DO_A_FLIP:
    {
        AllocatedString<32> str;
        char rank_char = (flip_answer_rank % 10) + '0';
        str = String::from("Correct Answer with rank: ");
        str.append(rank_char);

        do_label(nullptr, &ui_context, target, input_state, font, str, {10, 130, 0, 50}, {1, 0, 0, .8});
        do_label(nullptr, &ui_context, target, input_state, font, flip_answer_text, {10, 190, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::DO_AN_EEEEEGGGHHHH:
    {
        char incorrects_char = (incorrects % 10) + '0';
        AllocatedString<32> str;
        str = String::from("EEEGGGGHHH x");
        str.append(incorrects_char);
        do_label(nullptr, &ui_context, target, input_state, font, str, {10, 190, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::PROMPT_PASS_OR_PLAY:
    {
        char pass_or_play;
        String pass_str = String::from("PASS");
        bool pass = do_button(&pass_str, &ui_context, target, input_state, font, pass_str, {10, 130, 0, 50}, 8, {0, 0, 1, .8});
        if (pass)
        {
            pass_or_play = 0;
        }

        String play_str = String::from("PLAY");
        bool play = do_button(&play_str, &ui_context, target, input_state, font, play_str, {10, 190, 0, 50}, 8, {0, 0, 1, .8});
        if (play)
        {
            pass_or_play = 1;
        }

        if (pass || play)
        {
            char msg[MAX_MSG_SIZE];
            char *buf_pos = append_byte(msg + 2, (char)ClientMessageType::PASS_OR_PLAY);
            buf_pos = append_byte(buf_pos, pass_or_play);
            uint16_t msg_len = buf_pos - msg;
            append_short(msg, msg_len);

            server.send_all(msg, msg_len);
            printf("Sent PASS_OR_PLAY\n");
        }
    }
    break;
    case ServerMessageType::START_PLAY:
    {
        AllocatedString<64> str;
        str = String::from("Family ");
        str.append(playing_family + '0');
        str.append(String::from(" is now playing!"));
        do_label(nullptr, &ui_context, target, input_state, font, str, {10, 130, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::START_STEAL:
    {
        AllocatedString<64> str;
        str = String::from("Family ");
        str.append(playing_family + '0');
        str.append(String::from(" can now steal!"));
        do_label(nullptr, &ui_context, target, input_state, font, str, {10, 130, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::END_ROUND:
    {
        AllocatedString<64> str1;
        str1 = String::from("Round ");
        str1.append(round_num % 10 + '0');
        str1.append(String::from(" is now over :("));
        do_label(nullptr, &ui_context, target, input_state, font, str1, {10, 70, 0, 50}, {1, 0, 0, .8});

        char family0_score_buf[32];
        sprintf(family0_score_buf, "%d", family0_score);
        AllocatedString<64> str2;
        str2 = String::from("Family 0 score: ");
        str2.append(family0_score_buf);

        char family1_score_buf[32];
        sprintf(family1_score_buf, "%d", family1_score);
        AllocatedString<64> str3;
        str3 = String::from("Family 1 score: ");
        str3.append(family1_score_buf);
    }
    break;
    default:
        break;
    }

    return true;
}

void deinit()
{
    closesocket(server_socket);
    WSACleanup();
}