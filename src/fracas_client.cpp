#include <stdio.h>
#include <mutex>
#include <thread>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "assets.hpp"
#include "camera.hpp"
#include "debug_ui.hpp"
#include "font.hpp"
#include "mesh.hpp"
#include "main_menu.hpp"
#include "net/net.hpp"
#include "net/generated_rpc_client.hpp"
#include "platform.hpp"
#include "scene/scene.hpp"
#include "editor.hpp"
#include "ui.hpp"

#include "graphics/graphics_opengl.cpp"
#include "platform_windows.cpp"
#include "net/net.cpp"


// JoinGameMessage join_game_message;
// DescribeGameMessage describe_game_message;

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

void handle_LIST_GAMES(MessageReader *msg)
{
    while (msg->data < msg->end)
    {
        // uint16_t index;
        // // msg->read(&index);
        // if (index == 0)
        // {
        //     MainMenu::join_game_page.games.len = 0;
        // }

        // MainMenu::join_game_page.games.len++;
        // GameId game_id;
        // // msg->read(&game_id);
        // GameProperties game_properties;
        // read(msg, &game_properties);
        // MainMenu::join_game_page.games[index].name = game_properties.name;
    }
}

// void handle_JOIN_GAME_RESPONSE(MessageReader *msg)
// {
//     read(msg, &join_game_message);

//     printf("received %s\n", __func__);
//     printf("my_id: %d\n", join_game_message.my_id);
// }

// void handle_DESCRIBE_LOBBY(MessageReader *msg)
// {
//     read(msg, &describe_game_message);

//     printf("received %s\n", __func__);
//     printf("//////////////////////////////////////\n");
//     for (int i = 0; i < describe_game_message.num_players; i++)
//     {
//         printf("\tIn lobby: %.*s\n", describe_game_message.players[i].len, describe_game_message.players[i].data);
//     }
//     printf("//////////////////////////////////////\n");
// }

// void handleSTART_GAME(MessageReader *msg)
// {
//     ui_state = ServerMessageType::START_GAME;
//     animation_wait = 4.f;

//     printf("received %s\n", __func__);
// }

// void handleSTART_ROUND(MessageReader *msg)
// {
//     // msg->read(&round_num);

//     ui_state = ServerMessageType::START_ROUND;
//     animation_wait = 3.f;

//     printf("received %s\n", __func__);
// }

// void handleSTART_FACEOFF(MessageReader *msg)
// {
//     // msg->read(&faceoffer0_i);
//     // msg->read(&faceoffer1_i);

//     ui_state = ServerMessageType::START_FACEOFF;
//     animation_wait = 4.f;

//     printf("received %s\n", __func__);
//     printf("faceoffers: %d, %d\n", faceoffer0_i, faceoffer1_i);
// }

// void handleASK_QUESTION(MessageReader *msg)
// {
//     // msg->read(&question);

//     ui_state = ServerMessageType::ASK_QUESTION;

//     printf("received %s\n", __func__);
//     printf("question:  %.*s\n", question.len, question.data);
// }

// void handlePROMPT_FOR_ANSWER(MessageReader *msg)
// {
//     // msg->read(&answering_player_i);
//     // msg->read(&answer_end_time);

//     printf("Sending timestamp: %llu\n", answer_end_time);
//     ui_state = ServerMessageType::PROMPT_FOR_ANSWER;

//     printf("received %s\n", __func__);
//     printf("player: %d\n", answering_player_i);
// }

// void handlePLAYER_BUZZED(MessageReader *msg)
// {
//     // msg->read(&buzzing_family);

//     ui_state = ServerMessageType::PLAYER_BUZZED;
//     animation_wait = 4.f;

//     printf("received %s\n", __func__);
//     printf("buzzing_family: %d\n", buzzing_family);
// }

// void handlePROMPT_PASS_OR_PLAY(MessageReader *msg)
// {
//     // msg->read(&faceoff_winner);

//     ui_state = ServerMessageType::PROMPT_PASS_OR_PLAY;

//     printf("received %s\n", __func__);
//     printf("player: %d\n", faceoff_winner);
// }

