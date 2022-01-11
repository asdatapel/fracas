#pragma once

#include <cstdint>

#include "util.hpp"

const int MAX_GAMES = 128;
const int MAX_PLAYERS_PER_GAME = 12;
const int MAX_CLIENTS = MAX_PLAYERS_PER_GAME * MAX_GAMES;

typedef int32_t ClientId;
typedef int32_t GameId;

typedef AllocatedString<64> PlayerName;

float do_breakpoint = false;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef bool b8;
typedef float f32;
typedef double f64;

typedef i32 EntityId;