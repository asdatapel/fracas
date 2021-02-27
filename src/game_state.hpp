#pragma once

#include <cassert>
#include <chrono>
#include <utility>

#include "net.hpp"
#include "platform.hpp"
#include "util.hpp"

enum struct RoundStage
{
    START,
    FACEOFF,
    PASS_OR_PLAY,
    PLAY,
    STEAL,
    END,
};
struct AnswerState
{
    bool revealed = false;
    int score = 0;
    String answer;
};
struct Family
{
    String name;
    Array<int, 6> players;
};
struct GameState
{
    uint64_t preempt_timer = 0;
    uint64_t answer_timer = 0;

    Array<Client, 12> clients;

    bool waiting_on_ready = false;
    int num_ready = 0;
    void (*next_stage)(GameState &) = nullptr;

    Array<Family, 2> families;

    Array<int, 2> current_players;
    int playing_family = -1;
    int buzzing_family = -1;
    int faceoff_winning_family = -1;

    int round = -1;
    RoundStage round_stage = RoundStage::START;
    int this_round_points = 0;
    int incorrects = 0;
    int round_winner = -1;

    AllocatedString<128> last_answer;

    AllocatedString<128> question;
    Array<AnswerState, 8> answers;

    Array<int, 2> scores;

    std::pair<int, int> faceoff_players()
    {
        int family1Index = round % families[0].players.len;
        int family2Index = round % families[1].players.len;
        return {families[0].players[family1Index], families[1].players[family2Index]};
    }

    int who_buzzed()
    {
        auto faceoffers = faceoff_players();
        return buzzing_family == 0 ? faceoffers.first : faceoffers.second;
    }

    int who_didnt_buzz()
    {
        auto faceoffers = faceoff_players();
        return buzzing_family == 1 ? faceoffers.first : faceoffers.second;
    }

    int who_won_faceoff()
    {
        auto faceoffers = faceoff_players();
        return faceoff_winning_family == 0 ? faceoffers.first : faceoffers.second;
    }

    int whose_turn()
    {
        int index = current_players[playing_family] % families[playing_family].players.len;
        return families[playing_family].players[index];
    }

    int who_can_answer()
    {
        if (round_stage == RoundStage::FACEOFF)
        {
            if (buzzing_family == -1) // no one has buzzed
                return -1;
            else if (this_round_points == 0 && incorrects == 0) // no one has answered yet
            {
                // only the buzzer can answer
                return who_buzzed();
            }
            else // someone has buzzed, answered, and not gotten the top answer
            {
                // only the other person in the faceoff can answer
                return who_didnt_buzz();
            }
        }
        else if (round_stage == RoundStage::STEAL)
        {
            auto stealing_family = families[1 - playing_family];
            return stealing_family.players[0]; // when stealing, only the head of the family can answer
        }
        else if (round_stage == RoundStage::PLAY)
        {
            return whose_turn();
        }

        // can't answer during any of the other stages
        return -1;
    }

    int check_answer()
    {
        for (int i = 0; i < answers.len; ++i)
        {
            auto &a = answers[i];
            if (strcmp(last_answer, answers[i].answer))
            {
                return i;
            }
        }

        return -1;
    }

    bool are_all_answers_flipped()
    {
        for (int i = 0; i < answers.len; i++)
        {
            AnswerState &a = answers[i];
            if (a.score != 0 && !a.revealed)
                return false;
        }
        return true;
    };

    void broadcast(char *msg, int len)
    {
        for (int i = 0; i < clients.len; i++)
        {
            clients[i].peer.send_all(msg, len);
        }
    }
};

void build_and_broadcast_msg(GameState &game, char *(*build_func)(GameState &game, char *))
{
    char msg[MAX_MSG_SIZE];
    char *end = build_func(game, msg + 2);

    uint16_t msg_len = end - msg;
    append_short(msg, msg_len);

    game.broadcast(msg, msg_len);
}

