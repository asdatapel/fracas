#include <stdio.h>
#include <mutex>
#include <thread>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "assets.hpp"
#include "camera.hpp"
#include "font.hpp"
#include "mesh.hpp"
#include "net.hpp"
#include "platform.hpp"
#include "scene.hpp"
#include "ui.hpp"

#include "graphics_opengl.cpp"
#include "platform_windows.cpp"

#pragma comment(lib, "ws2_32.lib")

WSADATA wsa;
SOCKET server_socket;
Peer server;

Assets assets;
Scene scene;
Font ui_font;

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
        scene = init_scene(&assets);
        ui_state = ServerMessageType::DESCRIBE_LOBBY;
        ui_font = load_font(assets.font_files[(int)FontId::RESOURCES_FONTS_ANTON_REGULAR_TTF], 256);
    }

    return true;
}

enum struct MainMenu
{
    TITLE,
    CREATE_GAME,
    JOIN_GAME,
    SETTINGS,
    INGAME,
    EXIT,
};

float get_standard_border(RenderTarget target)
{
    return target.width / 50.f;
}

MainMenu do_create_game(UiContext2 *ui, const float time_step, RenderTarget target, InputState *input, Assets *assets)
{
    MainMenu ret = MainMenu::CREATE_GAME;
    static Font font;

    static bool init = false;
    if (!init)
    {
        init = true;
        font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ROBOTOCONDENSED_REGULAR_TTF], 64);
    }

    glDisable(GL_DEPTH_TEST);
    // button
    float standard_border = get_standard_border(target);
    float max_width = 0.2f * 1920;
    float want_width = 0.2f * target.width;
    float button_width = fminf(max_width, want_width);
    float button_height = button_width * 9.f / 16.f / 2.f;

    float center_x = target.width - standard_border - (button_width / 2.f);
    float center_y = target.height - standard_border - (button_height / 2.f);
    float left = center_x - (button_width / 2.f);
    float top = center_y - (button_height / 2.f);

    static Button back_button;
    back_button.color = {255 / 255.f, 125 / 255.f, 19 / 255.f, .7};
    back_button.rect = {left, top, button_width, button_height};
    back_button.str = String::from("Back");
    bool back = do_button2(&back_button, ui, target, input, font);
    if (back)
    {
        ret = MainMenu::TITLE;
    }

    {
        float standard_border = get_standard_border(target);
        float title_width = (target.width / 2.f) - (standard_border * 2);
        float title_height = standard_border * 2;
        Rect title_rect = {standard_border, standard_border, title_width, title_height};
        draw_rect(target, title_rect, {0, 0, 0, 0.4});
        draw_centered_text(font, target, String::from("New Game"), title_rect, .1f, 10, 1);

        float gap = (standard_border / 2.f);
        float internal_border = gap;
        float width = title_width;
        float height = target.height - (2.f * standard_border) - title_height - gap;

        Rect container_rect = {standard_border, title_rect.y + title_height + gap, width, height};
        draw_rect(target, container_rect, {0, 0, 0, 0.4});

        float x = container_rect.x + internal_border;
        float y = container_rect.y + internal_border;

        String text = String::from("Game Name:");
        float text_scale = standard_border / font.font_size_px;
        draw_text(font, target, text, x, y, text_scale, text_scale);
        y += standard_border + gap;

        static TextBox<64> game_name_text_box;
        game_name_text_box.rect = {x, y, container_rect.width - (internal_border * 2), standard_border * 2};
        game_name_text_box.color = {31 / 255.f, 121 / 255.f, 197 / 255.f, 1};
        do_text_box(&game_name_text_box, ui, target, input, font);
        y += game_name_text_box.rect.height + gap;

        text = String::from("I want to host this:");
        text_scale = standard_border / font.font_size_px;
        draw_text(font, target, text, x, y, text_scale, text_scale);
        y += standard_border + gap;

        static bool is_self_hosted = false;
        do_checkbox(&is_self_hosted, ui, target, input, font, {x, y, standard_border, standard_border}, {255 / 255.f, 125 / 255.f, 19 / 255.f, .7});

        float create_button_center_x = target.width - standard_border - (button_width / 2.f);
        float create_button_center_y = target.height - standard_border - (button_height / 2.f) - (button_height + gap);
        float create_button_left = create_button_center_x - (button_width / 2.f);
        float create_button_top = create_button_center_y - (button_height / 2.f);
        static Button create_button;
        create_button.color = {255 / 255.f, 125 / 255.f, 19 / 255.f, .7};
        create_button.rect = {create_button_left, create_button_top , button_width, button_height};
        create_button.str = String::from("Create");
        bool create = do_button2(&create_button, ui, target, input, font);
        if (create)
        {
            ret = MainMenu::TITLE;
        }
    }
    glEnable(GL_DEPTH_TEST);

    return ret;
}

