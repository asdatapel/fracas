#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <mutex>
#include <thread>

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

Font font;

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


Bitmap albedo;
Bitmap normal;
Bitmap metal;
Bitmap roughness;
Texture albedo_tex;
Texture normal_tex;
Texture metal_tex;
Texture roughness_tex;

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

        albedo = parse_bitmap(read_entire_file("resources/models/albedo.bmp"));
        albedo_tex = to_texture(albedo, true);
        normal = parse_bitmap(read_entire_file("resources/models/normal.bmp"));
        normal_tex = to_texture(normal, true);
        metal = parse_bitmap(read_entire_file("resources/models/metal.bmp"));
        metal_tex = to_texture(metal, true);
        roughness = parse_bitmap(read_entire_file("resources/models/roughness.bmp"));
        roughness_tex = to_texture(roughness, true);

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

        font = get_font_consolas();

        ui_state = ServerMessageType::DESCRIBE_LOBBY;
    }

    return true;
}

bool game_update(const float time_step, InputState *input_state, RenderTarget target)
{
    if (!init_if_not())
        return false;

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

    for (int i = 0; i < input_state->key_input.len; i++)
    {
        switch (input_state->key_input[i])
        {
        case Keys::S:
        {
            int this_msg_size = sizeof(uint16_t) + sizeof(ClientMessageType);
            char msg_data[MAX_MSG_SIZE];
            char *buf_pos = msg_data;
            buf_pos = append_short(buf_pos, this_msg_size);
            buf_pos = append_byte(buf_pos, (char)ClientMessageType::READY);

            server.send_all(msg_data, this_msg_size);
            printf("Sent READY\n");
        }
        break;
        default:
            break;
        }
    }

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

    switch (ui_state)
    {
    case ServerMessageType::DESCRIBE_LOBBY:
    {
        // for (int i = 0; i < 12; i++)
        // {
        //     float rect_width = 400;
        //     float rect_height = 100;
        //     float left = (target.width / 2) - rect_width - 10;
        //     float top = (target.height / 2) - (rect_height * 2) - (10 * 2);

        //     float this_left = (i % 2) * (rect_width + 10) + left;
        //     float this_top = (i / 2) * (rect_height + 10) + top;
        //     draw_rect(target, {this_left, this_top, rect_width, rect_height}, {i * .1f, (i % 4) * .25f, (i % 3) * .25f, 255});
        //     if (players[i].len != 0)
        //     {
        //         draw_string(target, &font, this_left, this_top + rect_height / 2, 1.f, players[i]);
        //     }
        // }

        // static AllocatedString<32> username;
        // do_text_box(&username, &ui_context, target, input_state, &font, &username, {500, 900, 300, 50}, 8, {1, 0, 0, .8});

        // String buttonstr = String::from("Set Name");
        // bool set_name = do_button(&buttonstr, &ui_context, target, input_state, &font, buttonstr, {850, 900, 0, 50}, 8, {0, 0, 1, .8});
        // if (set_name)
        // {
        //     int this_msg_size = sizeof(uint16_t) + sizeof(ClientMessageType) + sizeof(uint16_t) + username.len;
        //     if (this_msg_size > MAX_MSG_SIZE)
        //     {
        //         printf("Msg too big: %d", this_msg_size);
        //         return true;
        //     }

        //     char msg_data[MAX_MSG_SIZE];
        //     char *buf_pos = msg_data;
        //     buf_pos = append_short(buf_pos, this_msg_size);
        //     buf_pos = append_byte(buf_pos, (char)ClientMessageType::JOIN);
        //     buf_pos = append_string(buf_pos, username);

        //     server.send_all(msg_data, this_msg_size);
        // }

        // if (my_id == 0)
        // {
        //     String buttonstr = String::from("Start Game");
        //     bool start_game = do_button(&buttonstr, &ui_context, target, input_state, &font, buttonstr, {25, 25, 0, 75}, 15, {0, 1, 1, .8});
        //     if (start_game)
        //     {
        //         int this_msg_size = sizeof(uint16_t) + sizeof(ClientMessageType);
        //         char msg_data[MAX_MSG_SIZE];
        //         char *buf_pos = msg_data;
        //         buf_pos = append_short(buf_pos, this_msg_size);
        //         buf_pos = append_byte(buf_pos, (char)ClientMessageType::START);

        //         server.send_all(msg_data, this_msg_size);
        //     }
        // }
    }
    break;
    case ServerMessageType::START_GAME:
    {
        String str = String::from("Welcome to Family Feud!");
        do_label(&str, &ui_context, target, input_state, &font, str, {500, 900, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::START_ROUND:
    {
        char round_char = (round_num % 10) + '0';
        AllocatedString<32> str;
        str = String::from("Round ");
        str.append(round_char);

        do_label(&str, &ui_context, target, input_state, &font, str, {500, 900, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::START_FACEOFF:
    {
        String str = String::from("Now Facing Off:");
        do_label(nullptr, &ui_context, target, input_state, &font, str, {10, 10, 0, 50}, {1, 0, 0, .8});
        do_label(nullptr, &ui_context, target, input_state, &font, players[faceoffer0_i], {10, 70, 0, 50}, {1, 0, 0, .8});
        do_label(nullptr, &ui_context, target, input_state, &font, players[faceoffer1_i], {10, 130, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::ASK_QUESTION:
    {
        String str = String::from("Answer this question:");
        do_label(nullptr, &ui_context, target, input_state, &font, str, {10, 10, 0, 50}, {1, 0, 0, .8});
        do_label(nullptr, &ui_context, target, input_state, &font, question, {10, 130, 0, 50}, {1, 0, 0, .8});

        String buttonstr = String::from("BUZZ");
        bool buzz = do_button(&buttonstr, &ui_context, target, input_state, &font, buttonstr, {850, 900, 0, 50}, 8, {0, 0, 1, .8});
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
        do_label(nullptr, &ui_context, target, input_state, &font, question, {10, 130, 0, 50}, {1, 0, 0, .8});

        if (answering_player_i == my_id)
        {
            static AllocatedString<32> answer;
            do_text_box(&answer, &ui_context, target, input_state, &font, &answer, {500, 900, 300, 50}, 8, {1, 0, 0, .8});

            String buttonstr = String::from("Submit");
            bool submit = do_button(&buttonstr, &ui_context, target, input_state, &font, buttonstr, {850, 900, 0, 50}, 8, {0, 0, 1, .8});
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
            do_label(nullptr, &ui_context, target, input_state, &font, str, {10, 190, 0, 50}, {1, 0, 0, .8});
        }
    }
    break;
    case ServerMessageType::PLAYER_BUZZED:
    {
        AllocatedString<64> str;
        str = players[answering_player_i];
        str.append(String::from(" buzzed"));
        do_label(nullptr, &ui_context, target, input_state, &font, str, {10, 190, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::PLAYER_SAID_SOMETHING:
    {
        AllocatedString<64> str;
        str = players[answering_player_i];
        str.append(String::from(" said:"));
        do_label(nullptr, &ui_context, target, input_state, &font, str, {10, 130, 0, 50}, {1, 0, 0, .8});
        do_label(nullptr, &ui_context, target, input_state, &font, what_player_said, {10, 190, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::DO_A_FLIP:
    {
        AllocatedString<32> str;
        char rank_char = (flip_answer_rank % 10) + '0';
        str = String::from("Correct Answer with rank: ");
        str.append(rank_char);

        do_label(nullptr, &ui_context, target, input_state, &font, str, {10, 130, 0, 50}, {1, 0, 0, .8});
        do_label(nullptr, &ui_context, target, input_state, &font, flip_answer_text, {10, 190, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::DO_AN_EEEEEGGGHHHH:
    {
        char incorrects_char = (incorrects % 10) + '0';
        AllocatedString<32> str;
        str = String::from("EEEGGGGHHH x");
        str.append(incorrects_char);
        do_label(nullptr, &ui_context, target, input_state, &font, str, {10, 190, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::PROMPT_PASS_OR_PLAY:
    {
        char pass_or_play;
        String pass_str = String::from("PASS");
        bool pass = do_button(&pass_str, &ui_context, target, input_state, &font, pass_str, {10, 130, 0, 50}, 8, {0, 0, 1, .8});
        if (pass)
        {
            pass_or_play = 0;
        }

        String play_str = String::from("PLAY");
        bool play = do_button(&play_str, &ui_context, target, input_state, &font, play_str, {10, 190, 0, 50}, 8, {0, 0, 1, .8});
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
        do_label(nullptr, &ui_context, target, input_state, &font, str, {10, 130, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::START_STEAL:
    {
        AllocatedString<64> str;
        str = String::from("Family ");
        str.append(playing_family + '0');
        str.append(String::from(" can now steal!"));
        do_label(nullptr, &ui_context, target, input_state, &font, str, {10, 130, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::END_ROUND:
    {
        AllocatedString<64> str1;
        str1 = String::from("Round ");
        str1.append(round_num % 10 + '0');
        str1.append(String::from(" is now over :("));
        do_label(nullptr, &ui_context, target, input_state, &font, str1, {10, 70, 0, 50}, {1, 0, 0, .8});

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

    draw_threed(target, albedo_tex, normal_tex, metal_tex, roughness_tex);

    return true;
}

void deinit()
{
    closesocket(server_socket);
    WSACleanup();
}