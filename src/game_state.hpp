#pragma once

#include <cassert>
#include <chrono>
#include <utility>

#include "util.hpp"

Array<i32, 5> ROUND_MULTIPLIERS = {1, 1, 2, 2, 3};

enum struct RoundStage {
  START,
  FACEOFF,
  PLAY,
  STEAL,
  END,
};
struct PlayerData {
  ClientId id = -1;
  PlayerName name;
  int family = -1;
  bool ready = false;
};
struct AnswerState {
  bool revealed = false;
  i32 score     = 0;
  i32 index = -1;
  String answer;
};
struct GameState {
  Array<PlayerName, 2> family_names               = {{}, {}};
  Array<PlayerData, MAX_PLAYERS_PER_GAME> players = {};

  String question;
  Array<AnswerState, 8> answers;

  int round              = -1;
  RoundStage round_stage = RoundStage::START;
  int this_round_points  = 0;
  int incorrects         = 0;
  int round_winner       = -1;

  Array<int, 2> current_players = {0, 0};
  int playing_family            = -1;
  int buzzing_family            = -1;
  int faceoff_winning_family    = -1;

  Array<int, 2> scores = {{}, {}};

  i32 last_answer_index = -1;
  ClientId last_answer_client_id;

  PlayerData *get_player_data(ClientId client_id)
  {
    for (int i = 0; i < players.len; i++) {
      if (players[i].id == client_id) {
        return &players[i];
      }
    }

    return nullptr;
  }

  i32 get_player_position(ClientId id)
  {
    i32 family_counts[2] = {0, 0};
    for (int i = 0; i < players.len; i++) {
      if (players[i].id == id) {
        return family_counts[players[i].family];
      }
      family_counts[players[i].family]++;
    }

    return -1;
  }

  int num_players() { return players.len; }

  int num_players(int family)
  {
    int count = 0;
    for (int i = 0; i < players.len; i++) {
      if (players[i].family == family) {
        count++;
      }
    }
    return count;
  }

  bool are_all_players_ready()
  {
    for (int i = 0; i < players.len; i++) {
      if (!players[i].ready) {
        return false;
      }
    }
    return true;
  }

  PlayerData *get_indexed_player(int family, int index)
  {
    int count = 0;
    for (int i = 0; i < players.len; i++) {
      if (players[i].family == family) {
        if (count == index) {
          return &players[i];
        }
        count++;
      }
    }
    return nullptr;
  }

  std::pair<PlayerData *, PlayerData *> faceoff_players()
  {
    int family1Index = round % num_players(0);
    int family2Index = round % num_players(1);
    return {get_indexed_player(0, family1Index), get_indexed_player(1, family2Index)};
  }

  bool waiting_for_buzz() { return round_stage == RoundStage::FACEOFF && buzzing_family == -1; }

  PlayerData *who_buzzed()
  {
    auto faceoffers = faceoff_players();

    if (buzzing_family == 0) {
      return faceoffers.first;
    } else if (buzzing_family == 1) {
      return faceoffers.second;
    } else {
      return nullptr;
    }
  }

  PlayerData *who_didnt_buzz()
  {
    auto faceoffers = faceoff_players();
    if (buzzing_family == 1) {
      return faceoffers.first;
    } else if (buzzing_family == 0) {
      return faceoffers.second;
    } else {
      return nullptr;
    }
  }

  PlayerData *who_won_faceoff()
  {
    auto faceoffers = faceoff_players();

    if (faceoff_winning_family == 0) {
      return faceoffers.first;
    } else if (faceoff_winning_family == 1) {
      return faceoffers.second;
    } else {
      return nullptr;
    }
  }

  PlayerData *whose_turn()
  {
    int index = current_players[playing_family] % num_players(playing_family);
    return get_indexed_player(playing_family, index);
  }

  int which_team_is_this_player_in(ClientId client_id)
  {
    for (int i = 0; i < players.len; i++) {
      if (players[i].id == client_id) {
        return players[i].family;
      }
    }
    return -1;
  }

  PlayerData *who_can_answer()
  {
    if (round_stage == RoundStage::FACEOFF) {
      if (buzzing_family == -1)  // no one has buzzed
        return nullptr;
      else if (this_round_points == 0 && incorrects == 0)  // no one has answered yet
      {
        // only the buzzer can answer
        return who_buzzed();
      } else  // someone has buzzed, answered, and not gotten the top answer
      {
        // only the other person in the faceoff can answer
        return who_didnt_buzz();
      }
    } else if (round_stage == RoundStage::STEAL) {
      int stealing_family = 1 - playing_family;
      return get_indexed_player(stealing_family,
                                0);  // when stealing, only the head of the family can answer
    } else if (round_stage == RoundStage::PLAY) {
      return whose_turn();
    }

    // can't answer during any of the other stages
    return nullptr;
  }

  bool are_all_answers_flipped()
  {
    for (int i = 0; i < answers.len; i++) {
      AnswerState &a = answers[i];
      if (a.score != 0 && !a.revealed) return false;
    }
    return true;
  };

  bool is_player_in_this_game(ClientId client_id)
  {
    return which_team_is_this_player_in(client_id) >= 0;
  }
};

struct ClientGameData {
  ClientId my_id;
  GameId game_id;

  Array<PlayerName, 2> family_names               = {{}, {}};
  Array<PlayerData, MAX_PLAYERS_PER_GAME> players = {};
  PlayerData *get_player_data(ClientId client_id)
  {
    for (int i = 0; i < players.len; i++) {
      if (players[i].id == client_id) {
        return &players[i];
      }
    }

    return nullptr;
  }

  AllocatedString<128> question;
  i32 num_answers;
  Array<AnswerState, 8> answers;

  i32 round = -1;
  RoundStage round_stage;
  i32 this_round_score = 0;

  std::pair<ClientId, ClientId> faceoffers;
  i32 buzzing_family = -1;

  Array<i32, 2> scores = {{}, {}};

  i32 incorrects = 0;
};