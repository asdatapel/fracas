#pragma once

#include "../common.hpp"
#include "../util.hpp"

const uint16_t MAX_MSG_SIZE = 1024;

struct Peer;
struct Client;

enum struct ClientMessageType : uint8_t {
  LIST_GAMES,
  CREATE_GAME,
  JOIN_GAME,
  LEAVE_GAME,

  HOST_START_GAME,
  HOST_START_ASK_QUESTION,
  HOST_RESPOND_TO_ANSWER,
  HOST_END_GAME,

  READY,
  BUZZ,
  PASS_OR_PLAY,
  ANSWER,

  CELEBRATE,

  INVALID,
};

enum struct ServerMessageType : uint8_t {
  LIST_GAMES_RESPONSE = 0,
  JOIN_GAME_RESPONSE,

  JOIN_RESPONSE,
  DESCRIBE_LOBBY,
  START_GAME,
  START_ROUND,
  START_FACEOFF,
  ASK_QUESTION,
  PROMPT_PASS_OR_PLAY,
  PROMPT_FOR_ANSWER,
  PLAYER_BUZZED,
  START_PLAY,
  START_STEAL,
  PLAYER_SAID_SOMETHING,
  DO_A_FLIP,
  DO_AN_EEEEEGGGHHHH,
  END_ROUND,

  GAME_CREATED,
  GAME_ENDED,

  PLAYER_LEFT,

  INVALID,
};

char *append_byte(char *buf, char val);
char *append_bool(char *buf, bool val);
char *append_short(char *buf, uint16_t val);
char *append_long(char *buf, uint64_t val);
char *append_string(char *buf, char *str, uint16_t len);
char *append_string(char *buf, String str);
char *read_byte(char *buf, char *val);
char *read_bool(char *buf, bool *val);
char *read_short(char *buf, uint16_t *val);
char *read_uint32(char *buf, uint32_t *val);
char *read_long(char *buf, uint64_t *val);
char *read_string(char *buf, char *output_buf, uint16_t *len);
char *read_string(char *buf, String *output);
// returns pointer to string within buf, no copying;
char *read_string_inplace(char *buf, char **val, uint16_t *len);

struct MessageBuilder {
  char data_buf[MAX_MSG_SIZE];
  char *data;

  MessageBuilder();
  MessageBuilder(char header_type);
  void reset();
  void reset(char header_type);

  uint16_t get_len();
  void send(Peer *peer);
};
void append(MessageBuilder *msg, char val);
void append(MessageBuilder *msg, uint8_t val);
void append(MessageBuilder *msg, bool val);
void append(MessageBuilder *msg, uint16_t val);
void append(MessageBuilder *msg, uint32_t val);
void append(MessageBuilder *msg, int32_t val);
void append(MessageBuilder *msg, uint64_t val);
void append(MessageBuilder *msg, char *str, uint16_t len);
// void append(MessageBuilder *msg, String str);
template <size_t N>
void append(MessageBuilder *msg, AllocatedString<N> &str);

struct MessageReader {
  char data_buf[MAX_MSG_SIZE];
  char *data;

  char *end;

  MessageReader(char *data, uint16_t len);
  void check(char *ptr);
};

uint32_t read(MessageReader *msg, char *val);
uint32_t read(MessageReader *msg, uint8_t *val);
uint32_t read(MessageReader *msg, bool *val);
uint32_t read(MessageReader *msg, uint16_t *val);
uint32_t read(MessageReader *msg, uint32_t *val);
uint32_t read(MessageReader *msg, int32_t *val);
uint32_t read(MessageReader *msg, uint64_t *val);
uint32_t read(MessageReader *msg, char *output_buf, uint16_t *len);
// uint32_t read(MessageReader *msg, String *output);
template <size_t N>
uint32_t read(MessageReader *msg, AllocatedString<N> *str);
// returns pointer to string within buf, no copying
uint32_t read_string_inplace(MessageReader *msg, char **val, uint16_t *len);