MainMenu do_join_game(UiContext2 *ui, const float time_step, RenderTarget target, InputState *input, Assets *assets)
{
    MainMenu ret = MainMenu::JOIN_GAME;
    static Font font;
    static Button back_button;

    static bool init = false;
    if (!init)
    {
        init = true;
        font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ROBOTOCONDENSED_REGULAR_TTF], 64);
    }

    glDisable(GL_DEPTH_TEST);
    { // button
        float standard_border = get_standard_border(target);
        float max_width = 0.2f * 1920;
        float want_width = 0.2f * target.width;
        float width = fminf(max_width, want_width);
        float height = width * 9.f / 16.f / 2.f;

        float center_x = target.width - standard_border - (width / 2.f);
        float center_y = target.height - standard_border - (height / 2.f);
        float left = center_x - (width / 2.f);
        float top = center_y - (height / 2.f);

        back_button.color = {255 / 255.f, 125 / 255.f, 19 / 255.f, .7};
        back_button.rect = {left, top, width, height};
        back_button.str = String::from("Back");
        bool back = do_button2(&back_button, ui, target, input, font);
        if (back)
        {
            ret = MainMenu::TITLE;
        }
    }

    auto draw_list = [](RenderTarget target, UiContext2 *ui, InputState *input, Rect rect) {
        draw_rect(target, rect, {0, 0, 0, 0.4});

        float border = get_standard_border(target);
        float gap = border / 5.f;
        float item_width = rect.width - (border * 2);
        float item_height = 60; // not scaling for now

        float actual_height = rect.height - (border * 2);

        float total_item_height = 100 * (gap + item_height) - gap;
        float percent_visible = fminf(actual_height / total_item_height, 1.f);

        float scrollbar_width = border * .75f;
        float scrollbar_height = percent_visible * actual_height;
        float scrollbar_border = (border - scrollbar_width) / 2.f;
        float scrollbar_x = rect.x + rect.width - border + scrollbar_border;
        float scrollbar_max_y_offset = actual_height - scrollbar_height;

        static float scrollbar_y_offset = 0;
        for (int i = 0; i < input->key_input.len; i++)
        {
            if (input->key_input[i] == Keys::UP)
            {
                scrollbar_y_offset -= 5;
            }
            if (input->key_input[i] == Keys::DOWN)
            {
                scrollbar_y_offset += 5;
            }
        }
        if (scrollbar_y_offset < 0)
            scrollbar_y_offset = 0;
        if (scrollbar_y_offset > scrollbar_max_y_offset)
            scrollbar_y_offset = scrollbar_max_y_offset;

        float scrollbar_y = rect.y + border + scrollbar_y_offset;
        float scrollbar_percentage = scrollbar_y_offset / actual_height;
        static Rect scrollbar_rect;
        scrollbar_rect = {scrollbar_x, scrollbar_y, scrollbar_width, scrollbar_height};
        do_draggable(ui, input, &scrollbar_rect, scrollbar_rect);
        Color scrollbar_color = {255 / 255.f, 125 / 255.f, 19 / 255.f, 1};
        if (ui->focus_started == &scrollbar_rect)
        {
            scrollbar_color = darken(scrollbar_color, 0.1f);
            scrollbar_y_offset += input->mouse_y - input->prev_mouse_y;
        }
        draw_rect(target, scrollbar_rect, scrollbar_color);

        glEnable(GL_SCISSOR_TEST);
        Rect mask_rect = {rect.x + border, rect.y + border, rect.width - (border * 2), actual_height};
        glScissor(mask_rect.x, target.height - (mask_rect.y + mask_rect.height), mask_rect.width, mask_rect.height);
        static TextBox<64> server_text_boxes[100];

        for (int i = 0; i < 100; i++)
        {
            float y = -(total_item_height * scrollbar_percentage) + rect.y + border + i * (gap + item_height);
            server_text_boxes[i].rect = {rect.x + border, y, item_width, item_height};
            server_text_boxes[i].color = {31 / 255.f, 121 / 255.f, 197 / 255.f, 1};
            do_selectable(ui, input, server_text_boxes + i, server_text_boxes[i].rect, mask_rect);
            if (ui->active == server_text_boxes + i)
            {
                server_text_boxes[i].color = darken(server_text_boxes[i].color, .1f);
            }
            draw_rect(target, server_text_boxes[i].rect, server_text_boxes[i].color);
            // do_text_box(&server_text_boxes[i], ui, target, input, font);
        }
        glDisable(GL_SCISSOR_TEST);
    };
    {
        float standard_border = get_standard_border(target);
        float title_width = (target.width / 2.f) - (standard_border * 2);
        float title_height = standard_border * 2;
        Rect title_rect = {standard_border, standard_border, title_width, title_height};
        draw_rect(target, title_rect, {0, 0, 0, 0.4});
        draw_centered_text(font, target, String::from("Select Game"), title_rect, .1f, 10, 1);

        float gap = (standard_border / 2.f);
        float internal_border = gap;
        float width = title_width;
        float height = target.height - (2.f * standard_border) - title_height - gap;

        Rect container_rect = {standard_border, title_rect.y + title_height + gap, width, height};
        draw_list(target, ui, input, container_rect);
    }
    glEnable(GL_DEPTH_TEST);

    return ret;
}

