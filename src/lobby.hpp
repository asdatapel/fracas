#pragma once

#include <map>

#include "common.hpp"
#include "game_state.hpp"
#include "net.hpp"
#include "serialize.hpp"

std::map<GameId, GameState> games;
std::map<ClientId, Client> clients;

ClientId add_client(SOCKET s)
{
    static ClientId next_client_id = 1;
    next_client_id++;

    if (clients.size() > MAX_CLIENTS)
    {
        // TODO send error message SERVER_FULL
        return 0;
    }

    Client client;
    client.peer.socket = s;
    client.client_id = next_client_id;

    clients[next_client_id] = client;
    return next_client_id;
}

void remove_client(ClientId client_id)
{
    Client *client = &clients[client_id];
    if (client->game_id && games.count(client->game_id))
    {
        GameState *game = &games[client->game_id];
        game->add_player(client_id);
    }
    clients.erase(client_id);
}

GameId add_game(GameProperties properties)
{
    static GameId next_game_id = 1;
    next_game_id++;

    if (games.size() > MAX_GAMES)
    {
        return 0;
    }

    games[next_game_id] = GameState{properties};
    return next_game_id;
}

void handle_LIST_GAMES(MessageReader *msg, Client *client)
{
    // message should be otherwise empty, maybe it can contain a filter in the future

    const int PAGE_SIZE = 10;

    MessageBuilder resp((char)ServerMessageType::LIST_GAMES_RESPONSE);
    uint16_t count = 0;
    for (auto it : games)
    {
        GameId game_id = it.first;
        GameState *game = &it.second;
        if (game->has_started())
        {
            continue;
        }

        resp.append(count);
        resp.append(game_id);
        append(&resp, &game->properties);

        count++;
        if (count % PAGE_SIZE == 0)
        {
            resp.send(&client->peer);
            resp.reset((char)ServerMessageType::LIST_GAMES_RESPONSE);
        }
    }

    resp.send(&client->peer);
}

void handle_CREATE_GAME(MessageReader *msg, Client *client)
{
    GameProperties game_properties;
    game_properties.owner = client->client_id;
    msg->read(&game_properties.name);
    msg->read(&game_properties.is_self_hosted);

    GameId game_id = add_game(game_properties);
    if (game_id)
    {
        GameState *game = &games[game_id];

        MessageBuilder resp((char)ServerMessageType::JOIN_GAME_RESPONSE);
        JoinGameMessage out;
        out.game_id = game_id;
        out.my_id = client->client_id;
        out.game_properties = game->properties;
        append(&resp, &out);
        resp.send(&client->peer);
        
        // DescribeGameMessage out;
        // out.game_id = game_id;
        // out.num_players = game->num_players();
        // for (int i = 0; i < game->families[0].players.len; i++)
        // {
        //     out.players->append(game->families[0].players[i]);
        // }
        // for (int i = 0; i < game->families[1].players.len; i++)
        // {
        //     out.players->append(game->families[1].players[i]);
        // }

        // append(&resp, &game_properties);
        // resp.send(&client->peer);
    }
    else
    {
        // TODO send error message TOO_MANY_GAMES
    }
}

void handle_JOIN_GAME(MessageReader *msg, Client *client)
{
    GameId game_id;
    msg->read(&game_id);

    if (client->game_id != 0)
    {
        // TODO send error YOURE_ALREADY_IN_A_GAME_IDIOT
        return;
    }

    if (games.count(game_id) == 0)
    {
        // TODO send error REQUESTED_GAME_NOT_FOUND
        return;
    }

    GameState *game = &games[game_id];
    game->add_player(client->client_id);
    client->game_id = game_id;

    MessageBuilder resp((char)ServerMessageType::JOIN_GAME_RESPONSE);
    append(&resp, &game->properties);
    resp.send(&client->peer);
}

void handle_LEAVE_GAME(MessageReader *msg, Client *client)
{
    if (client->game_id == 0)
    {
        // TODO send error NOT_IN_GAME
        return;
    }

    if (games.count(client->game_id) == 0)
    {
        // TODO send error INTERNAL_ERROR
        return;
    }

    GameState *game = &games[client->game_id];
    game->remove_player(client->client_id);
    client->game_id = 0;
}