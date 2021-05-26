#pragma once

#include "net.hpp"

void append(MessageBuilder *msg, GameProperties *game_properties)
{

    msg->append(game_properties->owner);
    msg->append(game_properties->name);
    msg->append(game_properties->is_self_hosted);
}

uint32_t read(MessageReader *msg, GameProperties *game_properties)
{
    uint32_t len = 0;
    len += msg->read(&game_properties->owner);
    len += msg->read(&game_properties->name);
    len += msg->read(&game_properties->is_self_hosted);

    return len;
}

struct JoinGameMessage
{
    GameId game_id;
    ClientId my_id;
    GameProperties game_properties;
};
void append(MessageBuilder *msg, JoinGameMessage *out)
{
    msg->append(out->game_id);
    msg->append(out->my_id);
    append(msg, &out->game_properties);
}
uint32_t read(MessageReader *msg, JoinGameMessage *in)
{
    uint32_t len = 0;
    len += msg->read(&in->game_id);
    len += msg->read(&in->my_id);
    len += read(msg, &in->game_properties);

    return len;
}

struct DescribeGameMessage
{
    GameId game_id;
    uint8_t num_players;
    AllocatedString<32> players[MAX_PLAYERS_PER_GAME];
};
void append(MessageBuilder *msg, DescribeGameMessage *out)
{
    msg->append(out->game_id);
    msg->append(out->num_players);
    for (int i = 0; i < out->num_players && i < MAX_PLAYERS_PER_GAME; i++)
    {
        msg->append(out->players[i]);
    }
}
uint32_t read(MessageReader *msg, DescribeGameMessage *in)
{
    uint32_t len = 0;
    len += msg->read(&in->game_id);
    len += msg->read(&in->num_players);
    for (int i = 0; i < in->num_players && i < MAX_PLAYERS_PER_GAME; i++)
    {
        len += msg->read(&in->players[i]);
    }

    return len;
}
