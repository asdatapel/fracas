#pragma once

#include <cstdint>

#include "util.hpp"

const int MAX_GAMES = 128;
const int MAX_PLAYERS_PER_GAME = 12;
const int MAX_CLIENTS = MAX_PLAYERS_PER_GAME * MAX_GAMES;

typedef int32_t ClientId;
typedef int32_t GameId;

typedef AllocatedString<64> PlayerName;