char *construct_msg_join_response(GameState &game, char *buf_pos, int client_i)
{
    buf_pos = append_byte(buf_pos, (char)ServerMessageType::JOIN_RESPONSE);
    buf_pos = append_byte(buf_pos, client_i);

    return buf_pos;
}

char *construct_msg_describe_lobby(GameState &game, char *buf_pos)
{
    buf_pos = append_byte(buf_pos, (char)ServerMessageType::DESCRIBE_LOBBY);
    buf_pos = append_byte(buf_pos, game.clients.len);
    for (int i = 0; i < game.clients.len; i++)
    {
        if (game.clients[i].username.len)
            buf_pos = append_string(buf_pos, game.clients[i].username);
        else
            buf_pos = append_string(buf_pos, String::from("Joining..."));
    }

    return buf_pos;
}

char *construct_msg_start_game(GameState &game, char *buf_pos)
{
    buf_pos = append_byte(buf_pos, (char)ServerMessageType::START_GAME);
    return buf_pos;
}

char *construct_msg_start_round(GameState &game, char *buf_pos)
{
    buf_pos = append_byte(buf_pos, (char)ServerMessageType::START_ROUND);
    buf_pos = append_byte(buf_pos, game.round + 1);
    return buf_pos;
}

char *construct_msg_start_faceoff(GameState &game, char *buf_pos)
{
    const auto &[faceoffer0_i, faceoffer1_i] = game.faceoff_players();

    buf_pos = append_byte(buf_pos, (char)ServerMessageType::START_FACEOFF);
    buf_pos = append_byte(buf_pos, faceoffer0_i);
    buf_pos = append_byte(buf_pos, faceoffer1_i);
    return buf_pos;
}

char *construct_msg_ask_question(GameState &game, char *buf_pos)
{
    buf_pos = append_byte(buf_pos, (char)ServerMessageType::ASK_QUESTION);
    buf_pos = append_string(buf_pos, game.question);
    return buf_pos;
}

char *construct_msg_prompt_pass_or_play(GameState &game, char *buf_pos)
{
    buf_pos = append_byte(buf_pos, (char)ServerMessageType::PROMPT_PASS_OR_PLAY);
    buf_pos = append_byte(buf_pos, game.who_won_faceoff());
    return buf_pos;
}

char *construct_msg_prompt_for_answer(GameState &game, char *buf_pos)
{
    buf_pos = append_byte(buf_pos, (char)ServerMessageType::PROMPT_FOR_ANSWER);
    buf_pos = append_byte(buf_pos, game.who_can_answer());

    uint64_t answer_end_time = std::chrono::system_clock::now().time_since_epoch().count() + game.answer_timer;
    printf("Sending timestamp: %llu\n", answer_end_time);
    buf_pos = append_long(buf_pos, answer_end_time);
    return buf_pos;
}

char *construct_msg_player_buzzed(GameState &game, char *buf_pos)
{
    buf_pos = append_byte(buf_pos, (char)ServerMessageType::PLAYER_BUZZED);
    buf_pos = append_byte(buf_pos, game.buzzing_family);
    return buf_pos;
}

char *construct_msg_start_play(GameState &game, char *buf_pos)
{
    buf_pos = append_byte(buf_pos, (char)ServerMessageType::START_PLAY);
    buf_pos = append_byte(buf_pos, game.playing_family);
    return buf_pos;
}

char *construct_msg_start_steal(GameState &game, char *buf_pos)
{
    buf_pos = append_byte(buf_pos, (char)ServerMessageType::START_STEAL);
    buf_pos = append_byte(buf_pos, 1 - game.playing_family);
    return buf_pos;
}

char *construct_msg_player_said_something(GameState &game, char *buf_pos, String text)
{
    buf_pos = append_byte(buf_pos, (char)ServerMessageType::PLAYER_SAID_SOMETHING);
    buf_pos = append_string(buf_pos, text);
    return buf_pos;
}

