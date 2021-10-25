#pragma once

#include <cassert>
#include <chrono>
#include <utility>

#include "util.hpp"

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
    Array<PlayerName, 2> family_names = {{}, {}};
    Array<PlayerData, MAX_PLAYERS_PER_GAME> players = {};

    AllocatedString<128> question;
    Array<AnswerState, 8> answers;

    int round = -1;
    RoundStage round_stage = RoundStage::START;
    int this_round_points = 0;
    int incorrects = 0;
    int round_winner = -1;

    Array<int, 2> current_players = {0, 0};
    int playing_family = -1;
    int buzzing_family = -1;
    int faceoff_winning_family = -1;

    Array<int, 2> scores;

    AllocatedString<128> last_answer;
    ClientId last_answer_client_id;

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

    ClientId get_indexed_player(int team, int index)
    {
        int count = 0;
        for (int i = 0; i < players.len; i++)
        {
            if (players[i].team == team)
            {
                if (count == index)
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
        int family1Index = round % num_players(0);
        int family2Index = round % num_players(1);
        return {get_indexed_player(0, family1Index), get_indexed_player(1, family2Index)};
    }

    bool waiting_for_buzz()
    {
        return round_stage == RoundStage::FACEOFF &&
               buzzing_family == -1;
    }

    ClientId who_buzzed()
    {
        auto faceoffers = faceoff_players();

        if (buzzing_family == 0)
        {
            return faceoffers.first;
        }
        else if (buzzing_family == 1)
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
        if (buzzing_family == 1)
        {
            return faceoffers.first;
        }
        else if (buzzing_family == 0)
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

        if (faceoff_winning_family == 0)
        {
            return faceoffers.first;
        }
        else if (faceoff_winning_family == 1)
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
        int index = current_players[playing_family] % num_players(playing_family);
        return get_indexed_player(playing_family, index);
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
            int stealing_family = 1 - playing_family;
            return get_indexed_player(stealing_family, 0); // when stealing, only the head of the family can answer
        }
        else if (round_stage == RoundStage::PLAY)
        {
            return whose_turn();
        }

        // can't answer during any of the other stages
        return -1;
    }

    // return rank of answer, -1 if incorrect
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

    bool is_player_in_this_game(ClientId client_id)
    {
        return which_team_is_this_player_in(client_id) >= 0;
    }
};

struct ClientGameData
{
    ClientId my_id;
    GameState game_state;
};