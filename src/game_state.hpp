#pragma once

#include <cassert>
#include <chrono>
#include <utility>

#include "net/net.hpp"
#include "net/generated_rpc_server.hpp"
#include "util.hpp"

const uint32_t second = 1000000000;

struct GameProperties
{
    ClientId owner;
    AllocatedString<64> name = {};
    bool is_self_hosted = false;
    AllocatedString<64> owner_name;
};

enum struct LobbyStage
{
    NOT_STARTED,
    IN_GAME,
    ENDED,
    DEAD,
};
enum struct RoundStage
{
    START,
    FACEOFF,
    PLAY,
    STEAL,
    END,
};
struct PlayerData
{
    ClientId id = 0;
    PlayerName name;
    int team = -1;
    bool ready = false;
};
struct AnswerState
{
    bool revealed = false;
    int score = 0;
    String answer;
};
struct GameState
{
    AllocatedString<128> question;
    Array<AnswerState, 8> answers;

    int round = -1;
    RoundStage round_stage = RoundStage::START;
    int this_round_points = 0;
    int incorrects = 0;
    int round_winner = -1;

    Array<int, 2> current_players;
    int playing_family = -1;
    int buzzing_family = -1;
    int faceoff_winning_family = -1;

    Array<int, 2> scores;

    AllocatedString<128> last_answer;
    ClientId last_answer_client_id;
};
struct Lobby
{
    GameProperties properties;
    LobbyStage stage = LobbyStage::NOT_STARTED;
    GameState game = {};

    Array<PlayerName, 2> family_names = {{}, {}};
    Array<PlayerData, MAX_PLAYERS_PER_GAME> players = {};

    typedef void (Lobby::*Stage)(Broadcaster);
    typedef void (Lobby::*Waiter)(Broadcaster, uint64_t);
    Stage next_stage = nullptr;
    Waiter waiter = nullptr;
    uint64_t waiter_deadline = 0;
    uint64_t waiter_elapsed = 0;

    bool ready_to_delete = false;

    Lobby() = default;
    Lobby(GameProperties properties)
    {
        this->properties = properties;
    }

    void set_waiter(Waiter waiter, uint64_t deadline)
    {
        this->waiter = waiter;
        waiter_deadline = deadline;
        waiter_elapsed = 0;
    }

    void set_next_stage(Stage stage)
    {
        for (int i = 0; i < players.len; i++)
        {
            players[i].ready = false;
        }
        next_stage = stage;

        set_waiter(&Lobby::waiter_all_ready, 5 * second);
    }

    PlayerData *get_player_data(ClientId client_id)
    {
        for (int i = 0; i < players.len; i++)
        {
            if (players[i].id == client_id)
            {
                return &players[i];
            }
        }

        return nullptr;
    }

    int num_players()
    {
        return players.len;
    }

    int num_players(int team)
    {
        int count = 0;
        for (int i = 0; i < players.len; i++)
        {
            if (players[i].team == team)
            {
                count++;
            }
        }
        return count;
    }

    bool are_all_players_ready()
    {
        for (int i = 0; i < players.len; i++)
        {
            if (!players[i].ready)
            {
                return false;
            }
        }
        return true;
    }

    ClientId get_indexed_player(int team, int i)
    {
        int count = 0;
        for (int i = 0; i < players.len; i++)
        {
            if (players[i].team == team)
            {
                if (count == i)
                {
                    return players[i].id;
                }
                count++;
            }
        }
        return -1;
    }

    std::pair<ClientId, ClientId> faceoff_players()
    {
        int family1Index = game.round % num_players(0);
        int family2Index = game.round % num_players(1);
        return {get_indexed_player(0, family1Index), get_indexed_player(1, family2Index)};
    }

    bool waiting_for_buzz()
    {
        return game.round_stage == RoundStage::FACEOFF &&
               game.buzzing_family == -1;
    }

    ClientId who_buzzed()
    {
        auto faceoffers = faceoff_players();

        if (game.buzzing_family == 0)
        {
            return faceoffers.first;
        }
        else if (game.buzzing_family == 1)
        {
            return faceoffers.second;
        }
        else
        {
            return -1;
        }
    }