MainMenu do_settings(UiContext2 *ui, const float time_step, RenderTarget target, InputState *input, Assets *assets)
{
    MainMenu ret = MainMenu::SETTINGS;
    static Font font;
    static Button back_button;

    static bool init = false;
    if (!init)
    {
        init = true;
        font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ROBOTOCONDENSED_REGULAR_TTF], 64);
    }

    float border = target.width / 50.f;

    glDisable(GL_DEPTH_TEST);
    { // button
        float max_width = 0.2f * 1920;
        float want_width = 0.2f * target.width;
        float width = fminf(max_width, want_width);
        float height = width * 9.f / 16.f / 2.f;

        float center_x = target.width - border - (width / 2.f);
        float center_y = target.height - border - (height / 2.f);
        float left = center_x - (width / 2.f);
        float top = center_y - (height / 2.f);

        back_button.color = {255 / 255.f, 125 / 255.f, 19 / 255.f, .7};
        back_button.rect = {left, top, width, height};
        back_button.str = String::from("Back");
        bool back = do_button2(&back_button, ui, target, input, font);
        if (back)
        {
            ret = MainMenu::TITLE;
        }
    }

    {
        float title_width = (target.width / 2.f) - (border * 2);
        float title_height = border * 2;
        Rect title_rect = {border, border, title_width, title_height};
        draw_rect(target, title_rect, {0, 0, 0, 0.4});
        draw_centered_text(font, target, String::from("Settings"), title_rect, .1f, 10, 1);

        float gap = (border / 2.f);
        float width = title_width;
        float height = target.height - (2.f * border) - title_height - gap;
        Rect container_rect = {border, title_rect.y + title_height + gap, width, height};
        draw_rect(target, container_rect, {0, 0, 0, 0.4});

        float internal_border = gap / 2.f;
        static TextBox<64> server_text_box;
        server_text_box.rect = {container_rect.x + internal_border, container_rect.y + internal_border, container_rect.width - (internal_border * 2), border * 2};
        server_text_box.color = {31 / 255.f, 121 / 255.f, 197 / 255.f, 1};
        do_text_box(&server_text_box, ui, target, input, font);
    }
    glEnable(GL_DEPTH_TEST);

    return ret;
}

// false if user quit
MainMenu do_title_screen(UiContext2 *ui, const float time_step, RenderTarget target, InputState *input, Assets *assets)
{
    static Font font;
    static Button buttons[4];

    static bool init = false;
    if (!init)
    {
        init = true;

        font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ROBOTOCONDENSED_REGULAR_TTF], 64);
    }

    MainMenu ret = MainMenu::TITLE;
    glDisable(GL_DEPTH_TEST);
    float border = target.width / 50.f;
    { // title text
        String text_1 = String::from("FAMILY");
        String text_2 = String::from("FEUD");

        float want_width = 0.6f * target.width;
        float max_width = 0.6f * 1920;
        float width = fminf(max_width, want_width);
        float scale = width / get_text_width(ui_font, text_1);

        float line1_height = get_single_line_height(ui_font, text_1, scale);

        draw_text_cropped(ui_font, target, text_1, border, border, scale, scale);
        draw_text_cropped(ui_font, target, text_2, border, border + 15 + line1_height, scale, scale);
    }
    { // buttons
        float max_width = 0.2f * 1920;
        float want_width = 0.2f * target.width;
        float width = fminf(max_width, want_width);
        float height = width * 9.f / 16.f / 2.f;

        String button_text[4] = {
            String::from("Exit"),
            String::from("Settings"),
            String::from("Join Game"),
            String::from("Create Game"),
        };
        for (int i = 0; i < 4; i++)
        {
            float center_x = target.width - border - (width / 2.f);
            float center_y = target.height - border - i * (15 + height) - (height / 2.f);
            float left = center_x - (width / 2.f);
            float top = center_y - (height / 2.f);

            buttons[i].color = {255 / 255.f, 125 / 255.f, 19 / 255.f, .7};
            buttons[i].rect = {left, top, width, height};
            buttons[i].str = button_text[i];
        }

        bool create_game = do_button2(&buttons[3], ui, target, input, font);
        if (create_game)
        {
            ret = MainMenu::CREATE_GAME;
        }

        bool join_game = do_button2(&buttons[2], ui, target, input, font);
        if (join_game)
        {
            ret = MainMenu::JOIN_GAME;
        }

        bool options = do_button2(&buttons[1], ui, target, input, font);
        if (options)
        {
            ret = MainMenu::SETTINGS;
        }

        bool exit = do_button2(&buttons[0], ui, target, input, font);
        if (exit)
        {
            ret = MainMenu::EXIT;
        }
    }

    glEnable(GL_DEPTH_TEST);

    return ret;
}

