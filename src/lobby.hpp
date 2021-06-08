#pragma once

#include <unordered_map>

#include "common.hpp"
#include "game_state.hpp"
#include "net/net.hpp"
#include "net/generated_rpc_server.hpp"

struct ServerData
{
    std::unordered_map<GameId, GameState> games;
    std::unordered_map<ClientId, Client> clients;
};

ClientId add_client(ServerData *server_data, SOCKET s)
{
    static ClientId next_client_id = 1;
    next_client_id++;

    if (server_data->clients.size() > MAX_CLIENTS)
    {
        // TODO send error message SERVER_FULL
        return 0;
    }

    Client client;
    client.peer.s = s;
    client.client_id = next_client_id;

    server_data->clients[next_client_id] = client;
    return next_client_id;
}

void remove_client(ServerData *server_data, ClientId client_id)
{
    Client *client = &server_data->clients[client_id];
    if (client->game_id && server_data->games.count(client->game_id))
    {
        GameState &game = server_data->games[client->game_id];
        game.remove_player(client_id);
    }
}

GameId add_game(ServerData *server_data, GameProperties properties)
{
    static GameId next_game_id = 1;
    next_game_id++;

    if (server_data->games.size() > MAX_GAMES)
    {
        return 0;
    }

    server_data->games[next_game_id] = GameState{properties};
    server_data->games[next_game_id].add_player(properties.owner);
    return next_game_id;
}

void RpcServer::ListGames(ClientId client_id, ListGamesRequest *req, ListGamesResponse *resp)
{
    uint16_t count = 0;
    for (auto it : server_data->games)
    {
        GameId game_id = it.first;
        GameState *game = &it.second;

        if (game->stage == GameStage::NOT_STARTED)
        {
            GameMetadata game_msg;
            game_msg.id = game_id;
            game_msg.name = game->properties.name;
            game_msg.owner = server_data->clients[game->properties.owner].username;
            game_msg.num_players = game->num_players();
            game_msg.is_self_hosted = game->properties.is_self_hosted;
            resp->games.push_back(game_msg);
        }
    }
}

void RpcServer::GetGame(ClientId client_id, GetGameRequest *req, GetGameResponse *resp)
{
    if (server_data->games.count(req->game_id) == 0)
    {
        // TODO send NOT_FOUND
        return;
    }
    GameState *game = &server_data->games[req->game_id];

    resp->game.id = req->game_id;
    resp->game.name = game->properties.name;
    resp->game.owner = server_data->clients[game->properties.owner].username;
    resp->game.num_players = game->num_players();
    resp->game.is_self_hosted = game->properties.is_self_hosted;
    for (int i = 0; i < game->players.len; i++)
    {
        ClientId this_client_id = game->players[i].first;
        resp->players.push_back({this_client_id,
                                 server_data->clients[this_client_id].username,
                                 game->players[i].second == 1});
    }
}

void RpcServer::CreateGame(ClientId client_id, CreateGameRequest *req, CreateGameResponse *resp)
{
    Client *client = &server_data->clients[client_id];
    if (client->game_id)
    {
        return; // client is already in a game
    }

    // TODO validate settings

    GameProperties game_properties;
    game_properties.owner = client->client_id;
    game_properties.name = req->name;
    game_properties.is_self_hosted = req->is_self_hosted;

    GameId game_id = add_game(server_data, game_properties);
    if (!game_id)
    {
        // TODO send error message TOO_MANY_GAMES
    }

    GameState *game = &server_data->games[game_id];
    resp->game_id = game_id;
    resp->owner_id = client->client_id;
    client->game_id = game_id;
}

void RpcServer::JoinGame(ClientId client_id, JoinGameRequest *req, JoinGameResponse *resp)
{
    Client *client = &server_data->clients[client_id];
    if (client->game_id)
    {
        return; // client is already in a game
    }

    if (server_data->games.count(req->game_id) == 0)
    {
        // TODO send error, game NOT_FOUND
        return;
    }

    GameState *game = &server_data->games[req->game_id];
    game->add_player(client_id);
    client->game_id = req->game_id;
}

void RpcServer::SwapTeam(ClientId client_id, SwapTeamRequest *req, SwapTeamResponse *resp)
{
    Client *client = &server_data->clients[client_id];
    if (client->game_id != req->game_id)
    {
        return; // client is not in the requested game
    }
    if (server_data->games.count(req->game_id) == 0)
    {
        // TODO send error, game NOT_FOUND
        return;
    }
    GameState *game = &server_data->games[req->game_id];
    if (game->properties.owner != client_id)
    {
        return; // TODO client doesn't own this game
    }
    if (game->stage != GameStage::NOT_STARTED)
    {
        return; // TODO cannot swap after game has started
    }
    if (!game->is_player_in_this_game(req->user_id))
    {
        return; // error requested user isn't in this game
    }

    for (int i = 0; i < game->players.len; i++)
    {
        if (game->players[i].first == req->user_id)
        {
            game->players[i].second = 1 - game->players[i].second;
        }
    }
}

void RpcServer::LeaveGame(ClientId client_id, LeaveGameRequest *req, LeaveGameResponse *resp)
{
    Client *client = &server_data->clients[client_id];
    if (!client->game_id)
    {
        return; // client is not in a game
    }
    if (server_data->games.count(client->game_id) == 0)
    {
        // TODO send error INTERNAL_ERROR
        return;
    }

    GameState *game = &server_data->games[client->game_id];
    game->remove_player(client->client_id);
    client->game_id = 0;
}