    ClientId who_didnt_buzz()
    {
        auto faceoffers = faceoff_players();
        if (game.buzzing_family == 1)
        {
            return faceoffers.first;
        }
        else if (game.buzzing_family == 0)
        {
            return faceoffers.second;
        }
        else
        {
            return -1;
        }
    }

    ClientId who_won_faceoff()
    {
        auto faceoffers = faceoff_players();

        if (game.faceoff_winning_family == 0)
        {
            return faceoffers.first;
        }
        else if (game.faceoff_winning_family == 1)
        {
            return faceoffers.second;
        }
        else
        {
            return -1;
        }
    }

    ClientId whose_turn()
    {
        int index = game.current_players[game.playing_family] % num_players(game.playing_family);
        return get_indexed_player(game.playing_family, index);
    }

    int which_team_is_this_player_in(ClientId client_id)
    {
        for (int i = 0; i < players.len; i++)
        {
            if (players[i].id == client_id)
            {
                return players[i].team;
            }
        }
        return -1;
    }

    ClientId who_can_answer()
    {
        if (game.round_stage == RoundStage::FACEOFF)
        {
            if (game.buzzing_family == -1) // no one has buzzed
                return -1;
            else if (game.this_round_points == 0 && game.incorrects == 0) // no one has answered yet
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
        else if (game.round_stage == RoundStage::STEAL)
        {
            int stealing_family = 1 - game.playing_family;
            return get_indexed_player(stealing_family, 0); // when stealing, only the head of the family can answer
        }
        else if (game.round_stage == RoundStage::PLAY)
        {
            return whose_turn();
        }

        // can't answer during any of the other stages
        return -1;
    }

    // return rank of answer, -1 if incorrect
    int check_answer()
    {
        for (int i = 0; i < game.answers.len; ++i)
        {
            auto &a = game.answers[i];
            if (strcmp(game.last_answer, game.answers[i].answer))
            {
                return i;
            }
        }
        return -1;
    }

    bool are_all_answers_flipped()
    {
        for (int i = 0; i < game.answers.len; i++)
        {
            AnswerState &a = game.answers[i];
            if (a.score != 0 && !a.revealed)
                return false;
        }
        return true;
    };

    bool is_player_in_this_game(ClientId client_id)
    {
        if (properties.is_self_hosted && properties.owner == client_id)
        {
            return true;
        }
        return which_team_is_this_player_in(client_id) >= 0;
    }

    void add_player(ClientId client_id, AllocatedString<64> name)
    {
        if (is_player_in_this_game(client_id))
        {
            // TODO maybe return error
            return;
        }
        if (num_players() >= MAX_PLAYERS_PER_GAME)
        {
            // TODO return error GAME_FULL
            return;
        }

        int next_family = 0;
        if (num_players(1) < num_players(0))
        {
            next_family = 1;
        }
        players.append({client_id, name, next_family});
    }

    void remove_player(ClientId client_id, Broadcaster broadcaster)
    {
        for (int i = 0; i < players.len; i++)
        {
            if (players[i].id == client_id)
            {
                players.shift_delete(i);
            }
        }

        if (properties.owner == client_id)
        {
            end_game(broadcaster);
            // MAYBETODO we can switch the owner instead, as long as the game isn't self hosted
        }

        if (stage != LobbyStage::NOT_STARTED && (num_players(0) == 0 || num_players(1) == 0))
        {
            end_game(broadcaster);
        }

        broadcaster.broadcast(&RpcServer::PlayerLeft, PlayerLeftMessage{client_id});
    }

    bool ready_to_start()
    {
        if (stage != LobbyStage::NOT_STARTED)
            return false;

        int fam_0_count = num_players(0);
        int fam_1_count = num_players(1);
        return fam_0_count > 0 && fam_0_count < MAX_PLAYERS_PER_GAME / 2 &&
               fam_1_count > 0 && fam_1_count < MAX_PLAYERS_PER_GAME / 2;
    }

    void start_game(Broadcaster broadcaster, GameId game_id)
    {
        game = GameState();
        stage = LobbyStage::IN_GAME;

        broadcaster.broadcast(&RpcServer::GameStarted, GameStartedMessage{game_id});

        set_next_stage(&Lobby::stage_start_round);
    }

    bool has_started()
    {
        return game.round >= 0;
    }

    void end_game(Broadcaster broadcaster)
    {
        stage = LobbyStage::ENDED;
        broadcaster.broadcast(&RpcServer::InGameEndGame, Empty{});
    }

    void do_next_stage(Broadcaster broadcaster)
    {
        (this->*next_stage)(broadcaster);
    }

    void stage_start_round(Broadcaster broadcaster)
    {
        game.round++;
        game.round_stage = RoundStage::START;

        game.playing_family = -1;
        game.buzzing_family = -1;
        game.faceoff_winning_family = -1;

        game.this_round_points = 0;
        game.incorrects = 0;
        game.round_winner = -1;

        game.last_answer.len = 0;

        game.question = string_to_allocated_string<128>("name a color?");
        game.answers.append({false, 15, "red"});
        game.answers.append({false, 15, "green"});
        game.answers.append({false, 14, "blue"});
        game.answers.append({false, 13, "orange"});
        game.answers.append({false, 13, "pink"});
        game.answers.append({false, 13, "purple"});
        game.answers.append({false, 2, "muave"});

        broadcaster.broadcast(&RpcServer::InGameStartRound, InGameStartRoundMessage{game.round});
        set_next_stage(&Lobby::stage_start_faceoff);
    }

    void stage_start_faceoff(Broadcaster broadcaster)
    {
        game.round_stage = RoundStage::FACEOFF;

        broadcaster.broadcast(&RpcServer::InGameStartFaceoff, Empty{});
        set_next_stage(&Lobby::stage_ask_question);
    }

    void stage_ask_question(Broadcaster broadcaster)
    {
        broadcaster.broadcast(&RpcServer::InGameAskQuestion, InGameAskQuestionMessage{game.question});
        set_waiter(&Lobby::waiter_buzz, 10 * second);
    }

    void stage_prompt_for_answer(Broadcaster broadcaster)
    {
        broadcaster.broadcast(&RpcServer::InGamePromptForAnswer, InGamePromptForAnswerMessage{who_can_answer()});

        set_waiter(&Lobby::waiter_answer, 30 * second);
    }

    void stage_respond_to_answer(Broadcaster broadcaster)
    {
        int this_answer_incorrect = 0; // used to simplify checking which faceoffer is answering
        int answer_i = check_answer();
        int score = answer_i == -1 ? 0 : game.answers[answer_i].score;
        if (answer_i == -1)
        {
            this_answer_incorrect = 1;
            game.incorrects += 1;

            broadcaster.broadcast(&RpcServer::InGameEggghhhh, InGameEggghhhhMessage{game.incorrects});
        }
        else
        {
            game.answers[answer_i].revealed = true;
            game.this_round_points += score;

            broadcaster.broadcast(&RpcServer::InGameFlipAnswer, InGameFlipAnswerMessage{answer_i, game.last_answer, score});
        }

        if (game.round_stage == RoundStage::FACEOFF)
        {
            if (game.this_round_points == score && game.incorrects == this_answer_incorrect) // no one has answered yet
            {
                if (answer_i == 0)
                {
                    // buzzer wins automatically
                    game.faceoff_winning_family = game.buzzing_family;
                    set_next_stage(&Lobby::stage_prompt_pass_or_play);
                }
                else
                {
                    set_next_stage(&Lobby::stage_prompt_for_answer);
                }
            }
            else
            {
                if (game.this_round_points == 0)
                {
                    // both faceoffers failed, move on to next round
                    set_next_stage(&Lobby::stage_end_round);
                }
                else if (score > game.this_round_points)
                {
                    // non-buzzer wins
                    game.faceoff_winning_family = 1 - game.buzzing_family;
                    set_next_stage(&Lobby::stage_prompt_pass_or_play);
                }
                else
                {
                    // buzzer wins
                    game.faceoff_winning_family = game.buzzing_family;
                    set_next_stage(&Lobby::stage_prompt_pass_or_play);
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
            set_next_stage(&Lobby::stage_end_round);
        }
        else
        {
            if (game.incorrects == 3)
            {
                set_next_stage(&Lobby::stage_start_steal);
            }
            else
            {
                game.current_players[game.playing_family] += 1;

                if (are_all_answers_flipped())
                {
                    game.round_winner = game.playing_family;
                    set_next_stage(&Lobby::stage_end_round);
                }
                else
                {
                    set_next_stage(&Lobby::stage_prompt_for_answer);
                }
            }
        }
    };

    void stage_prompt_pass_or_play(Broadcaster broadcaster)
    {
        broadcaster.broadcast(&RpcServer::InGamePromptPassOrPlay, Empty{});
        set_waiter(&Lobby::waiter_pass_or_play, 10 * second);
    }

    void stage_start_play(Broadcaster broadcaster)
    {
        game.round_stage = RoundStage::PLAY;
        broadcaster.broadcast(&RpcServer::InGameStartPlay, InGameStartPlayMessage{game.faceoff_winning_family});
        set_next_stage(&Lobby::stage_prompt_for_answer);
    }

    void stage_start_steal(Broadcaster broadcaster)
    {
        game.round_stage = RoundStage::STEAL;
        broadcaster.broadcast(&RpcServer::InGameStartPlay, InGameStartPlayMessage{1 - game.faceoff_winning_family});
        set_next_stage(&Lobby::stage_prompt_for_answer);
    }

    void stage_end_round(Broadcaster broadcaster)
    {
        game.round_stage = RoundStage::END;
        game.scores[game.round_winner] += game.this_round_points;

        broadcaster.broadcast(&RpcServer::InGameEndRound, Empty{});
        set_next_stage(&Lobby::stage_start_round);
    }

    void stage_end_game(Broadcaster broadcaster)
    {
        game.round_stage = RoundStage::END;
        end_game(broadcaster);

        set_waiter(&Lobby::waiter_end_game, 10 * 60 * second);
    }

    void waiter_buzz(Broadcaster broadcaster, uint64_t elapsed_micros)
    {
        waiter_elapsed += elapsed_micros;
        if (waiter_elapsed >= waiter_deadline)
        {
            game.incorrects += 1;
            broadcaster.broadcast(&RpcServer::InGameEggghhhh, InGameEggghhhhMessage{game.incorrects});
            set_next_stage(&Lobby::stage_end_round);
        }
    }
    void waiter_pass_or_play(Broadcaster broadcaster, uint64_t elapsed_micros)
    {
        waiter_elapsed += elapsed_micros;
        if (waiter_elapsed >= waiter_deadline)
        {
            // default to PLAY
            game.playing_family = game.faceoff_winning_family;
            broadcaster.broadcast(&RpcServer::InGamePlayerChosePassOrPlay, InGameChoosePassOrPlayMessage{true});
            set_next_stage(&Lobby::stage_start_play);
        }
    }
    void waiter_answer(Broadcaster broadcaster, uint64_t elapsed_micros)
    {
        waiter_elapsed += elapsed_micros;
        if (waiter_elapsed >= waiter_deadline)
        {
            AllocatedString<64> empty_answer = string_to_allocated_string<64>("...");
            game.last_answer = empty_answer;
            broadcaster.broadcast(&RpcServer::InGamePlayerAnswered, InGameAnswerMessage{empty_answer});

            set_next_stage(&Lobby::stage_respond_to_answer);
        }
    }
    void waiter_all_ready(Broadcaster broadcaster, uint64_t elapsed_micros)
    {
        waiter_elapsed += elapsed_micros;
        if (waiter_elapsed >= waiter_deadline)
        {
            do_next_stage(broadcaster);
        }
    }
    void waiter_end_game(Broadcaster broadcaster, uint64_t elapsed_micros)
    {
        waiter_elapsed += elapsed_micros;
        if (waiter_elapsed >= waiter_deadline)
        {
            ready_to_delete = true;
        }
    }

    void tick(Broadcaster broadcaster, uint64_t elapsed_time)
    {
        if (waiter)
        {
            (this->*waiter)(broadcaster, elapsed_time);
        }
    }
};