char *construct_msg_do_an_flip(GameState &game, char *buf_pos, int answer_i)
{
    buf_pos = append_byte(buf_pos, (char)ServerMessageType::DO_A_FLIP);
    buf_pos = append_byte(buf_pos, answer_i);
    buf_pos = append_string(buf_pos, game.answers[answer_i].answer);
    buf_pos = append_short(buf_pos, game.answers[answer_i].score);
    return buf_pos;
}

char *construct_msg_do_an_eeeegeggghghhghg(GameState &game, char *buf_pos)
{
    buf_pos = append_byte(buf_pos, (char)ServerMessageType::DO_AN_EEEEEGGGHHHH);
    buf_pos = append_byte(buf_pos, game.incorrects);
    return buf_pos;
}

char *construct_msg_end_round(GameState &game, char *buf_pos)
{
    buf_pos = append_byte(buf_pos, (char)ServerMessageType::END_ROUND);
    buf_pos = append_byte(buf_pos, game.round_winner); // -1 if no one won
    buf_pos = append_short(buf_pos, game.scores[0]);
    buf_pos = append_short(buf_pos, game.scores[1]);
    return buf_pos;
}

void set_next_stage(GameState &game, void (*func)(GameState &game))
{
    game.next_stage = func;
    game.preempt_timer = 0; // 10 seconds

    DEBUG_PRINT("waiting on READY\n");
    game.waiting_on_ready = true;
    game.num_ready = 0;
}

void do_next_stage(GameState &game)
{
    DEBUG_PRINT("done waiting on READY\n");
    game.waiting_on_ready = false;
    for (int i = 0; i < game.clients.len; i++)
    {
        game.clients[i].ready = false;
    }

    assert(game.next_stage);
    game.next_stage(game);
}

void stage_ask_question(GameState &game)
{
    build_and_broadcast_msg(game, construct_msg_ask_question);
}

void stage_start_faceoff(GameState &game)
{
    game.round_stage = RoundStage::FACEOFF;
    build_and_broadcast_msg(game, construct_msg_start_faceoff);

    set_next_stage(game, stage_ask_question);
}

void stage_prompt_for_answer(GameState &game)
{
    game.answer_timer = 30000000000; // 30 seconds

    build_and_broadcast_msg(game, construct_msg_prompt_for_answer);
}

void stage_prompt_pass_or_play(GameState &game)
{
    game.round_stage = RoundStage::PASS_OR_PLAY;
    build_and_broadcast_msg(game, construct_msg_prompt_pass_or_play);
}

void stage_start_play(GameState &game)
{
    game.round_stage = RoundStage::PLAY;

    build_and_broadcast_msg(game, construct_msg_start_play);

    set_next_stage(game, stage_prompt_for_answer);
}

void stage_start_steal(GameState &game)
{
    game.round_stage = RoundStage::STEAL;

    build_and_broadcast_msg(game, construct_msg_start_steal);

    set_next_stage(game, stage_prompt_for_answer);
}

void stage_start_round(GameState &game)
{
    game.round_stage = RoundStage::START;
    game.round++;

    game.playing_family = -1;
    game.buzzing_family = -1;
    game.faceoff_winning_family = -1;

    game.this_round_points = 0;
    game.incorrects = 0;
    game.round_winner = -1;

    game.last_answer.len = 0;

    game.question = String::from("What is the answer?");
    game.answers.append({false, 15, String::from("answer1")});
    game.answers.append({false, 15, String::from("answer2")});
    game.answers.append({false, 15, String::from("answer3")});

    build_and_broadcast_msg(game, construct_msg_start_round);

    set_next_stage(game, stage_start_faceoff);
};

void stage_end_round(GameState &game)
{
    game.round_stage = RoundStage::END;
    game.scores[game.round_winner] += game.this_round_points;

    build_and_broadcast_msg(game, construct_msg_end_round);

    set_next_stage(game, stage_start_round);
}