// void handleSTART_PLAY(MessageReader *msg)
// {
//     // msg->read(&playing_family);

//     ui_state = ServerMessageType::START_PLAY;
//     animation_wait = 4.f;

//     printf("received %s\n", __func__);
//     printf("playing_family: %d\n", playing_family);
// }

// void handleSTART_STEAL(MessageReader *msg)
// {
//     // msg->read(&playing_family);

//     ui_state = ServerMessageType::START_STEAL;
//     animation_wait = 4.f;

//     printf("received %s\n", __func__);
//     printf("playing_family: %d\n", playing_family);
// }

// void handlePLAYER_SAID_SOMETHING(MessageReader *msg)
// {
//     // msg->read(&what_player_said);

//     ui_state = ServerMessageType::PLAYER_SAID_SOMETHING;
//     animation_wait = 4.f;

//     printf("received %s\n", __func__);
//     printf("what_player_said: %.*s\n", what_player_said.len, what_player_said.data);
// }

// void handleDO_A_FLIP(MessageReader *msg)
// {
//     // msg->read(&flip_answer_rank);
//     // msg->read(&flip_answer_text);
//     // msg->read(&flip_answer_score);

//     ui_state = ServerMessageType::DO_A_FLIP;
//     animation_wait = 4.f;

//     printf("received %s\n", __func__);
//     printf("answer rank: %d, text: %.*s, score: %d\n", flip_answer_rank, flip_answer_text.len, flip_answer_text.data, flip_answer_score);
// }

// void handleDO_AN_EEEEEGGGHHHH(MessageReader *msg)
// {
//     // msg->read(&incorrects);

//     ui_state = ServerMessageType::DO_AN_EEEEEGGGHHHH;
//     animation_wait = 4.f;

//     printf("received %s\n", __func__);
//     printf("incorrects: %d\n", incorrects);
// }

// void handleEND_ROUND(MessageReader *msg)
// {
//     // msg->read(&round_winner);
//     // msg->read(&family0_score);
//     // msg->read(&family1_score);

//     ui_state = ServerMessageType::END_ROUND;
//     animation_wait = 4.f;

//     printf("received %s\n", __func__);
//     printf("winner: %d, family0 score: %d, family1 score: %d\n", round_winner, family0_score, family1_score);
// }

// typedef void (*HandleFunc)(MessageReader *);
// HandleFunc handle_funcs[(int)ServerMessageType::INVALID] = {
//     handle_LIST_GAMES,         // LIST_GAMES
//     handle_JOIN_GAME_RESPONSE, // JOIN_GAME_RESPONSE

//     // handleJOIN_RESPONSE,         // JOIN_RESPONSE
//     // handleDESCRIBE_LOBBY,        // DESCRIBE_LOBBY
//     // handleSTART_GAME,            // START_GAME
//     // handleSTART_ROUND,           // START_ROUND
//     // handleSTART_FACEOFF,         // START_FACEOFF
//     // handleASK_QUESTION,          // ASK_QUESTION
//     // handlePROMPT_PASS_OR_PLAY,   // PROMPT_PASS_OR_PLAY
//     // handlePROMPT_FOR_ANSWER,     // PROMPT_FOR_ANSWER
//     // handlePLAYER_BUZZED,         // PLAYER_BUZZED
//     // handleSTART_PLAY,            // START_PLAY
//     // handleSTART_STEAL,           // START_STEAL
//     // handlePLAYER_SAID_SOMETHING, // PLAYER_SAID_SOMETHING
//     // handleDO_A_FLIP,             // DO_A_FLIP
//     // handleDO_AN_EEEEEGGGHHHH,    // DO_AN_EEEEEGGGHHHH
//     // handleEND_ROUND,             // END_ROUND
// };

Peer server;

Assets assets;
Scene scene;
Editor editor;
Font ui_font;

StackAllocator allocator;
StackAllocator temp;
Memory memory{&allocator, &temp};

float animation_wait = 0.f;

// game data
ServerMessageType ui_state;