// false if user quit
bool do_main_menu(const float time_step, RenderTarget target, InputState *input, Assets *assets)
{
    static MainMenu menu = MainMenu::TITLE;
    static UiContext2 ui;

    if (menu != MainMenu::INGAME)
    { // background
        static float t = 0;
        t += time_step;
        bind_shader(blurred_colors_shader);
        bind_1f(blurred_colors_shader, UniformId::T, t);
        bind_2i(blurred_colors_shader, UniformId::RESOLUTION, target.width, target.height);
        bind_2f(blurred_colors_shader, UniformId::POS, 0, 0);
        bind_2f(blurred_colors_shader, UniformId::SCALE, target.width, target.height);
        draw_rect();
    }

    switch (menu)
    {
    case (MainMenu::TITLE):
    {
        menu = do_title_screen(&ui, time_step, target, input, assets);
    }
    break;
    case (MainMenu::CREATE_GAME):
    {
        menu = do_create_game(&ui, time_step, target, input, assets);
    }
    break;
    case (MainMenu::JOIN_GAME):
    {
        menu = do_join_game(&ui, time_step, target, input, assets);
    }
    break;
    case (MainMenu::SETTINGS):
    {
        menu = do_settings(&ui, time_step, target, input, assets);
    }
    break;
    case (MainMenu::INGAME):
    {
        clear_bars(target, &scene);
        draw_scene(&scene, target, input);
    }
    break;
    }

    return menu != MainMenu::EXIT;
}

