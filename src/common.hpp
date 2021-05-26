#pragma once

#include <cstdint>

const int MAX_GAMES = 128;
const int MAX_PLAYERS_PER_GAME = 12;
const int MAX_CLIENTS = MAX_PLAYERS_PER_GAME * MAX_GAMES;

typedef uint32_t ClientId;
typedef uint32_t GameId;