bool init_if_not()
{
    static bool initted = false;
    if (!initted)
    {
        initted = true;

        init_net();
        server.open("127.0.0.1", 6519, false);

        allocator.init(1024ull * 1024 * 1024 * 2); // 2gb
        temp.init(1024 * 1024 * 50);               // 50 mb
        assets.init(memory);
        scene.init(&assets, memory);
        ui_state = ServerMessageType::INVALID;
        ui_font = load_font(assets.font_files[(int)FontId::RESOURCES_FONTS_ROBOTOCONDENSED_LIGHT_TTF], 32, memory.temp);

        imm_init(&assets, memory);

        // MainMenu::init(&assets);
    }

    return true;
}

struct ClientData
{
    MainMenu main_menu;
    MainPage main;
    SettingsPage settings;
    CreateGamePage create;
    JoinGamePage join;
    LobbyPage lobby;

    ClientData(Assets *assets, RpcClient *client, Memory memory)
        : main(assets, memory),
          settings(assets, client, memory),
          create(assets, client, memory),
          join(assets, client, memory),
          lobby(assets, client, memory)
    {
        main_menu.main = &main;
        main_menu.settings = &settings;
        main_menu.create = &create;
        main_menu.join = &join;
        main_menu.lobby = &lobby;
        main_menu.current = main_menu.main;
    }
};

void RpcClient::HandleStartGame(StartGameRequest *req)
{
    client_data->main_menu.current = nullptr;
}

void RpcClient::HandleStartGame(StartGameRequest *req)
{
    client_data->main_menu.current = nullptr;
}