bool game_update(const float time_step, InputState *input_state, RenderTarget target)
{
    if (!init_if_not())
        return false;

    bind(target);

    if (!do_main_menu(time_step, target, input_state, &assets))
    {
        return false;
    }

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

    glDisable(GL_DEPTH_TEST);

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

            // Color color = {i * .1f, (i % 4) * .25f, (i % 3) * .25f, 255};
            // bind_shader(basic_shader);
            // bind_2i(basic_shader, UniformId::RESOLUTION, target.width, target.height);
            // bind_2f(basic_shader, UniformId::POS, this_left, this_top);
            // bind_2f(basic_shader, UniformId::SCALE, rect_width, rect_height);
            // bind_4f(basic_shader, UniformId::COLOR, color.r, color.g, color.b, color.a);
            // draw_rect();

            if (players[i].len != 0)
            {
                draw_text_cropped(ui_font, target, players[i], this_left, this_top + rect_height / 2, 1.f, 1.f);
            }
        }

        static AllocatedString<32> username;
        do_text_box(&username, &ui_context, target, input_state, ui_font, &username, {500, 900, 300, 50}, 8, {1, 0, 0, .8});

        String buttonstr = String::from("Set Name");
        bool set_name = do_button(&buttonstr, &ui_context, target, input_state, ui_font, buttonstr, {850, 900, 0, 50}, 8, {0, 0, 1, .8});
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
            bool start_game = do_button(&buttonstr, &ui_context, target, input_state, ui_font, buttonstr, {25, 25, 0, 75}, 15, {0, 1, 1, .8});
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
        do_label(&str, &ui_context, target, input_state, ui_font, str, {500, 900, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::START_ROUND:
    {
        char round_char = (round_num % 10) + '0';
        AllocatedString<32> str;
        str = String::from("Round ");
        str.append(round_char);

        do_label(&str, &ui_context, target, input_state, ui_font, str, {500, 900, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::START_FACEOFF:
    {
        String str = String::from("Now Facing Off:");
        do_label(nullptr, &ui_context, target, input_state, ui_font, str, {10, 10, 0, 50}, {1, 0, 0, .8});
        do_label(nullptr, &ui_context, target, input_state, ui_font, players[faceoffer0_i], {10, 70, 0, 50}, {1, 0, 0, .8});
        do_label(nullptr, &ui_context, target, input_state, ui_font, players[faceoffer1_i], {10, 130, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::ASK_QUESTION:
    {
        String str = String::from("Answer this question:");
        do_label(nullptr, &ui_context, target, input_state, ui_font, str, {10, 10, 0, 50}, {1, 0, 0, .8});
        do_label(nullptr, &ui_context, target, input_state, ui_font, question, {10, 130, 0, 50}, {1, 0, 0, .8});

        String buttonstr = String::from("BUZZ");
        bool buzz = do_button(&buttonstr, &ui_context, target, input_state, ui_font, buttonstr, {850, 900, 0, 50}, 8, {0, 0, 1, .8});
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
        do_label(nullptr, &ui_context, target, input_state, ui_font, question, {10, 130, 0, 50}, {1, 0, 0, .8});

        if (answering_player_i == my_id)
        {
            static AllocatedString<32> answer;
            do_text_box(&answer, &ui_context, target, input_state, ui_font, &answer, {500, 900, 300, 50}, 8, {1, 0, 0, .8});

            String buttonstr = String::from("Submit");
            bool submit = do_button(&buttonstr, &ui_context, target, input_state, ui_font, buttonstr, {850, 900, 0, 50}, 8, {0, 0, 1, .8});
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
            do_label(nullptr, &ui_context, target, input_state, ui_font, str, {10, 190, 0, 50}, {1, 0, 0, .8});
        }
    }
    break;
    case ServerMessageType::PLAYER_BUZZED:
    {
        AllocatedString<64> str;
        str = players[answering_player_i];
        str.append(String::from(" buzzed"));
        do_label(nullptr, &ui_context, target, input_state, ui_font, str, {10, 190, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::PLAYER_SAID_SOMETHING:
    {
        AllocatedString<64> str;
        str = players[answering_player_i];
        str.append(String::from(" said:"));
        do_label(nullptr, &ui_context, target, input_state, ui_font, str, {10, 130, 0, 50}, {1, 0, 0, .8});
        do_label(nullptr, &ui_context, target, input_state, ui_font, what_player_said, {10, 190, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::DO_A_FLIP:
    {
        AllocatedString<32> str;
        char rank_char = (flip_answer_rank % 10) + '0';
        str = String::from("Correct Answer with rank: ");
        str.append(rank_char);

        do_label(nullptr, &ui_context, target, input_state, ui_font, str, {10, 130, 0, 50}, {1, 0, 0, .8});
        do_label(nullptr, &ui_context, target, input_state, ui_font, flip_answer_text, {10, 190, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::DO_AN_EEEEEGGGHHHH:
    {
        char incorrects_char = (incorrects % 10) + '0';
        AllocatedString<32> str;
        str = String::from("EEEGGGGHHH x");
        str.append(incorrects_char);
        do_label(nullptr, &ui_context, target, input_state, ui_font, str, {10, 190, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::PROMPT_PASS_OR_PLAY:
    {
        char pass_or_play;
        String pass_str = String::from("PASS");
        bool pass = do_button(&pass_str, &ui_context, target, input_state, ui_font, pass_str, {10, 130, 0, 50}, 8, {0, 0, 1, .8});
        if (pass)
        {
            pass_or_play = 0;
        }

        String play_str = String::from("PLAY");
        bool play = do_button(&play_str, &ui_context, target, input_state, ui_font, play_str, {10, 190, 0, 50}, 8, {0, 0, 1, .8});
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
        do_label(nullptr, &ui_context, target, input_state, ui_font, str, {10, 130, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::START_STEAL:
    {
        AllocatedString<64> str;
        str = String::from("Family ");
        str.append(playing_family + '0');
        str.append(String::from(" can now steal!"));
        do_label(nullptr, &ui_context, target, input_state, ui_font, str, {10, 130, 0, 50}, {1, 0, 0, .8});
    }
    break;
    case ServerMessageType::END_ROUND:
    {
        AllocatedString<64> str1;
        str1 = String::from("Round ");
        str1.append(round_num % 10 + '0');
        str1.append(String::from(" is now over :("));
        do_label(nullptr, &ui_context, target, input_state, ui_font, str1, {10, 70, 0, 50}, {1, 0, 0, .8});

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

    glEnable(GL_DEPTH_TEST);

    return true;
}

void deinit()
{
    closesocket(server_socket);
    WSACleanup();
}