void stage_respond_to_answer(GameState &game)
{
    int answerIncorrect = 0; // used to simplify checking which faceoffer is answering
    int answer_i = game.check_answer();
    int score = answer_i == -1 ? 0 : game.answers[answer_i].score;
    if (answer_i == -1)
    {
        answerIncorrect = 1;
        game.incorrects += 1;

        build_and_broadcast_msg(game, construct_msg_do_an_eeeegeggghghhghg);
    }
    else
    {
        game.answers[answer_i].revealed = true;
        game.this_round_points += score;

        char msg[MAX_MSG_SIZE];
        char *end = construct_msg_do_an_flip(game, msg + 2, answer_i);
        uint16_t msg_len = end - msg;
        append_short(msg, msg_len);
        game.broadcast(msg, msg_len);
    }

    if (game.round_stage == RoundStage::FACEOFF)
    {
        if (game.this_round_points == score && game.incorrects == answerIncorrect) // no one has answered yet
        {
            if (answer_i == 0)
            {
                // buzzer wins automatically
                game.faceoff_winning_family = game.buzzing_family;
                set_next_stage(game, stage_prompt_pass_or_play);
            }
            else
            {
                set_next_stage(game, stage_prompt_for_answer);
            }
        }
        else
        {
            if (game.this_round_points == 0)
            {
                // both faceoffers failed, move on to next round
                set_next_stage(game, stage_end_round);
            }
            else if (score > game.this_round_points)
            {
                // non-buzzer wins
                game.faceoff_winning_family = 1 - game.buzzing_family;
                set_next_stage(game, stage_prompt_pass_or_play);
            }
            else
            {
                // buzzer wins
                game.faceoff_winning_family = game.buzzing_family;
                set_next_stage(game, stage_prompt_pass_or_play);
            }
        }
    }
    else if (game.round_stage == RoundStage::STEAL)
    {
        if (answer_i == -1) // wrong answer
        {
            game.round_winner = game.playing_family;
        }
        else // right answer
        {
            game.round_winner = 1 - game.playing_family;
        }

        // always progress round, since stealing family only gets one chance
        set_next_stage(game, stage_end_round);
    }
    else
    {
        if (game.incorrects == 3)
        {
            set_next_stage(game, stage_start_steal);
        }
        else
        {
            game.current_players[game.playing_family] += 1;

            if (game.are_all_answers_flipped())
            {
                game.round_winner = game.playing_family;
                set_next_stage(game, stage_end_round);
            }
            else
            {
                set_next_stage(game, stage_prompt_for_answer);
            }
        }
    }
};

void handleJOIN(GameState &game, char *data, int client_i)
{
    Client &client = game.clients[client_i];

    char *str;
    uint16_t str_len;
    data = read_string_inplace(data, &str, &str_len);
    client.set_username(String{str, str_len});

    char msg[MAX_MSG_SIZE];
    char *end = construct_msg_join_response(game, msg + 2, client_i);
    uint16_t msg_len = end - msg;
    append_short(msg, msg_len);
    client.peer.send_all(msg, msg_len);

    build_and_broadcast_msg(game, construct_msg_describe_lobby);
};

void handleREADY(GameState &game, char *data, int client_i)
{
    DEBUG_PRINT("Recieved READY\n");
    Client &client = game.clients[client_i];

    if (!game.waiting_on_ready)
        return;

    if (!client.ready)
    {
        client.ready = true;
        game.num_ready++;
        DEBUG_PRINT("READY count now at %d\n", game.num_ready);
    }

    if (game.num_ready == game.clients.len)
    {
        do_next_stage(game);
    }
};