bool game_update(const float time_step, InputState *input_state, RenderTarget main_target)
{
    if (!init_if_not())
        return false;

    static RpcClient client("127.0.0.1", 6666);
    static ClientData client_data(&assets, &client, memory);

    static bool inited = false;
    if (!inited)
    {
        inited = true;

        client.client_data = &client_data;
    }

    int msg_len;
    char msg[MAX_MSG_SIZE];
    if ((msg_len = client.peer.recieve_msg(msg)) > 0)
    {
        if (client.handle_rpc(msg, msg_len))
        {
            client.peer.pop_message();
        }
    }

    if (client_data.main_menu.current)
    {
        main_target.bind();

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        { // background
            static float t = 0;
            t += time_step;
            bind_shader(blurred_colors_shader);
            bind_1f(blurred_colors_shader, UniformId::T, t);
            bind_2i(blurred_colors_shader, UniformId::RESOLUTION, main_target.width, main_target.height);
            bind_2f(blurred_colors_shader, UniformId::POS, 0, 0);
            bind_2f(blurred_colors_shader, UniformId::SCALE, main_target.width, main_target.height);
            draw_rect();
        }

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        client_data.main_menu.current->update_and_draw(main_target, input_state, &client_data.main_menu);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }
    else
    {
        editor.update_and_draw(&scene, main_target, input_state, memory);
    }

    // if (animation_wait)
    // {
    //     animation_wait -= time_step;
    //     if (animation_wait < 0)
    //     {
    //         animation_wait = 0;
    //         int this_msg_size = sizeof(uint16_t) + sizeof(ClientMessageType);
    //         char msg_data[MAX_MSG_SIZE];
    //         char *buf_pos = msg_data;
    //         buf_pos = append_short(buf_pos, this_msg_size);
    //         buf_pos = append_byte(buf_pos, (char)ClientMessageType::READY);

    //         server.send_all(msg_data, this_msg_size);
    //         printf("Sent READY\n");
    //     }
    // }

    for (int i = 0; i < input_state->key_input.len; i++)
    {
        switch (input_state->key_input[i])
        {
        // case Keys::S:
        // {
        //     int this_msg_size = sizeof(uint16_t) + sizeof(ClientMessageType);
        //     char msg_data[MAX_MSG_SIZE];
        //     char *buf_pos = msg_data;
        //     buf_pos = append_short(buf_pos, this_msg_size);
        //     buf_pos = append_byte(buf_pos, (char)ClientMessageType::READY);

        //     server.send_all(msg_data, this_msg_size);
        //     printf("Sent READY\n");
        // }
        // break;
        case Keys::A:
        {
            // ListGamesRequest req;
            // ListGamesResponse resp;
            // client.ListGame(&req, &resp);
            // printf("%i\n", resp.game.id);
        }
        break;
        default:
            break;
        }
    }

    static UiContext ui_context;

    glDisable(GL_DEPTH_TEST);

    // switch (ui_state)
    // {
    // case ServerMessageType::DESCRIBE_LOBBY:
    // {
    //     for (int i = 0; i < 12; i++)
    //     {
    //         float rect_width = 400;
    //         float rect_height = 100;
    //         float left = (target.width / 2) - rect_width - 10;
    //         float top = (target.height / 2) - (rect_height * 2) - (10 * 2);

    //         float this_left = (i % 2) * (rect_width + 10) + left;
    //         float this_top = (i / 2) * (rect_height + 10) + top;

    //         Color color = {i * .1f, (i % 4) * .25f, (i % 3) * .25f, 255};
    //         bind_shader(basic_shader);
    //         bind_2i(basic_shader, UniformId::RESOLUTION, target.width, target.height);
    //         bind_2f(basic_shader, UniformId::POS, this_left, this_top);
    //         bind_2f(basic_shader, UniformId::SCALE, rect_width, rect_height);
    //         bind_4f(basic_shader, UniformId::COLOR, color.r, color.g, color.b, color.a);
    //         draw_rect();

    //         if (describe_game_message.players[i].len != 0)
    //         {
    //             draw_text_cropped(ui_font, target, describe_game_message.players[i], this_left, this_top + rect_height / 2, 1.f, 1.f);
    //         }
    //     }

    //     static AllocatedString<32> username;
    //     do_text_box(&username, &ui_context, target, input_state, ui_font, &username, {500, 900, 300, 50}, 8, {1, 0, 0, .8});

    //     String buttonstr = String::from("Set Name");
    //     bool set_name = do_button(&buttonstr, &ui_context, target, input_state, ui_font, buttonstr, {850, 900, 0, 50}, 8, {0, 0, 1, .8});
    //     if (set_name)
    //     {
    //         int this_msg_size = sizeof(uint16_t) + sizeof(ClientMessageType) + sizeof(uint16_t) + username.len;
    //         if (this_msg_size > MAX_MSG_SIZE)
    //         {
    //             printf("Msg too big: %d", this_msg_size);
    //             return true;
    //         }

    //         char msg_data[MAX_MSG_SIZE];
    //         char *buf_pos = msg_data;
    //         buf_pos = append_short(buf_pos, this_msg_size);
    //         //buf_pos = append_byte(buf_pos, (char)ClientMessageType::JOIN);
    //         buf_pos = append_string(buf_pos, username);

    //         server.send_all(msg_data, this_msg_size);
    //     }

    //     if (join_game_message.my_id == 0)
    //     {
    //         String buttonstr = String::from("Start Game");
    //         bool start_game = do_button(&buttonstr, &ui_context, target, input_state, ui_font, buttonstr, {25, 25, 0, 75}, 15, {0, 1, 1, .8});
    //         if (start_game)
    //         {
    //             int this_msg_size = sizeof(uint16_t) + sizeof(ClientMessageType);
    //             char msg_data[MAX_MSG_SIZE];
    //             char *buf_pos = msg_data;
    //             buf_pos = append_short(buf_pos, this_msg_size);
    //             //buf_pos = append_byte(buf_pos, (char)ClientMessageType::START);

    //             server.send_all(msg_data, this_msg_size);
    //         }
    //     }
    // }
    // break;
    // case ServerMessageType::START_GAME:
    // {
    //     String str = String::from("Welcome to Family Feud!");
    //     do_label(&str, &ui_context, target, input_state, ui_font, str, {500, 900, 0, 50}, {1, 0, 0, .8});
    // }
    // break;
    // case ServerMessageType::START_ROUND:
    // {
    //     char round_char = (round_num % 10) + '0';
    //     AllocatedString<32> str;
    //     str = String::from("Round ");
    //     str.append(round_char);

    //     do_label(&str, &ui_context, target, input_state, ui_font, str, {500, 900, 0, 50}, {1, 0, 0, .8});
    // }
    // break;
    // case ServerMessageType::START_FACEOFF:
    // {
    //     String str = String::from("Now Facing Off:");
    //     do_label(nullptr, &ui_context, target, input_state, ui_font, str, {10, 10, 0, 50}, {1, 0, 0, .8});
    //     do_label(nullptr, &ui_context, target, input_state, ui_font, describe_game_message.players[faceoffer0_i], {10, 70, 0, 50}, {1, 0, 0, .8});
    //     do_label(nullptr, &ui_context, target, input_state, ui_font, describe_game_message.players[faceoffer1_i], {10, 130, 0, 50}, {1, 0, 0, .8});
    // }
    // break;
    // case ServerMessageType::ASK_QUESTION:
    // {
    //     String str = String::from("Answer this question:");
    //     do_label(nullptr, &ui_context, target, input_state, ui_font, str, {10, 10, 0, 50}, {1, 0, 0, .8});
    //     do_label(nullptr, &ui_context, target, input_state, ui_font, question, {10, 130, 0, 50}, {1, 0, 0, .8});

    //     String buttonstr = String::from("BUZZ");
    //     bool buzz = do_button(&buttonstr, &ui_context, target, input_state, ui_font, buttonstr, {850, 900, 0, 50}, 8, {0, 0, 1, .8});
    //     if (buzz)
    //     {
    //         int this_msg_size = sizeof(uint16_t) + sizeof(ClientMessageType);
    //         char msg_data[MAX_MSG_SIZE];
    //         char *buf_pos = msg_data;
    //         buf_pos = append_short(buf_pos, this_msg_size);
    //         buf_pos = append_byte(buf_pos, (char)ClientMessageType::BUZZ);

    //         server.send_all(msg_data, this_msg_size);
    //         printf("Sent BUZZ\n");
    //     }
    // }
    // break;
    // case ServerMessageType::PROMPT_FOR_ANSWER:
    // {
    //     do_label(nullptr, &ui_context, target, input_state, ui_font, question, {10, 130, 0, 50}, {1, 0, 0, .8});

    //     if (answering_player_i == join_game_message.my_id)
    //     {
    //         static AllocatedString<32> answer;
    //         do_text_box(&answer, &ui_context, target, input_state, ui_font, &answer, {500, 900, 300, 50}, 8, {1, 0, 0, .8});

    //         String buttonstr = String::from("Submit");
    //         bool submit = do_button(&buttonstr, &ui_context, target, input_state, ui_font, buttonstr, {850, 900, 0, 50}, 8, {0, 0, 1, .8});
    //         if (submit)
    //         {
    //             char msg[MAX_MSG_SIZE];
    //             char *buf_pos = append_byte(msg + 2, (char)ClientMessageType::ANSWER);
    //             buf_pos = append_string(buf_pos, answer);
    //             uint16_t msg_len = buf_pos - msg;
    //             append_short(msg, msg_len);

    //             server.send_all(msg, msg_len);
    //             printf("Sent ANSWER\n");
    //         }
    //     }
    //     else
    //     {
    //         AllocatedString<64> str;
    //         str = describe_game_message.players[answering_player_i];
    //         str.append(String::from(" is answering"));
    //         do_label(nullptr, &ui_context, target, input_state, ui_font, str, {10, 190, 0, 50}, {1, 0, 0, .8});
    //     }
    // }
    // break;
    // case ServerMessageType::PLAYER_BUZZED:
    // {
    //     AllocatedString<64> str;
    //     str = describe_game_message.players[answering_player_i];
    //     str.append(String::from(" buzzed"));
    //     do_label(nullptr, &ui_context, target, input_state, ui_font, str, {10, 190, 0, 50}, {1, 0, 0, .8});
    // }
    // break;
    // case ServerMessageType::PLAYER_SAID_SOMETHING:
    // {
    //     AllocatedString<64> str;
    //     str = describe_game_message.players[answering_player_i];
    //     str.append(String::from(" said:"));
    //     do_label(nullptr, &ui_context, target, input_state, ui_font, str, {10, 130, 0, 50}, {1, 0, 0, .8});
    //     do_label(nullptr, &ui_context, target, input_state, ui_font, what_player_said, {10, 190, 0, 50}, {1, 0, 0, .8});
    // }
    // break;
    // case ServerMessageType::DO_A_FLIP:
    // {
    //     AllocatedString<32> str;
    //     char rank_char = (flip_answer_rank % 10) + '0';
    //     str = String::from("Correct Answer with rank: ");
    //     str.append(rank_char);

    //     do_label(nullptr, &ui_context, target, input_state, ui_font, str, {10, 130, 0, 50}, {1, 0, 0, .8});
    //     do_label(nullptr, &ui_context, target, input_state, ui_font, flip_answer_text, {10, 190, 0, 50}, {1, 0, 0, .8});
    // }
    // break;
    // case ServerMessageType::DO_AN_EEEEEGGGHHHH:
    // {
    //     char incorrects_char = (incorrects % 10) + '0';
    //     AllocatedString<32> str;
    //     str = String::from("EEEGGGGHHH x");
    //     str.append(incorrects_char);
    //     do_label(nullptr, &ui_context, target, input_state, ui_font, str, {10, 190, 0, 50}, {1, 0, 0, .8});
    // }
    // break;
    // case ServerMessageType::PROMPT_PASS_OR_PLAY:
    // {
    //     char pass_or_play;
    //     String pass_str = String::from("PASS");
    //     bool pass = do_button(&pass_str, &ui_context, target, input_state, ui_font, pass_str, {10, 130, 0, 50}, 8, {0, 0, 1, .8});
    //     if (pass)
    //     {
    //         pass_or_play = 0;
    //     }

    //     String play_str = String::from("PLAY");
    //     bool play = do_button(&play_str, &ui_context, target, input_state, ui_font, play_str, {10, 190, 0, 50}, 8, {0, 0, 1, .8});
    //     if (play)
    //     {
    //         pass_or_play = 1;
    //     }

    //     if (pass || play)
    //     {
    //         char msg[MAX_MSG_SIZE];
    //         char *buf_pos = append_byte(msg + 2, (char)ClientMessageType::PASS_OR_PLAY);
    //         buf_pos = append_byte(buf_pos, pass_or_play);
    //         uint16_t msg_len = buf_pos - msg;
    //         append_short(msg, msg_len);

    //         server.send_all(msg, msg_len);
    //         printf("Sent PASS_OR_PLAY\n");
    //     }
    // }
    // break;
    // case ServerMessageType::START_PLAY:
    // {
    //     AllocatedString<64> str;
    //     str = String::from("Family ");
    //     str.append(playing_family + '0');
    //     str.append(String::from(" is now playing!"));
    //     do_label(nullptr, &ui_context, target, input_state, ui_font, str, {10, 130, 0, 50}, {1, 0, 0, .8});
    // }
    // break;
    // case ServerMessageType::START_STEAL:
    // {
    //     AllocatedString<64> str;
    //     str = String::from("Family ");
    //     str.append(playing_family + '0');
    //     str.append(String::from(" can now steal!"));
    //     do_label(nullptr, &ui_context, target, input_state, ui_font, str, {10, 130, 0, 50}, {1, 0, 0, .8});
    // }
    // break;
    // case ServerMessageType::END_ROUND:
    // {
    //     AllocatedString<64> str1;
    //     str1 = String::from("Round ");
    //     str1.append(round_num % 10 + '0');
    //     str1.append(String::from(" is now over :("));
    //     do_label(nullptr, &ui_context, target, input_state, ui_font, str1, {10, 70, 0, 50}, {1, 0, 0, .8});

    //     char family0_score_buf[32];
    //     sprintf(family0_score_buf, "%d", family0_score);
    //     AllocatedString<64> str2;
    //     str2 = String::from("Family 0 score: ");
    //     str2.append(family0_score_buf);

    //     char family1_score_buf[32];
    //     sprintf(family1_score_buf, "%d", family1_score);
    //     AllocatedString<64> str3;
    //     str3 = String::from("Family 1 score: ");
    //     str3.append(family1_score_buf);
    // }
    // break;
    // default:
    //     break;
    // }

    glEnable(GL_DEPTH_TEST);

    return true;
}

void deinit()
{
    server.close();
    deinit_net();
}