void handleSTART(GameState &game, char *data, int client_i)
{
    DEBUG_PRINT("Recieved START\n");
    if (game.round == -1 && client_i == 0) // aka host
    {
        for (int i = 0; i < game.clients.len; i++)
        {
            if (!game.clients[i].username.len)
                return; // TODO send error
        }

        // init families
        game.families.len = 2;
        game.families[0].players.len = game.clients.len / 2;
        for (int i = 0; i < game.families[0].players.len; i++)
        {
            game.families[0].players[i] = i;
        }
        game.families[1].players.len = game.clients.len - game.families[0].players.len;
        for (int i = 0; i < game.families[1].players.len; i++)
        {
            game.families[1].players[i] = game.families[0].players.len + i;
        }

        game.current_players = {0, 0};

        game.round = -1;

        game.playing_family = -1;
        game.buzzing_family = -1;
        game.faceoff_winning_family = -1;

        game.round_stage = RoundStage::START;

        game.this_round_points = 0;
        game.incorrects = 0;
        game.round_winner = -1;

        game.last_answer.len = 0;

        game.question.len = 0;
        game.scores = {0, 0};

        build_and_broadcast_msg(game, construct_msg_start_game);

        DEBUG_PRINT("sent START\n");
        set_next_stage(game, stage_start_round);
    }
};

void handleBUZZ(GameState &game, char *data, int client_i)
{
    if (game.round_stage != RoundStage::FACEOFF || game.buzzing_family != -1)
        return;

    const auto &[faceoffer0_i, faceoffer1_i] = game.faceoff_players();
    if (client_i == faceoffer0_i)
    {
        game.buzzing_family = 0;
    }
    else if (client_i == faceoffer1_i)
    {
        game.buzzing_family = 1;
    }
    else
    {
        return;
    }

    build_and_broadcast_msg(game, construct_msg_player_buzzed);

    set_next_stage(game, stage_prompt_for_answer);
};

void handlePASS_OR_PLAY(GameState &game, char *data, int client_i)
{
    if (game.round_stage != RoundStage::PASS_OR_PLAY)
        return;

    if (client_i != game.who_won_faceoff())
        return;

    char play;
    data = read_byte(data, &play);

    if (play)
    {
        game.playing_family = game.faceoff_winning_family;

        auto construct_msg_player_said_play = [](GameState &game, char *buf_pos) {
            return construct_msg_player_said_something(game, buf_pos, String::from("Play!"));
        };
        build_and_broadcast_msg(game, construct_msg_player_said_play);
    }
    else
    {
        game.playing_family = 1 - game.faceoff_winning_family;

        auto construct_msg_player_said_pass = [](GameState &game, char *buf_pos) {
            return construct_msg_player_said_something(game, buf_pos, String::from("Pass!"));
        };
        build_and_broadcast_msg(game, construct_msg_player_said_pass);
    }
    set_next_stage(game, stage_start_play);
}

void handleANSWER(GameState &game, char *data, int client_i)
{
    if (client_i != game.who_can_answer())
        return;

    // TODO(asad): check timer here?

    data = read_string(data, &game.last_answer);

    auto construct_msg_player_said_answer = [](GameState &game, char *buf_pos) {
        return construct_msg_player_said_something(game, buf_pos, game.last_answer);
    };
    build_and_broadcast_msg(game, construct_msg_player_said_answer);

    set_next_stage(game, stage_respond_to_answer);
}

typedef void (*HandleFunc)(GameState&, char *, int);
HandleFunc handle_funcs[(int)ClientMessageType::INVALID] = {
    handleJOIN,         // JOIN
    handleREADY,        // READY
    handleSTART,        // START
    handleBUZZ,         // BUZZ
    handlePASS_OR_PLAY, // PASS_OR_PLAY
    handleANSWER,       // ANSWER
};

void server_tick(GameState &game, uint64_t elapsed_micros)
{
    if (game.waiting_on_ready)
    {
        game.preempt_timer += elapsed_micros;
        if (game.preempt_timer > 10000000000)
        {
            do_next_stage(game);
        }
    }
}